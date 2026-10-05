#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include "ui/backend/VulkanOverlay.h"
#include "ui/backend/blur/Blur.h"
#include "ui/backend/blur/Gaussian.h"
#include "ui/backend/blur/shaders/GaussianSpirv.h"
#include "core/RuntimeControl.h"
#define NOMINMAX
#include <windows.h>
#include "third_party/minhook/include/MinHook.h"
#include "third_party/fatalwidgets/imgui/imgui.h"
#include "third_party/fatalwidgets/imgui/imgui_impl_vulkan.h"
#include <array>
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef NOVA_VULKAN_TEST
void VulkanTestCapture(VkCommandBuffer, VkImage, VkExtent2D);
#endif

namespace nova::vkoverlay {
namespace {
VkInstance instance{};
VkPhysicalDevice physical{};
VkDevice device{};
VkQueue queue{};
VkSurfaceKHR surface{};
VkSwapchainKHR initialSwapchain{};
uint32_t family{};
void* window{};
Callbacks callbacks{};
PFN_vkGetInstanceProcAddr gipa{};
PFN_vkGetDeviceProcAddr gdpa{};
PFN_vkQueuePresentKHR originalPresent{};
PFN_vkCreateSwapchainKHR originalCreate{};
PFN_vkDestroySwapchainKHR originalDestroy{};
std::array<void*, 3> targets{};
std::recursive_mutex mutex;
std::atomic<bool> disabled{false};
bool imguiReady{};
bool drew{};
VkCommandBuffer recording{};
uint32_t recordingImageIndex{};

#define NOVA_VK_FUNCTIONS(X) \
    X(GetPhysicalDeviceSurfaceCapabilitiesKHR) X(GetPhysicalDeviceQueueFamilyProperties) \
    X(GetSwapchainImagesKHR) X(CreateImageView) X(DestroyImageView) \
    X(CreateRenderPass) X(DestroyRenderPass) X(CreateFramebuffer) X(DestroyFramebuffer) \
    X(CreateCommandPool) X(DestroyCommandPool) X(AllocateCommandBuffers) X(ResetCommandPool) \
    X(BeginCommandBuffer) X(EndCommandBuffer) X(CmdBeginRenderPass) X(CmdEndRenderPass) \
    X(CreateSemaphore) X(DestroySemaphore) X(CreateFence) X(DestroyFence) \
    X(WaitForFences) X(ResetFences) X(QueueSubmit) X(QueueWaitIdle) \
    X(GetPhysicalDeviceFormatProperties) X(GetPhysicalDeviceMemoryProperties) \
    X(CreateImage) X(DestroyImage) X(GetImageMemoryRequirements) \
    X(AllocateMemory) X(FreeMemory) X(BindImageMemory) X(CmdPipelineBarrier) X(CmdBlitImage) \
    X(CreateSampler) X(DestroySampler) X(CreateDescriptorSetLayout) X(DestroyDescriptorSetLayout) \
    X(CreateDescriptorPool) X(DestroyDescriptorPool) X(AllocateDescriptorSets) X(UpdateDescriptorSets) \
    X(CreateShaderModule) X(DestroyShaderModule) X(CreatePipelineLayout) X(DestroyPipelineLayout) \
    X(CreateGraphicsPipelines) X(DestroyPipeline) X(CmdBindPipeline) X(CmdBindDescriptorSets) \
    X(CmdPushConstants) X(CmdSetViewport) X(CmdSetScissor) X(CmdDraw)
#define DECLARE(name) PFN_vk##name p##name{};
NOVA_VK_FUNCTIONS(DECLARE)
#undef DECLARE

struct Image {
    VkImage image{};
    VkImageView view{};
    VkFramebuffer framebuffer{};
    // A presentation semaphore belongs to the acquired image, not the CPU frame.
    VkSemaphore ready{};
};
struct Frame {
    VkCommandPool pool{};
    VkCommandBuffer command{};
    VkFence fence{};
};
struct Swapchain {
    VkSwapchainKHR handle{};
    VkFormat format{};
    VkExtent2D extent{};
    VkRenderPass pass{};
    uint32_t minImages{};
    uint64_t serial{};
    bool blurSupported = false;
    std::vector<Image> images;
    // Match ImGui's rotating buffers with the same number of fenced CPU slots.
    std::vector<Frame> frames;
} chain;

void Check(VkResult result) {
    if (result < VK_SUCCESS) throw std::runtime_error("Vulkan result " + std::to_string(result));
}
void Require(VkResult result) {
    if (result != VK_SUCCESS) throw std::runtime_error("Vulkan operation incomplete: " + std::to_string(result));
}
PFN_vkVoidFunction Function(const char* name) {
    auto value = gdpa ? gdpa(device, name) : nullptr;
    return value ? value : (gipa ? gipa(instance, name) : nullptr);
}
void Fail(const char* reason) {
    LogTo("Vulkan: %s", reason);
    ReportBootstrapFailed(reason);
    OpenCircuit();
    RequestUnload();
}

#include "ui/backend/blur/VulkanBlur.h"

void DestroyResources() {
    // Also reached on the game thread before Minecraft destroys/recreates its swapchain.
    // ImGui's backend owns font uploads/buffers/descriptors, Nova owns views and fences.
    if (device && queue && pQueueWaitIdle) Check(pQueueWaitIdle(queue));
    DestroyBlur();
    if (ImGui::GetCurrentContext() && ImGui::GetIO().BackendRendererUserData)
        ImGui_ImplVulkan_Shutdown();
    imguiReady = false;
    callbacks.rendererLost();
    for (auto& frame : chain.frames) {
        if (frame.pool) pDestroyCommandPool(device, frame.pool, nullptr);
        if (frame.fence) pDestroyFence(device, frame.fence, nullptr);
    }
    for (auto& image : chain.images) {
        if (image.framebuffer) pDestroyFramebuffer(device, image.framebuffer, nullptr);
        if (image.view) pDestroyImageView(device, image.view, nullptr);
        if (image.ready) pDestroySemaphore(device, image.ready, nullptr);
    }
    if (chain.pass) pDestroyRenderPass(device, chain.pass, nullptr);
    chain.frames.clear(); chain.images.clear(); chain.pass = VK_NULL_HANDLE;
    chain.serial = 0;
}

void CreateResources() {
    uint32_t count{};
    Require(pGetSwapchainImagesKHR(device, chain.handle, &count, nullptr));
    if (count < 2) throw std::runtime_error("Vulkan: swapchain has fewer than two images");
    std::vector<VkImage> images(count);
    Require(pGetSwapchainImagesKHR(device, chain.handle, &count, images.data()));
    images.resize(count);
    chain.images.resize(count); chain.frames.resize(count);
    VkAttachmentDescription color{};
    color.format = chain.format;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference attachment{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1; subpass.pColorAttachments = &attachment;
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    dependency.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    pass.attachmentCount = 1; pass.pAttachments = &color;
    pass.subpassCount = 1; pass.pSubpasses = &subpass;
    pass.dependencyCount = 1; pass.pDependencies = &dependency;
    Require(pCreateRenderPass(device, &pass, nullptr, &chain.pass));
    for (uint32_t i = 0; i < count; ++i) {
        auto& target = chain.images[i];
        target.image = images[i];
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image = images[i]; view.viewType = VK_IMAGE_VIEW_TYPE_2D; view.format = chain.format;
        view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        Require(pCreateImageView(device, &view, nullptr, &target.view));
        VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer.renderPass = chain.pass; framebuffer.attachmentCount = 1;
        framebuffer.pAttachments = &target.view;
        framebuffer.width = chain.extent.width; framebuffer.height = chain.extent.height;
        framebuffer.layers = 1;
        Require(pCreateFramebuffer(device, &framebuffer, nullptr, &target.framebuffer));
        VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        Require(pCreateSemaphore(device, &semaphore, nullptr, &target.ready));
        auto& slot = chain.frames[i];
        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pool.queueFamilyIndex = family; pool.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        Require(pCreateCommandPool(device, &pool, nullptr, &slot.pool));
        VkCommandBufferAllocateInfo alloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        alloc.commandPool = slot.pool; alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; alloc.commandBufferCount = 1;
        Require(pAllocateCommandBuffers(device, &alloc, &slot.command));
        VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        Require(pCreateFence(device, &fence, nullptr, &slot.fence));
    }
    LogTo("Vulkan: overlay swapchain %ux%u, format=%d, images=%u", chain.extent.width,
        chain.extent.height, chain.format, count);
}

VKAPI_ATTR VkResult VKAPI_CALL HookCreate(VkDevice target, const VkSwapchainCreateInfoKHR* info,
    const VkAllocationCallbacks* alloc, VkSwapchainKHR* output) {
    const auto next = originalCreate;
    if (target != device || !info || info->surface != surface || disabled.load())
        return next(target, info, alloc, output);
    std::lock_guard lock(mutex);
    VkSwapchainCreateInfoKHR copy = *info;
    VkSurfaceCapabilitiesKHR capabilities{};
    VkResult queried = pGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &capabilities);
    if (queried != VK_SUCCESS || !(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
        Fail("Vulkan surface cannot host the Nova color attachment");
        return next(target, info, alloc, output);
    }
    copy.imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    constexpr VkImageUsageFlags blurUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    const bool blurSupported = (capabilities.supportedUsageFlags & blurUsage) == blurUsage;
    if (blurSupported) copy.imageUsage |= blurUsage;
    const VkResult result = next(target, &copy, alloc, output);
    if (result == VK_SUCCESS) {
        // Minecraft destroys the old chain before configure(). Handle oldSwapchain too.
        try { DestroyResources(); }
        catch (const std::exception& e) { Fail(e.what()); return result; }
        chain.handle = *output; chain.format = copy.imageFormat; chain.extent = copy.imageExtent;
        chain.blurSupported = blurSupported;
        chain.minImages = copy.minImageCount < 2 ? 2 : copy.minImageCount;
        initialSwapchain = VK_NULL_HANDLE;
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL HookDestroy(VkDevice target, VkSwapchainKHR swapchain,
    const VkAllocationCallbacks* alloc) {
    const auto next = originalDestroy;
    if (target == device && !disabled.load()) {
        std::lock_guard lock(mutex);
        if (chain.handle == swapchain) {
            try { DestroyResources(); chain = {}; }
            catch (const std::exception& e) { Fail(e.what()); }
        }
    }
    next(target, swapchain, alloc);
}

VKAPI_ATTR VkResult VKAPI_CALL HookPresent(VkQueue target, const VkPresentInfoKHR* info) {
    const auto next = originalPresent;
    if (target != queue || !info || info->swapchainCount != 1 || disabled.load()) return next(target, info);
    std::lock_guard lock(mutex);
    const VkSwapchainKHR swapchain = info->pSwapchains[0];
    if (IsCircuitOpen()) {
        try { callbacks.frame(window); }
        catch (const std::exception& e) { Fail(e.what()); }
        return next(target, info);
    }
    if (swapchain != chain.handle) {
        const VkResult result = next(target, info);
        // Existing images only have TRANSFER_DST usage. Ask Minecraft's verified
        // isSuboptimal/configure path to rebuild on its own render thread.
        if (swapchain == initialSwapchain && result == VK_SUCCESS) return VK_SUBOPTIMAL_KHR;
        return result;
    }
    VkPresentInfoKHR present = *info;
    VkSemaphore completion{};
    bool submitted = false;
    const auto savedBlurRefresh = worldBlur.blurRefresh;
    const auto savedLayerRefresh = layerBlur.blurRefresh;
    std::array<VkImageLayout, 4> savedBlurLayouts{}, savedLayerLayouts{};
    for (size_t i = 0; i < savedBlurLayouts.size(); ++i) {
        savedBlurLayouts[i] = worldBlur.blurLevels[i].layout;
        savedLayerLayouts[i] = layerBlur.blurLevels[i].layout;
    }
    try {
        if (chain.images.empty()) CreateResources();
        const uint32_t imageIndex = info->pImageIndices[0];
        if (imageIndex >= chain.images.size()) throw std::runtime_error("Vulkan acquired image is out of range");
        Frame& frame = chain.frames[chain.serial % chain.frames.size()];
        Require(pWaitForFences(device, 1, &frame.fence, VK_TRUE, 5'000'000'000ull));
        Require(pResetCommandPool(device, frame.pool, 0));
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        Require(pBeginCommandBuffer(frame.command, &begin));
        RenderBlur(frame.command, chain.images[imageIndex].image, ui::blur::GetOptions());
        VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        pass.renderPass = chain.pass; pass.framebuffer = chain.images[imageIndex].framebuffer;
        pass.renderArea.extent = chain.extent;
        pCmdBeginRenderPass(frame.command, &pass, VK_SUBPASS_CONTENTS_INLINE);
        recording = frame.command; recordingImageIndex = imageIndex; drew = false;
        callbacks.frame(window);
        recording = VK_NULL_HANDLE;
        pCmdEndRenderPass(frame.command);
#ifdef NOVA_VULKAN_TEST
        if (drew && !IsCircuitOpen()) VulkanTestCapture(frame.command, chain.images[imageIndex].image, chain.extent);
#endif
        Require(pEndCommandBuffer(frame.command));
        if (drew && !IsCircuitOpen()) {
            completion = chain.images[imageIndex].ready;
            std::vector<VkPipelineStageFlags> waits(info->waitSemaphoreCount, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
            VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submit.waitSemaphoreCount = info->waitSemaphoreCount; submit.pWaitSemaphores = info->pWaitSemaphores;
            submit.pWaitDstStageMask = waits.data();
            submit.commandBufferCount = 1; submit.pCommandBuffers = &frame.command;
            submit.signalSemaphoreCount = 1; submit.pSignalSemaphores = &completion;
            Require(pResetFences(device, 1, &frame.fence));
            Require(pQueueSubmit(queue, 1, &submit, frame.fence));
            submitted = true;
            ++chain.serial;
            present.waitSemaphoreCount = 1; present.pWaitSemaphores = &completion;
        }
    } catch (const std::exception& e) { recording = VK_NULL_HANDLE; Fail(e.what()); }
    catch (...) { recording = VK_NULL_HANDLE; Fail("Vulkan overlay callback failed"); }
    if (!submitted) {
        // Recording alone does not change GPU layouts or populate the cache.
        worldBlur.blurRefresh = savedBlurRefresh; layerBlur.blurRefresh = savedLayerRefresh;
        for (size_t i = 0; i < savedBlurLayouts.size(); ++i) {
            worldBlur.blurLevels[i].layout = savedBlurLayouts[i];
            layerBlur.blurLevels[i].layout = savedLayerLayouts[i];
        }
    }
    // Never wait on the game's binary semaphore twice. Only substitute after a
    // successful submit; retain pNext, pResults and the driver's original result.
    const VkResult result = next(target, submitted ? &present : info);
    if (submitted && (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)) callbacks.submitted();
    return result;
}
}

bool Install(const Handles& handles, void* hwnd,Callbacks host) {
    if(!host.frame || !host.rendererLost || !host.submitted)return false;
    callbacks=host;
    disabled.store(false);
    instance = reinterpret_cast<VkInstance>(handles.instance);
    physical = reinterpret_cast<VkPhysicalDevice>(handles.physicalDevice);
    device = reinterpret_cast<VkDevice>(handles.device);
    queue = reinterpret_cast<VkQueue>(handles.queue);
    surface = reinterpret_cast<VkSurfaceKHR>(handles.surface);
    initialSwapchain = reinterpret_cast<VkSwapchainKHR>(handles.swapchain);
    family = handles.queueFamily; window = hwnd;
    if (!instance || !physical || !device || !queue || !surface || !window) return false;
    HMODULE loader = GetModuleHandleW(L"vulkan-1.dll");
    if (!loader) return false;
    gipa = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(loader, "vkGetInstanceProcAddr"));
    gdpa = gipa ? reinterpret_cast<PFN_vkGetDeviceProcAddr>(gipa(instance, "vkGetDeviceProcAddr")) : nullptr;
    if (!gipa || !gdpa) return false;
#define RESOLVE(name) p##name = reinterpret_cast<PFN_vk##name>(Function("vk" #name)); if (!p##name) return false;
    NOVA_VK_FUNCTIONS(RESOLVE)
#undef RESOLVE
    uint32_t count{};
    pGetPhysicalDeviceQueueFamilyProperties(physical, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    pGetPhysicalDeviceQueueFamilyProperties(physical, &count, families.data());
    if (family >= count || !(families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT)) return false;
    VkSurfaceCapabilitiesKHR caps{};
    if (pGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &caps) != VK_SUCCESS ||
        !(caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) return false;
    targets = {reinterpret_cast<void*>(Function("vkQueuePresentKHR")),
        reinterpret_cast<void*>(Function("vkCreateSwapchainKHR")), reinterpret_cast<void*>(Function("vkDestroySwapchainKHR"))};
    const MH_STATUS status = MH_Initialize();
    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) return false;
    void* hooks[]{reinterpret_cast<void*>(&HookPresent), reinterpret_cast<void*>(&HookCreate), reinterpret_cast<void*>(&HookDestroy)};
    void** originals[]{reinterpret_cast<void**>(&originalPresent), reinterpret_cast<void**>(&originalCreate), reinterpret_cast<void**>(&originalDestroy)};
    unsigned created = 0;
    for (; created < targets.size(); ++created)
        if (!targets[created] || MH_CreateHook(targets[created], hooks[created], originals[created]) != MH_OK) break;
    if (created != targets.size()) {
        while (created) MH_RemoveHook(targets[--created]);
        targets = {}; return false;
    }
    // Enable create/destroy before present can request the first rebuild.
    for (int i : {1, 2, 0}) {
        if (MH_EnableHook(targets[i]) != MH_OK) {
            for (void* target : targets) { MH_DisableHook(target); MH_RemoveHook(target); }
            targets = {}; return false;
        }
    }
    LogTo("Vulkan: device present/create/destroy hooks installed; awaiting swapchain rebuild");
    return true;
}

bool InitializeImGui() {
    if (imguiReady) return true;
    if (!chain.pass || chain.images.empty()) return false;
    if (!ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_2,
        [](const char* name, void*) { return Function(name); })) return false;
    ImGui_ImplVulkan_InitInfo info{};
    info.ApiVersion = VK_API_VERSION_1_2;
    info.Instance = instance; info.PhysicalDevice = physical; info.Device = device;
    info.QueueFamily = family; info.Queue = queue;
    info.DescriptorPoolSize = 64;
    info.MinImageCount = chain.minImages;
    info.ImageCount = static_cast<uint32_t>(chain.images.size());
    info.PipelineInfoMain.RenderPass = chain.pass;
    info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    info.CheckVkResultFn = &Check;
    imguiReady = ImGui_ImplVulkan_Init(&info);
    return imguiReady;
}
void NewFrame() { ImGui_ImplVulkan_NewFrame(); }
void Render(ImDrawData* data) {
    if (!imguiReady || !recording || !data) return;
    ImGui_ImplVulkan_RenderDrawData(data, recording);
    drew = true;
}
void ShutdownImGui() { std::lock_guard lock(mutex); DestroyResources(); }
void DisableHooks() {
    disabled.store(true);
    for (void* target : targets) if (target) MH_DisableHook(target);
}
void Finalize() {
    for (void* target : targets) if (target) MH_RemoveHook(target);
    targets = {};
    chain = {};
}
}
