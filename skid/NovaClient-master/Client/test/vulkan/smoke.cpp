// Standalone Vulkan window/driver test. Does not attach to Minecraft or another process.
#define NOMINMAX
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#include <vulkan/vulkan.h>
#include "ui/backend/VulkanOverlay.h"
#include "ui/backend/blur/Blur.h"
#include "ui/skin/SkinTextures.h"
#include "ui/hud/musichud/CoverTexture.h"
#include "imgui_internal.h"
#include "core/RuntimeControl.h"
#include "imgui.h"
#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <vector>

static VkInstance inst{};
static VkPhysicalDevice phys{};
static VkDevice dev{};
static VkQueue graphics{};
static VkSurfaceKHR surface{};
static VkSwapchainKHR swap{};
static VkFormat format{};
static VkExtent2D extent{};
static uint32_t family{};
static HWND window{};
static PFN_vkGetInstanceProcAddr getInstance{};
static PFN_vkGetDeviceProcAddr getDevice{};
static bool circuit{}, failed{}, teardown{}, capture{true};
static int rendered{};
static VkBuffer readback{}, upload{};
static VkDeviceMemory uploadMemory{};
static bool pattern{}, blurTest{}, suppressDraw{};
static bool layerTest{}, nestedLayer{}, reverseLayer{};
static nova::ui::blur::Options blurOptions{};
namespace nova::ui::blur { Options GetOptions() { return blurOptions; } }
static VkDeviceMemory readMemory{};
static VkCommandPool pool{};
static VkCommandBuffer command{};
static VkFence fence{};
static VkSemaphore acquire{};
static std::vector<VkImage> images;
static std::vector<VkSemaphore> complete;
static PFN_vkVoidFunction Fn(const char* name) {
    auto fn = getDevice && dev ? getDevice(dev, name) : nullptr;
    if (!fn) fn = getInstance(inst, name);
    if (!fn) throw std::runtime_error(name);
    return fn;
}
#define V(name) reinterpret_cast<PFN_vk##name>(Fn("vk" #name))
static void Check(VkResult result) { if (result != VK_SUCCESS) throw std::runtime_error("VkResult " + std::to_string(result)); }
static void Assert(bool pass, const char* message) { if (!pass) throw std::runtime_error(message); }
void LogTo(const char* fmt, ...) { va_list args; va_start(args, fmt); vprintf(fmt, args); va_end(args); puts(""); }
bool IsCircuitOpen() { return circuit; }
void OpenCircuit() { circuit = true; }
void RequestUnload() {}
void ReportBootstrapFailed(const char* message) { failed = true; LogTo("FAIL: %s", message); }
namespace nova::glhook {
void VulkanRendererLost() {}
void VulkanFrameSubmitted() { ++rendered; }
void VulkanFrame(void*) {
    if (circuit) {
        vkoverlay::ShutdownImGui();
        ui::skins::Release();
        ui::music::ReleaseCovers();
        if (ImGui::GetCurrentContext()) ImGui::DestroyContext();
        vkoverlay::DisableHooks(); teardown = true; return;
    }
    if (!ImGui::GetCurrentContext()) {
        ImGui::CreateContext();
        ImGui::GetIO().IniFilename = nullptr;
    }
    Assert(vkoverlay::InitializeImGui(), "initialize ImGui Vulkan");
    ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(extent.width), static_cast<float>(extent.height));
    ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
    vkoverlay::NewFrame();
    ImGui::NewFrame();
    if (blurTest) ui::blur::Background(ImGui::GetBackgroundDrawList(), ImVec2(144, 88),
        ImVec2(240, 168), IM_COL32(0, 0, 0, 255), 16);
    const auto skin = ui::skins::Store("test-skin", 16, 16, std::vector<std::uint32_t>(256, IM_COL32(0, 255, 0, 255)));
    Assert(skin != 0, "skin registration");
    ImGui::GetBackgroundDrawList()->AddImage(ui::skins::Resolve(skin), ImVec2(60, 8), ImVec2(92, 40));
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(8, 8), ImVec2(48, 48), IM_COL32(255, 0, 0, 255));
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(8, 60), IM_COL32_WHITE, "Nova Vulkan");
    auto cover = std::make_shared<smtc::Artwork>();
    cover->width = cover->height = 16; cover->version = rendered + 1;
    cover->rgba.assign(256, IM_COL32(255, 255, 0, 255));
    ui::music::RetainCovers(cover->version, 0);
    Assert(ui::music::DrawCover(ImGui::GetBackgroundDrawList(), cover, ImVec2(100, 8), ImVec2(132, 40), 4), "cover replacement accepted");
    Assert(ImGui::GetCurrentContext()->UserTextures.Size <= 9, "bounded cover retirement with existing skin");
    if(layerTest) {
        auto* draws=ImGui::GetForegroundDrawList();
        draws->AddRectFilled({8,96},{64,184},reverseLayer?IM_COL32(0,0,255,255):IM_COL32(255,0,0,255));
        draws->AddRectFilled({64,96},{120,184},reverseLayer?IM_COL32(255,0,0,255):IM_COL32(0,0,255,255));
        ui::blur::LayerBackground(draws,{16,104},{112,176},0u);
        if(nestedLayer) {
            draws->AddRectFilled({32,112},{64,168},IM_COL32(0,255,0,255));
            ui::blur::LayerBackground(draws,{48,120},{96,160},0u);
        }
        ui::blur::Background(draws,{0,184},{8,192},0u);
    }
    ImGui::Render();
    if (!suppressDraw) vkoverlay::Render(ImGui::GetDrawData());
}
}

void VulkanTestCapture(VkCommandBuffer cmd, VkImage image, VkExtent2D size) {
    if (!capture) return;
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image; barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    V(CmdPipelineBarrier)(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {size.width, size.height, 1};
    V(CmdCopyImageToBuffer)(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback, 1, &region);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT; barrier.dstAccessMask = 0;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    V(CmdPipelineBarrier)(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
}

static void MakeSwap() {
    Check(V(QueueWaitIdle)(graphics));
    if (swap) V(DestroySwapchainKHR)(dev, swap, nullptr);
    for (auto sem : complete) V(DestroySemaphore)(dev, sem, nullptr);
    complete.clear(); images.clear();
    VkSurfaceCapabilitiesKHR caps{};
    Check(V(GetPhysicalDeviceSurfaceCapabilitiesKHR)(phys, surface, &caps));
    extent = caps.currentExtent;
    VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    info.surface = surface;
    info.minImageCount = (caps.minImageCount > 3 ? caps.minImageCount : 3);
    if (caps.maxImageCount && info.minImageCount > caps.maxImageCount) info.minImageCount = caps.maxImageCount;
    info.imageFormat = format; info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    info.imageExtent = extent; info.imageArrayLayers = 1;
    // Match Minecraft's original usage, adding SRC solely for test pixel readback.
    info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = caps.currentTransform; info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR; info.clipped = VK_TRUE;
    Check(V(CreateSwapchainKHR)(dev, &info, nullptr, &swap));
    uint32_t count{};
    Check(V(GetSwapchainImagesKHR)(dev, swap, &count, nullptr));
    images.resize(count); complete.resize(count);
    Check(V(GetSwapchainImagesKHR)(dev, swap, &count, images.data()));
    VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    for (auto& sem : complete) Check(V(CreateSemaphore)(dev, &semaphore, nullptr, &sem));
}

static void Draw() {
    Check(V(WaitForFences)(dev, 1, &fence, VK_TRUE, 5'000'000'000ull));
    uint32_t index{};
    VkResult got = V(AcquireNextImageKHR)(dev, swap, 5'000'000'000ull, acquire, VK_NULL_HANDLE, &index);
    if (got == VK_ERROR_OUT_OF_DATE_KHR) { MakeSwap(); return; }
    Assert(got == VK_SUCCESS || got == VK_SUBOPTIMAL_KHR, "acquire");
    Check(V(ResetCommandPool)(dev, pool, 0));
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    Check(V(BeginCommandBuffer)(command, &begin));
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = images[index]; barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    V(CmdPipelineBarrier)(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
    VkClearColorValue color{{0.f, 0.2f, 0.4f, 1.f}};
    V(CmdClearColorImage)(command, images[index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &barrier.subresourceRange);
    if (pattern) {
        barrier.srcAccessMask = barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        V(CmdPipelineBarrier)(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
        VkBufferImageCopy region{};
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageOffset = {128, 64, 0}; region.imageExtent = {128, 128, 1};
        V(CmdCopyBufferToImage)(command, upload, images[index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; barrier.dstAccessMask = 0;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    V(CmdPipelineBarrier)(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
    Check(V(EndCommandBuffer)(command));
    VkPipelineStageFlags stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1; submit.pWaitSemaphores = &acquire; submit.pWaitDstStageMask = &stage;
    submit.commandBufferCount = 1; submit.pCommandBuffers = &command;
    submit.signalSemaphoreCount = 1; submit.pSignalSemaphores = &complete[index];
    Check(V(ResetFences)(dev, 1, &fence));
    Check(V(QueueSubmit)(graphics, 1, &submit, fence));
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1; present.pWaitSemaphores = &complete[index];
    present.swapchainCount = 1; present.pSwapchains = &swap; present.pImageIndices = &index;
    const auto result = V(QueuePresentKHR)(graphics, &present);
    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) MakeSwap();
    else Check(result);
    if (capture) Check(V(QueueWaitIdle)(graphics));
    Assert(!failed, "overlay failure");
}

static void Pixels() {
    void* bytes{};
    Check(V(MapMemory)(dev, readMemory, 0, VK_WHOLE_SIZE, 0, &bytes));
    auto at = [&](int x, int y) { return static_cast<unsigned char*>(bytes) + (y * extent.width + x) * 4; };
    const int red = format == VK_FORMAT_B8G8R8A8_UNORM ? 2 : 0;
    Assert(at(20, 20)[red] > 245 && at(20, 20)[1] < 5, "GPU readback: red Nova overlay missing");
    Assert(at(70, 20)[red] < 5 && at(70, 20)[1] > 245, "GPU readback: registered skin texture missing");
    Assert(at(110, 20)[red] > 245 && at(110, 20)[1] > 245, "GPU readback: SMTC cover missing");
    Assert(at(100, 100)[red] < 5 && at(100, 100)[1] >= 50 && at(100, 100)[1] <= 52,
        "GPU readback: game's background was not preserved");
    V(UnmapMemory)(dev, readMemory);
}

static int BlurPixel(int x, int y) {
    void* bytes{}; Check(V(MapMemory)(dev, readMemory, 0, VK_WHOLE_SIZE, 0, &bytes));
    const int red = format == VK_FORMAT_B8G8R8A8_UNORM ? 2 : 0;
    const int value = static_cast<unsigned char*>(bytes)[(y * extent.width + x) * 4 + red];
    V(UnmapMemory)(dev, readMemory); return value;
}
static void BlurPixels() {
    Pixels();
    Assert(BlurPixel(191, 128) > 5 && BlurPixel(191, 128) < 45, "local HUD blurred edge");
    Assert(BlurPixel(191, 70) == 0 && BlurPixel(192, 70) == 255, "outside HUD unchanged");
    Assert(BlurPixel(144, 88) == 0, "rounded HUD corner");
}
static void LayerPixels(bool nested=false,bool reversed=false) {
    void* bytes{};Check(V(MapMemory)(dev,readMemory,0,VK_WHOLE_SIZE,0,&bytes));
    const int red=format==VK_FORMAT_B8G8R8A8_UNORM?2:0,blue=2-red;
    const auto* p=static_cast<unsigned char*>(bytes)+(140*extent.width+60)*4;
    if(nested)Assert(p[1]>100 && p[blue]>20,"nested blur includes parent UI");
    else Assert(p[reversed?blue:red]>100 && p[reversed?red:blue]>30 && p[1]<5,"live blur includes lower UI");
    const auto* world=static_cast<unsigned char*>(bytes)+(188*extent.width+4)*4;
    Assert(world[red]<5 && world[1]>=50 && world[1]<=52,"live capture polluted world cache");
    V(UnmapMemory)(dev,readMemory);
}

int main() {
    try {
        HMODULE library = LoadLibraryW(L"vulkan-1.dll");
        Assert(library != nullptr, "Vulkan loader missing");
        getInstance = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(library, "vkGetInstanceProcAddr"));
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO}; app.apiVersion = VK_API_VERSION_1_2;
        const char* extensions[]{VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
        VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        create.pApplicationInfo = &app; create.enabledExtensionCount = 2; create.ppEnabledExtensionNames = extensions;
        Check(V(CreateInstance)(&create, nullptr, &inst));
        getDevice = V(GetDeviceProcAddr);
        WNDCLASSW wc{}; wc.lpfnWndProc = DefWindowProcW; wc.hInstance = GetModuleHandleW(nullptr); wc.lpszClassName = L"NovaVulkanSmoke";
        RegisterClassW(&wc);
        window = CreateWindowW(wc.lpszClassName, L"Nova Vulkan test", WS_POPUP, 0, 0, 256, 192, nullptr, nullptr, wc.hInstance, nullptr);
        Assert(window != nullptr, "test window");
        VkWin32SurfaceCreateInfoKHR win{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR}; win.hinstance = wc.hInstance; win.hwnd = window;
        Check(V(CreateWin32SurfaceKHR)(inst, &win, nullptr, &surface));
        uint32_t count{}; Check(V(EnumeratePhysicalDevices)(inst, &count, nullptr));
        std::vector<VkPhysicalDevice> devices(count); Check(V(EnumeratePhysicalDevices)(inst, &count, devices.data()));
        for (auto candidate : devices) {
            uint32_t families{}; V(GetPhysicalDeviceQueueFamilyProperties)(candidate, &families, nullptr);
            std::vector<VkQueueFamilyProperties> props(families); V(GetPhysicalDeviceQueueFamilyProperties)(candidate, &families, props.data());
            for (uint32_t i = 0; i < families; ++i) {
                VkBool32 present{}; Check(V(GetPhysicalDeviceSurfaceSupportKHR)(candidate, i, surface, &present));
                if ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) { phys = candidate; family = i; break; }
            }
            if (phys) break;
        }
        Assert(phys != nullptr, "graphics/present device");
        VkPhysicalDeviceProperties props{}; V(GetPhysicalDeviceProperties)(phys, &props); LogTo("GPU: %s", props.deviceName);
        float priority = 1;
        VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO}; q.queueFamilyIndex = family; q.queueCount = 1; q.pQueuePriorities = &priority;
        const char* extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        VkDeviceCreateInfo dc{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO}; dc.queueCreateInfoCount = 1; dc.pQueueCreateInfos = &q;
        dc.enabledExtensionCount = 1; dc.ppEnabledExtensionNames = &extension;
        Check(V(CreateDevice)(phys, &dc, nullptr, &dev)); V(GetDeviceQueue)(dev, family, 0, &graphics);
        Check(V(GetPhysicalDeviceSurfaceFormatsKHR)(phys, surface, &count, nullptr));
        std::vector<VkSurfaceFormatKHR> formats(count); Check(V(GetPhysicalDeviceSurfaceFormatsKHR)(phys, surface, &count, formats.data()));
        for (auto item : formats) if ((item.format == VK_FORMAT_B8G8R8A8_UNORM || item.format == VK_FORMAT_R8G8B8A8_UNORM) && item.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) { format = item.format; break; }
        Assert(format != VK_FORMAT_UNDEFINED, "UNORM swapchain format");
        MakeSwap();
        VkBufferCreateInfo buffer{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO}; buffer.size = 512 * 512 * 4; buffer.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        Check(V(CreateBuffer)(dev, &buffer, nullptr, &readback));
        VkMemoryRequirements req{}; V(GetBufferMemoryRequirements)(dev, readback, &req);
        VkPhysicalDeviceMemoryProperties memory{}; V(GetPhysicalDeviceMemoryProperties)(phys, &memory);
        uint32_t type = UINT32_MAX;
        for (uint32_t i = 0; i < memory.memoryTypeCount; ++i) if ((req.memoryTypeBits & (1u << i)) &&
            (memory.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) { type = i; break; }
        Assert(type != UINT32_MAX, "host coherent readback");
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO}; allocation.allocationSize = req.size; allocation.memoryTypeIndex = type;
        Check(V(AllocateMemory)(dev, &allocation, nullptr, &readMemory)); Check(V(BindBufferMemory)(dev, readback, readMemory, 0));
        buffer.size = 128 * 128 * 4; buffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        Check(V(CreateBuffer)(dev, &buffer, nullptr, &upload));
        V(GetBufferMemoryRequirements)(dev, upload, &req);
        Assert((req.memoryTypeBits & (1u << type)) != 0, "upload memory type");
        allocation.allocationSize = req.size;
        Check(V(AllocateMemory)(dev, &allocation, nullptr, &uploadMemory));
        Check(V(BindBufferMemory)(dev, upload, uploadMemory, 0));
        void* uploadBytes{}; Check(V(MapMemory)(dev, uploadMemory, 0, VK_WHOLE_SIZE, 0, &uploadBytes));
        auto* pixels = static_cast<unsigned char*>(uploadBytes);
        for (int y = 0; y < 128; ++y) for (int x = 0; x < 128; ++x) {
            const int offset = (y * 128 + x) * 4;
            pixels[offset] = pixels[offset + 1] = pixels[offset + 2] = x < 64 ? 0 : 255;
            pixels[offset + 3] = 255;
        }
        V(UnmapMemory)(dev, uploadMemory);
        VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO}; cp.queueFamilyIndex = family;
        Check(V(CreateCommandPool)(dev, &cp, nullptr, &pool));
        VkCommandBufferAllocateInfo cb{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO}; cb.commandPool = pool; cb.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; cb.commandBufferCount = 1;
        Check(V(AllocateCommandBuffers)(dev, &cb, &command));
        VkFenceCreateInfo fc{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO}; fc.flags = VK_FENCE_CREATE_SIGNALED_BIT; Check(V(CreateFence)(dev, &fc, nullptr, &fence));
        VkSemaphoreCreateInfo sc{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO}; Check(V(CreateSemaphore)(dev, &sc, nullptr, &acquire));
        auto install = [&] {
            nova::vkoverlay::Handles h{reinterpret_cast<uintptr_t>(inst), reinterpret_cast<uintptr_t>(phys), reinterpret_cast<uintptr_t>(dev),
                reinterpret_cast<uintptr_t>(graphics), reinterpret_cast<uint64_t>(surface), reinterpret_cast<uint64_t>(swap), family};
            Assert(nova::vkoverlay::Install(h, window,{nova::glhook::VulkanFrame,nova::glhook::VulkanRendererLost,nova::glhook::VulkanFrameSubmitted}), "install Vulkan hooks");
        };
        install();
        for (int i = 0; i < 5; ++i) Draw();
        Assert(rendered > 0, "first-frame acknowledgement"); Pixels();
        pattern = blurTest = true; blurOptions = {true, 3, 5};
        for (int i = 0; i < 3; ++i) Draw();
        BlurPixels();
        blurOptions.strength = 1; Draw(); const int weak = BlurPixel(182, 128);
        blurOptions.strength = 5; Draw();
        Assert(BlurPixel(182, 128) > weak + 5, "adjustable blur strength");
        blurOptions.enabled = false; Draw();
        Assert(BlurPixel(200, 128) == 0, "disabled panel has original tint");
        blurOptions = {true, 3, 30}; Draw(); BlurPixels();
        suppressDraw = true; blurOptions.strength = 4; Draw();
        suppressDraw = false; Draw(); BlurPixels();
        layerTest=true;blurOptions={true,3,5};Draw();LayerPixels();
        reverseLayer=true;Draw();LayerPixels(false,true);
        reverseLayer=false;nestedLayer=true;Draw();LayerPixels(true);
        suppressDraw=true;Draw();suppressDraw=false;Draw();LayerPixels(true);
        layerTest=nestedLayer=false;
        SetWindowPos(window, nullptr, 0, 0, 320, 240, SWP_NOACTIVATE | SWP_NOZORDER);
        MakeSwap();
        for (int i = 0; i < 5; ++i) Draw();
        BlurPixels();
        layerTest=true;Draw();LayerPixels();layerTest=false;
        capture = false;
        for (int i = 0; i < 120; ++i) Draw();
        circuit = true; Draw(); Assert(teardown, "teardown on presenting thread");
        nova::vkoverlay::Finalize();
        circuit = teardown = false; capture = true; install();
        for (int i = 0; i < 4; ++i) Draw();
        BlurPixels();
        circuit = true; Draw(); Assert(teardown, "second teardown"); nova::vkoverlay::Finalize();
        Check(V(DeviceWaitIdle)(dev));
        V(DestroySwapchainKHR)(dev, swap, nullptr);
        for (auto sem : complete) V(DestroySemaphore)(dev, sem, nullptr);
        V(DestroySemaphore)(dev, acquire, nullptr); V(DestroyFence)(dev, fence, nullptr);
        V(DestroyCommandPool)(dev, pool, nullptr); V(DestroyBuffer)(dev, readback, nullptr); V(FreeMemory)(dev, readMemory, nullptr);
        V(DestroyBuffer)(dev, upload, nullptr); V(FreeMemory)(dev, uploadMemory, nullptr);
        V(DestroyDevice)(dev, nullptr); dev = VK_NULL_HANDLE;
        V(DestroySurfaceKHR)(inst, surface, nullptr); V(DestroyInstance)(inst, nullptr);
        DestroyWindow(window);
        puts("PASS: real GPU local and nested UI blur, independent layer refresh/world cache, state reset, strength, textures, discard, resize, 120 frames, teardown and reattach");
        return 0;
    } catch (const std::exception& e) { fprintf(stderr, "FAIL: %s\n", e.what()); return 1; }
}
