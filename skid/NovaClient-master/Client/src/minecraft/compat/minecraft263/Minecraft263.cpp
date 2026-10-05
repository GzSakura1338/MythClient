#include "minecraft/compat/minecraft263/Minecraft263.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/easecation/EaseCation.h"
#include "minecraft/compat/neoforge12110/NeoForge12110.h"
#include "minecraft/compat/lunar12111/Lunar12111.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace nova::compat::mc263 {
namespace {
using namespace names;
std::vector<std::pair<std::string, jclass>> classes;
std::string error;
jfieldID gui{}, mouse{}, options{}, sprint{}, value{};
jmethodID screen{}, grabbed{}, grab{}, release{}, setDown{}, gamma{}, box{}, unbox{};
jclass doubleClass{};
bool savedMouse{}, savedGrabbed{};
void* windowHandle{};
bool vulkan{};
vkoverlay::Handles vkHandles{};

bool Clean(JNIEnv* env) {
    if (!env->ExceptionCheck()) return true;
    env->ExceptionClear();
    return false;
}

// Keep transient references bounded on the game's already-attached render thread.
struct Frame {
    JNIEnv* env;
    bool valid;
    explicit Frame(JNIEnv* e) : env(e), valid(e && e->PushLocalFrame(32) == JNI_OK) {}
    ~Frame() { if (valid) { Clean(env); env->PopLocalFrame(nullptr); } }
};

jclass Class(JNIEnv* env, const char* name) {
    for (const auto& entry : classes) if (entry.first == name) return entry.second;
    jobject loader = compat::ClassLoader(env);
    jclass local = loader ? LoadClassInLoader(env, loader, name) : nullptr;
    if (loader) env->DeleteGlobalRef(loader);
    const bool clean = Clean(env);
    if (!clean || !local) { error = std::string("26.3 class unavailable: ") + name; return nullptr; }
    auto global = static_cast<jclass>(env->NewGlobalRef(local));
    env->DeleteLocalRef(local);
    if (global) classes.emplace_back(name, global);
    return global;
}

jmethodID Method(JNIEnv* env, names::Member m, bool isStatic = false) {
    jclass type = Class(env, m.owner);
    if (!type) return nullptr;
    auto id = isStatic ? env->GetStaticMethodID(type, m.name, m.descriptor)
                       : env->GetMethodID(type, m.name, m.descriptor);
    if (!Clean(env) || !id) { error = std::string("26.3 method unavailable: ") + m.owner + "." + m.name; return nullptr; }
    return id;
}
jfieldID Field(JNIEnv* env, names::Member m, bool isStatic = false) {
    jclass type = Class(env, m.owner);
    if (!type) return nullptr;
    auto id = isStatic ? env->GetStaticFieldID(type, m.name, m.descriptor)
                       : env->GetFieldID(type, m.name, m.descriptor);
    if (!Clean(env) || !id) { error = std::string("26.3 field unavailable: ") + m.owner + "." + m.name; return nullptr; }
    return id;
}
std::string Text(JNIEnv* env, jstring string) {
    if (!Clean(env) || !string) return {};
    const char* bytes = env->GetStringUTFChars(string, nullptr);
    if (!bytes) { Clean(env); return {}; }
    std::string result(bytes);
    env->ReleaseStringUTFChars(string, bytes);
    return result;
}
jobject ClientField(JNIEnv* env, jfieldID field) {
    jobject mc = compat::Minecraft(env);
    return mc && field ? env->GetObjectField(mc, field) : nullptr;
}
jobject Gamma(JNIEnv* env) {
    jobject opts = ClientField(env, options);
    if (!Clean(env) || !opts) return nullptr;
    return env->CallObjectMethod(opts, gamma);
}
}

const char* Error() { return error.empty() ? "Minecraft 26.3 interfaces are unavailable." : error.c_str(); }
void* WindowHandle() { return windowHandle; }
bool IsVulkan() { return vulkan; }
bool VulkanHandles(vkoverlay::Handles* out) {
    if (!out || !vulkan || !vkHandles.device) return false;
    *out = vkHandles;
    return true;
}

static bool ResolveVulkan(JNIEnv* env) {
    auto objectField = [&](jobject obj, names::Member mapping) -> jobject {
        auto f = Field(env, mapping);
        if (!obj || !f) return nullptr;
        jobject result = env->GetObjectField(obj, f);
        return Clean(env) ? result : nullptr;
    };
    auto call = [&](jobject obj, names::Member mapping) -> jobject {
        auto method = Method(env, mapping);
        if (!obj || !method) return nullptr;
        jobject result = env->CallObjectMethod(obj, method);
        return Clean(env) ? result : nullptr;
    };
    auto address = Method(env, LwjglPointer_address);
    if (!address) return false;
    auto pointer = [&](jobject obj) -> std::uintptr_t {
        if (!obj) return 0;
        const auto result = env->CallLongMethod(obj, address);
        return Clean(env) ? static_cast<std::uintptr_t>(result) : 0;
    };
    jobject front = objectField(compat::Minecraft(env), Minecraft_Field_windowSurface);
    jobject surface = objectField(front, FrontendGpuSurface_backend);
    jobject device = objectField(surface, VulkanGpuSurface_device);
    jobject logical = call(device, VulkanDevice_vkDevice);
    jobject physical = call(logical, LwjglVkDevice_getPhysicalDevice);
    jobject instance = call(physical, LwjglVkPhysicalDevice_getInstance);
    jobject queue = call(device, VulkanDevice_graphicsQueue);
    vkHandles.device = pointer(logical);
    vkHandles.physicalDevice = pointer(physical);
    vkHandles.instance = pointer(instance);
    vkHandles.queue = pointer(call(queue, VulkanQueue_vkQueue));
    auto family = Method(env, VulkanQueue_queueFamilyIndex);
    auto surfaceId = Field(env, VulkanGpuSurface_surface);
    auto swapchain = Field(env, VulkanGpuSurface_swapchain);
    if (!queue || !surface || !family || !surfaceId || !swapchain) return false;
    vkHandles.queueFamily = static_cast<std::uint32_t>(env->CallIntMethod(queue, family));
    if (!Clean(env)) return false;
    vkHandles.surface = static_cast<std::uint64_t>(env->GetLongField(surface, surfaceId));
    vkHandles.swapchain = static_cast<std::uint64_t>(env->GetLongField(surface, swapchain));
    return Clean(env) && vkHandles.device && vkHandles.physicalDevice && vkHandles.instance &&
        vkHandles.queue && vkHandles.surface;
}

bool Initialize(JNIEnv* env) {
    error.clear();
    Frame frame(env);
    if (!frame.valid) return false;
    auto currentVersion = Method(env, SharedConstants_getCurrentVersion, true);
    auto versionId = Method(env, WorldVersion_id);
    if (!currentVersion || !versionId) return false;
    jobject version = env->CallStaticObjectMethod(Class(env, names::SharedConstants), currentVersion);
    if (!Clean(env) || !version) return false;
    const auto id = Text(env, static_cast<jstring>(env->CallObjectMethod(version, versionId)));
    if (id != "26.3") { error = "Expected Minecraft 26.3, found: " + id; return false; }

    auto backendField = Field(env, RenderSystem_Field_BACKEND, true);
    auto backendName = Method(env, GpuBackend_getName);
    if (!backendField || !backendName) return false;
    jobject backend = env->GetStaticObjectField(Class(env, names::RenderSystem), backendField);
    if (!Clean(env) || !backend) { error = "26.3 renderer is not initialized. Wait for the main menu."; return false; }
    const auto renderer = Text(env, static_cast<jstring>(env->CallObjectMethod(backend, backendName)));
    vulkan = renderer == "Vulkan";
    if (renderer != "OpenGL" && !vulkan) {
        error = "Unsupported 26.3 renderer: " + renderer;
        return false;
    }
    if (vulkan && !ResolveVulkan(env)) {
        if (error.empty()) error = "26.3 Vulkan device or surface is not ready.";
        return false;
    }
    auto getWindow = Method(env, Minecraft_getWindow);
    auto handle = Method(env, Window_handle);
    jobject mc = compat::Minecraft(env);
    if (!getWindow || !handle || !mc) return false;
    jobject window = env->CallObjectMethod(mc, getWindow);
    if (!Clean(env) || !window) return false;
    windowHandle = reinterpret_cast<void*>(env->CallLongMethod(window, handle));
    if (!Clean(env) || !windowHandle) { error = "26.3 SDL window is not ready."; return false; }

    // Each lookup clears its own exception, so failures never poison later JNI calls.
    gui = Field(env, Minecraft_Field_gui);
    screen = Method(env, Gui_screen);
    mouse = Field(env, Minecraft_Field_mouseHandler);
    grabbed = Method(env, MouseHandler_isMouseGrabbed);
    grab = Method(env, MouseHandler_grabMouse);
    release = Method(env, MouseHandler_releaseMouse);
    options = Field(env, Minecraft_Field_options);
    sprint = Field(env, Options_Field_keySprint);
    setDown = Method(env, KeyMapping_setDown);
    gamma = Method(env, Options_gamma);
    value = Field(env, OptionInstance_Field_value);
    doubleClass = Class(env, "java.lang.Double");
    box = Method(env, {"java.lang.Double", "valueOf", "(D)Ljava/lang/Double;"}, true);
    unbox = Method(env, {"java.lang.Double", "doubleValue", "()D"});
    if (!gui || !screen || !mouse || !grabbed || !grab || !release || !options || !sprint ||
        !setDown || !gamma || !value || !doubleClass || !box || !unbox) return false;
    LogTo("26.3: version verified; SDL/%s, GUI, mouse and options resolved", renderer.c_str());
    return true;
}

jobject Screen(JNIEnv* env, jobject mc) {
    if (IsLunar12111()) return lunar12111::Screen(env,mc);
    if (IsEaseCation()) return easecation::Screen(env,mc);
    if (IsNeoForge12110()) return neoforge12110::Screen(env,mc);
    if (!gui || !screen || !mc) return nullptr;
    jobject ui = env->GetObjectField(mc, gui);
    if (env->ExceptionCheck() || !ui) return nullptr;
    jobject result = env->CallObjectMethod(ui, screen);
    env->DeleteLocalRef(ui);
    return result;
}

bool CaptureMouse(bool capture) {
    if (UsesGlfwModernRuntime()) {
        access::Frame f;
        auto handler=f.get(f.client(),names::Minecraft_Field_mouseHandler);
        if (!handler) return false;
        if (capture && !savedMouse) {
            savedGrabbed=f.call<bool>(handler,names::MouseHandler_isMouseGrabbed);
            if (!f.check()) return false;
            savedMouse=true;
        }
        if (!capture && !savedMouse) return true;
        const bool result=f.invoke(handler,!capture&&savedGrabbed&&!compat::ScreenOpen() ? names::MouseHandler_grabMouse : names::MouseHandler_releaseMouse);
        if (result && !capture) savedMouse=false;
        return result;
    }
    JniAttach attach;
    Frame frame(attach.env);
    if (!frame.valid) return false;
    JNIEnv* env = attach.env;
    jobject handler = ClientField(env, mouse);
    if (!Clean(env) || !handler) return false;
    if (capture && !savedMouse) {
        savedGrabbed = env->CallBooleanMethod(handler, grabbed) == JNI_TRUE;
        if (!Clean(env)) return false;
        savedMouse = true;
    }
    if (!capture && !savedMouse) return true;
    // Do not grab the game if its own screen opened while ClickGUI was active.
    env->CallVoidMethod(handler, !capture && savedGrabbed && !compat::ScreenOpen() ? grab : release);
    if (!Clean(env)) return false;
    if (!capture) savedMouse = false;
    return true;
}

bool SetSprintKeyDown(bool down) {
    if (UsesGlfwModernRuntime()) {
        access::Frame f; auto opts=f.get(f.client(),names::Minecraft_Field_options);
        return f.invoke(f.get(opts,names::Options_Field_keySprint),names::KeyMapping_setDown,down&&!compat::ScreenOpen()?JNI_TRUE:JNI_FALSE);
    }
    JniAttach attach;
    Frame frame(attach.env);
    if (!frame.valid) return false;
    JNIEnv* env = attach.env;
    jobject opts = ClientField(env, options);
    if (!Clean(env) || !opts) return false;
    jobject key = env->GetObjectField(opts, sprint);
    if (!Clean(env) || !key) return false;
    env->CallVoidMethod(key, setDown, down && !compat::ScreenOpen() ? JNI_TRUE : JNI_FALSE);
    return Clean(env);
}
bool GetGamma(double* out) {
    if (!out) return false;
    if (UsesGlfwModernRuntime()) {
        access::Frame f; auto opts=f.get(f.client(),names::Minecraft_Field_options);
        auto number=f.get(f.call(opts,names::Options_gamma),names::OptionInstance_Field_value);
        *out=f.call<double>(number,{"java.lang.Double","doubleValue","()D"});return number&&f.check();
    }
    JniAttach attach;
    Frame frame(attach.env);
    if (!frame.valid) return false;
    JNIEnv* env = attach.env;
    jobject option = Gamma(env);
    if (!Clean(env) || !option) return false;
    jobject number = env->GetObjectField(option, value);
    if (!Clean(env) || !number || !env->IsInstanceOf(number, doubleClass)) return false;
    *out = env->CallDoubleMethod(number, unbox);
    return Clean(env);
}
bool SetGamma(double target) {
    if (UsesGlfwModernRuntime()) {
        access::Frame f;auto opts=f.get(f.client(),names::Minecraft_Field_options);
        auto number=f.staticCall({"java.lang.Double","valueOf","(D)Ljava/lang/Double;"},target);
        return number&&f.set(f.call(opts,names::Options_gamma),names::OptionInstance_Field_value,number);
    }
    JniAttach attach;
    Frame frame(attach.env);
    if (!frame.valid) return false;
    JNIEnv* env = attach.env;
    jobject option = Gamma(env);
    if (!Clean(env) || !option) return false;
    jobject number = env->CallStaticObjectMethod(doubleClass, box, static_cast<jdouble>(target));
    if (!Clean(env) || !number) return false;
    env->SetObjectField(option, value, number);
    return Clean(env);
}

void Shutdown(JNIEnv* env) {
    for (const auto& entry : classes) env->DeleteGlobalRef(entry.second);
    classes.clear();
    gui = mouse = options = sprint = value = nullptr;
    screen = grabbed = grab = release = setDown = gamma = box = unbox = nullptr;
    doubleClass = nullptr;
    savedMouse = savedGrabbed = false;
    windowHandle = nullptr;
    vulkan = false;
    vkHandles = {};
}
}
