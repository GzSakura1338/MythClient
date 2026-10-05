#include "minecraft/compat/Profile.h"
#include "minecraft/java/api/game_bridgeassist.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263.h"
#include "minecraft/compat/easecation/EaseCation.h"
#include "minecraft/compat/neoforge12110/NeoForge12110.h"
#include "minecraft/compat/lunar12111/Lunar12111.h"
#define NOVA_EASECATION_PROFILE
#include "minecraft/compat/mappings/easecation.inc"
#undef NOVA_EASECATION_PROFILE
#define NOVA_NEOFORGE12110_PROFILE
#include "minecraft/compat/mappings/neoforge12110.inc"
#undef NOVA_NEOFORGE12110_PROFILE
#define NOVA_FORGE1201_PROFILE
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_PROFILE
#define NOVA_LUNAR189_PROFILE
#include "minecraft/compat/mappings/Lunar189.inc"
#undef NOVA_LUNAR189_PROFILE
#define NOVA_LUNAR1710_PROFILE
#include "minecraft/compat/mappings/Lunar1710.inc"
#undef NOVA_LUNAR1710_PROFILE
#include "minecraft/java/api/jni_lookup.h"

#include <cstring>

namespace nova::compat {
namespace {
constexpr RenderBackend Forge1201Renderer = RenderBackend::Glfw;
constexpr RenderBackend Lunar189Renderer = RenderBackend::Lwjgl2;
constexpr RenderBackend Lunar1710Renderer = RenderBackend::Lwjgl2;
constexpr RenderBackend Minecraft263Renderer = RenderBackend::Sdl;
constexpr RenderBackend EaseCationRenderer = RenderBackend::Glfw;
constexpr RenderBackend NeoForge12110Renderer = RenderBackend::Glfw;
constexpr RenderBackend Lunar12111Renderer = RenderBackend::Glfw;
constexpr GameMapping Lunar12111Mapping{
    "method_1551", "field_1724", "Lnet/minecraft/class_746;",
    "field_1687", "Lnet/minecraft/class_638;", "field_1755", "Lnet/minecraft/class_437;"
};
constexpr GameMapping Minecraft263Mapping{
    mc263::names::Minecraft_getInstance.name,
    mc263::names::Minecraft_Field_player.name, mc263::names::Minecraft_Field_player.descriptor,
    mc263::names::Minecraft_Field_level.name, mc263::names::Minecraft_Field_level.descriptor,
    nullptr, nullptr // 26.3 resolves Minecraft.gui -> Gui.screen() in its runtime.
};
constexpr std::uint32_t BasicCapabilities = static_cast<std::uint32_t>(Capability::Settings) |
    static_cast<std::uint32_t>(Capability::PlayerEffects) | static_cast<std::uint32_t>(Capability::ClickInput) |
    static_cast<std::uint32_t>(Capability::RotationEvents) |
    static_cast<std::uint32_t>(Capability::NetworkEvents) | static_cast<std::uint32_t>(Capability::BasicPlace);
constexpr std::uint32_t Minecraft263Capabilities = BasicCapabilities;
constexpr std::uint32_t EaseCationCapabilities = BasicCapabilities;
constexpr std::uint32_t NeoForge12110Capabilities = BasicCapabilities;
constexpr std::uint32_t Lunar12111Capabilities = BasicCapabilities;
constexpr std::uint32_t Forge1201Capabilities = static_cast<std::uint32_t>(Capability::Forge1201Events) | BasicCapabilities;
constexpr std::uint32_t Lunar189Capabilities = BasicCapabilities;
constexpr std::uint32_t Lunar1710Capabilities = BasicCapabilities;
constexpr Profile profiles[]{
#define NOVA_PROFILE_minecraft(symbol, value, key, label) {value, key, label, symbol##Renderer, &symbol##Mapping, symbol##Capabilities},
#define NOVA_PROFILE_cs2(symbol, value, key, label)
#define NOVA_PROFILE_nte(symbol, value, key, label)
#define NOVA_PROFILE(symbol, value, key, label, game) NOVA_PROFILE_##game(symbol, value, key, label)
#include "../../../../shared/ClientProfiles.inc"
#undef NOVA_PROFILE
#undef NOVA_PROFILE_cs2
#undef NOVA_PROFILE_nte
#undef NOVA_PROFILE_minecraft
};
const Profile* selected = &profiles[0];
jclass minecraft = nullptr;
jobject loader = nullptr;
jmethodID instance = nullptr;
jfieldID screen = nullptr;
jfieldID player = nullptr;
jfieldID world = nullptr;
jmethodID localServer = nullptr;
jmethodID connection = nullptr;
jclass mouse = nullptr;
jclass chatScreen = nullptr;
jmethodID setGrabbed = nullptr;
jmethodID isGrabbed = nullptr;
bool mouseSaved = false;
bool wasGrabbed = false;

bool Good(JNIEnv* env) {
    if (!env->ExceptionCheck()) return true;
    env->ExceptionClear();
    return false;
}

jclass Loaded(JNIEnv* env, const char* signature) {
    jint count = 0;
    jclass* classes = nullptr;
    if (g_jvmti->GetLoadedClasses(&count, &classes) != JVMTI_ERROR_NONE) return nullptr;
    jclass found = nullptr;
    for (jint i = 0; i < count; ++i) {
        char* sig = nullptr;
        if (!found && g_jvmti->GetClassSignature(classes[i], &sig, nullptr) == JVMTI_ERROR_NONE) {
            if (sig && std::strcmp(sig, signature) == 0)
                found = static_cast<jclass>(env->NewGlobalRef(classes[i]));
            if (sig) g_jvmti->Deallocate(reinterpret_cast<unsigned char*>(sig));
        }
        env->DeleteLocalRef(classes[i]);
    }
    g_jvmti->Deallocate(reinterpret_cast<unsigned char*>(classes));
    return found;
}
}

const Profile& Current() { return *selected; }
bool IsLunar189() { return selected->id == NovaClient_Lunar189; }
bool IsLunar1710() { return selected->id == NovaClient_Lunar1710; }
bool IsLegacyLunar() { return IsLunar189() || IsLunar1710(); }
bool IsMinecraft263() { return selected->id == NovaClient_Minecraft263; }
bool IsEaseCation() { return selected->id == NovaClient_EaseCation; }
bool IsNeoForge12110() { return selected->id == NovaClient_NeoForge12110; }
bool IsLunar12111() { return selected->id == NovaClient_Lunar12111; }
bool UsesGlfwModernRuntime() { return IsEaseCation() || IsNeoForge12110() || IsLunar12111(); }
bool UsesModernRuntime() { return IsMinecraft263() || UsesGlfwModernRuntime(); }
const char* InitializationError() {
    if (IsLunar12111()) return "Lunar 1.21.11 requires its Fabric intermediary runtime and an initialized GLFW window. See nova-base.log.";
    if(IsNeoForge12110())return "Minecraft 1.21.10 (NeoForge) requires its Mojang-named NeoForge runtime and GLFW window. See nova-base.log.";
    return IsEaseCation() ? "EaseCation requires the 1.21.8 NeoForge runtime (Mojang names). See nova-base.log for missing interfaces." : IsMinecraft263() ? mc263::Error() :
        "The selected client profile does not match this JVM, or required game interfaces are unavailable.";
}

bool Has(Capability capability) {
    return (selected->capabilities & static_cast<std::uint32_t>(capability)) != 0;
}

bool Select(unsigned id) {
    for (const auto& profile : profiles) {
        if (profile.id == id) { selected = &profile; return true; }
    }
    return false;
}

bool Initialize(JNIEnv* env) {
    minecraft = Loaded(env, IsLunar12111() ? "Lnet/minecraft/class_310;" : "Lnet/minecraft/client/Minecraft;");
    if (!minecraft) return false;
    jobject localLoader = nullptr;
    if (g_jvmti->GetClassLoader(minecraft, &localLoader) != JVMTI_ERROR_NONE || !localLoader)
        return false;
    loader = env->NewGlobalRef(localLoader);
    env->DeleteLocalRef(localLoader);
    if (!loader) return false;
    const auto& map = *selected->mapping;
    instance = env->GetStaticMethodID(minecraft, map.instance, IsLunar12111() ? "()Lnet/minecraft/class_310;" : "()Lnet/minecraft/client/Minecraft;");
    if (!Good(env) || !instance) return false;
    player = env->GetFieldID(minecraft, map.player, map.playerType);
    if (!Good(env) || !player) return false;
    world = env->GetFieldID(minecraft, map.world, map.worldType);
    if (!Good(env) || !world) return false;
    if (IsLunar12111()) {
        jclass genesis = Loaded(env, "Lcom/moonsworth/lunar/genesis/Genesis;");
        if (!genesis) return false;
        env->DeleteGlobalRef(genesis);
        if (!lunar12111::Initialize(env)) return false;
    } else if (IsEaseCation()) {
        if (!easecation::Initialize(env)) return false;
    } else if (IsNeoForge12110()) {
        if (!neoforge12110::Initialize(env)) return false;
    } else if (IsMinecraft263()) {
        if (!mc263::Initialize(env)) return false;
    } else {
        screen = env->GetFieldID(minecraft, map.screen, map.screenType);
        if (!Good(env) || !screen) return false;
    }
    if (selected->renderer == RenderBackend::Lwjgl2) {
        jclass genesis = Loaded(env, "Lcom/moonsworth/lunar/genesis/Genesis;");
        if (!genesis) return false;
        env->DeleteGlobalRef(genesis);
        mouse = Loaded(env, "Lorg/lwjgl/input/Mouse;");
        if (!mouse) return false;
        setGrabbed = env->GetStaticMethodID(mouse, "setGrabbed", "(Z)V");
        if (!Good(env) || !setGrabbed) return false;
        isGrabbed = env->GetStaticMethodID(mouse, "isGrabbed", "()Z");
        if (!Good(env) || !isGrabbed) return false;
    }
    if (IsLegacyLunar() && !lunar::Initialize(env)) return false;
    LogTo("Profile: %s resolved Minecraft, player, world, screen and class loader", selected->key);
    return true;
}

jobject ClassLoader(JNIEnv* env) { return loader ? env->NewGlobalRef(loader) : nullptr; }

jobject Minecraft(JNIEnv* env) {
    if (!minecraft || !instance) return nullptr;
    jobject value = env->CallStaticObjectMethod(minecraft, instance);
    if (!Good(env)) { if (value) env->DeleteLocalRef(value); return nullptr; }
    return value;
}

jobject LocalPlayer(JNIEnv* env) {
    jobject mc = Minecraft(env);
    if (!mc || !player) { if (mc) env->DeleteLocalRef(mc); return nullptr; }
    jobject value = env->GetObjectField(mc, player);
    env->DeleteLocalRef(mc);
    if (!Good(env)) { if (value) env->DeleteLocalRef(value); return nullptr; }
    return value;
}

bool ScreenOpen() {
    JniAttach env;
    if (!env || !minecraft || !instance || (!screen && !UsesModernRuntime())) return true;
    jobject mc = env->CallStaticObjectMethod(minecraft, instance);
    if (!Good(env.env) || !mc) { if (mc) env->DeleteLocalRef(mc); return true; }
    jobject value = UsesModernRuntime() ? mc263::Screen(env.env, mc) : env->GetObjectField(mc, screen);
    const bool open = !Good(env.env) || value != nullptr;
    if (value) env->DeleteLocalRef(value);
    env->DeleteLocalRef(mc);
    return open;
}

bool ChatScreenOpen() {
    JniAttach attach;
    if (!attach || (!screen && !UsesModernRuntime())) return true;
    JNIEnv* env = attach.env;
    jobject mc = Minecraft(env);
    if (!mc) return true;
    jobject current = UsesModernRuntime() ? mc263::Screen(env, mc) : env->GetObjectField(mc, screen);
    env->DeleteLocalRef(mc);
    if (!Good(env)) {
        if (current) env->DeleteLocalRef(current);
        return true;
    }
    if (!current) return false;
    if (!chatScreen) {
        const char* name = (IsLunar189() || IsLunar1710()) ? "net.minecraft.client.gui.GuiChat"
                                       : Forge1201ChatScreenDot;
        const char* signature = (IsLunar189() || IsLunar1710()) ? "Lnet/minecraft/client/gui/GuiChat;"
                                            : Forge1201ChatScreenSig;
        jclass local = nova_base::loadOrFind(env, loader, name, signature);
        if (Good(env) && local) chatScreen = static_cast<jclass>(env->NewGlobalRef(local));
        if (local) env->DeleteLocalRef(local);
    }
    const bool clean = Good(env);
    const bool open = !clean || !chatScreen || env->IsInstanceOf(current, chatScreen);
    env->DeleteLocalRef(current);
    return open;
}

bool ValidatePlayer() {
    JniAttach env;
    if (!env || !minecraft) return false;
    jobject mc = env->CallStaticObjectMethod(minecraft, instance);
    if (!Good(env.env) || !mc) { if (mc) env->DeleteLocalRef(mc); return false; }
    jobject p = env->GetObjectField(mc, player);
    jobject w = env->GetObjectField(mc, world);
    const bool valid = Good(env.env) && p && w;
    if (p) env->DeleteLocalRef(p);
    if (w) env->DeleteLocalRef(w);
    env->DeleteLocalRef(mc);
    return valid;
}

bool RemoteWorldReady(JNIEnv* env) {
    if (!env || selected->id != NovaClient_Forge1201 || !minecraft || !player || !world || !screen)
        return false;
    if (!localServer) {
        localServer = env->GetMethodID(minecraft, Forge1201LocalServer, "()Z");
        if (!Good(env) || !localServer) return false;
    }
    if (!connection) {
        connection = env->GetMethodID(minecraft, Forge1201Connection, Forge1201ConnectionDesc);
        if (!Good(env) || !connection) return false;
    }
    if (env->PushLocalFrame(8) != JNI_OK) { Good(env); return false; }
    bool ready = false;
    jobject mc = Minecraft(env);
    if (mc) {
        // A non-null connection alone also occurs during login. Require the
        // player/world and a closed loading screen; integrated worlds do not count.
        const bool local = env->CallBooleanMethod(mc, localServer) == JNI_TRUE;
        if (Good(env) && !local) {
            jobject p = env->GetObjectField(mc, player);
            if (Good(env) && p) {
                jobject w = env->GetObjectField(mc, world);
                if (Good(env) && w) {
                    jobject gui = env->GetObjectField(mc, screen);
                    if (Good(env) && !gui) {
                        jobject listener = env->CallObjectMethod(mc, connection);
                        ready = Good(env) && listener;
                    }
                }
            }
        }
    }
    env->PopLocalFrame(nullptr);
    return ready;
}

bool CaptureMouse(bool capture) {
    if (UsesModernRuntime()) return mc263::CaptureMouse(capture);
    JniAttach env;
    if (!env || !mouse || !setGrabbed) return false;
    if (capture && !mouseSaved) {
        wasGrabbed = env->CallStaticBooleanMethod(mouse, isGrabbed) == JNI_TRUE;
        if (!Good(env.env)) return false;
        mouseSaved = true;
    }
    if (!capture && !mouseSaved) return true;
    const bool grabbed = capture ? false : wasGrabbed;
    env->CallStaticVoidMethod(mouse, setGrabbed, grabbed ? JNI_TRUE : JNI_FALSE);
    if (!Good(env.env)) return false;
    if (!capture) mouseSaved = false;
    return true;
}

void RestoreMouse() { CaptureMouse(false); }

void Shutdown(JNIEnv* env) {
    java::ReleaseBridgeAssistAccess(env);
    if (UsesModernRuntime()) mc263::Shutdown(env);
    if (IsEaseCation()) easecation::Shutdown(env);
    if (IsNeoForge12110()) neoforge12110::Shutdown(env);
    if (IsLunar12111()) { lunar12111::Shutdown(env); easecation::Shutdown(env); }
    if (IsLegacyLunar()) lunar::Shutdown(env);
    if (mouse) env->DeleteGlobalRef(mouse);
    if (chatScreen) env->DeleteGlobalRef(chatScreen);
    if (minecraft) env->DeleteGlobalRef(minecraft);
    if (loader) env->DeleteGlobalRef(loader);
    mouse = nullptr; chatScreen = nullptr; minecraft = nullptr; loader = nullptr;
    instance = nullptr; screen = nullptr; player = nullptr; world = nullptr;
    localServer = nullptr; connection = nullptr;
    setGrabbed = nullptr; isGrabbed = nullptr;
}
}
