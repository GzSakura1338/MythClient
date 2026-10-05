#include "minecraft/java/api/base.h"
#include "minecraft/java/api/hook_bridge.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/game_fakeplayer.h"
#include "minecraft/hooks/Transformer.h"
#include "minecraft/hooks/Edits.h"
#include "minecraft/modules/ModuleManager.h"
#include "minecraft/host/render/OverlayHost.h"
#include "core/SelfUnload.h"
#include "core/Runtime.h"
#include "minecraft/host/MinecraftHost.h"
#include "proxy/ProxyService.h"
#include "minecraft/proxy/ProxyHooks.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar12111/Edits.h"
#include "smtc/SmtcService.h"
#include "chrome/BrowserService.h"
#include "minecraft/accounts/AccountManager.h"
#include "minecraft/accounts/lunar/AccountCompatibility.h"
#include <cstdio>
#include <cstdlib>
#include <windows.h>

#include <atomic>
#include <cwchar>
#include <cstring>

namespace {
nova::HostContext hostContext;
using nova::runtime::AcquireInstance;
using nova::runtime::ReleaseInstance;

bool IsHeyPixelPreload(bool* proxy = nullptr) {
    // The existing HeyPixel helper extracts System.load's DLL under this name.
    // Reflective injection has bootstrap parameters and keeps its normal startup.
    if (hostContext.reflective || hostContext.profile != NovaClient_Forge1201)
        return false;
    HMODULE image = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&IsHeyPixelPreload), &image))
        return false;
    wchar_t path[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(image, path, MAX_PATH);
    if (!length || length >= MAX_PATH) return false;
    const wchar_t* name = std::wcsrchr(path, L'\\');
    name = name ? name + 1 : path;
    if (proxy) *proxy = std::wcsncmp(name, L"nova-client-proxy-", 18) == 0;
    return std::wcsncmp(name, L"nova-client-", 12) == 0 &&
           length >= 4 && _wcsicmp(path + length - 4, L".dll") == 0;
}

bool WaitForHeyPixelGame(JNIEnv* env) {
    LogTo("HeyPixel: winmm startup; waiting for initialized Minecraft interfaces (no server join required)");
    for (int attempt = 0; attempt < 1200; ++attempt) {
        // Do not trigger Minecraft's static initializer on the Nova worker.
        // Forge must initialize the game on its own startup/render thread.
        jint count = 0;
        jclass* classes = nullptr;
        bool initialized = false;
        if (g_jvmti->GetLoadedClasses(&count, &classes) == JVMTI_ERROR_NONE) {
            for (jint i = 0; i < count; ++i) {
                char* signature = nullptr;
                if (!initialized && g_jvmti->GetClassSignature(classes[i], &signature, nullptr) == JVMTI_ERROR_NONE) {
                    if (signature && std::strcmp(signature, "Lnet/minecraft/client/Minecraft;") == 0) {
                        jint status = 0;
                        initialized = g_jvmti->GetClassStatus(classes[i], &status) == JVMTI_ERROR_NONE &&
                                      (status & JVMTI_CLASS_STATUS_INITIALIZED) != 0;
                    }
                    if (signature) g_jvmti->Deallocate(reinterpret_cast<unsigned char*>(signature));
                }
                env->DeleteLocalRef(classes[i]);
            }
            if (classes) g_jvmti->Deallocate(reinterpret_cast<unsigned char*>(classes));
        }
        if (initialized) {
            if (nova::compat::Initialize(env)) {
                jobject mc = nova::compat::Minecraft(env);
                if (mc) {
                    env->DeleteLocalRef(mc);
                    LogTo("HeyPixel: game interfaces ready; starting Nova runtime without waiting for a world");
                    return true;
                }
            }
            nova::compat::Shutdown(env);
        }
        Sleep(250);
    }
    return false;
}

bool WaitForHeyPixelWorld(JNIEnv* env) {
    char name[64];
    std::snprintf(name, sizeof(name), "Local\\NovaUnload-%lu", GetCurrentProcessId());
    HANDLE wake = CreateEventA(nullptr, TRUE, FALSE, name);
    if (!wake) {
        LogTo("HeyPixel: could not create the waiting event; runtime stays inactive");
        return false;
    }
    nova::runtime::SetUnloadEvent(wake);
    LogTo("HeyPixel: waiting for a remote world; modules, bridge, transformer and overlay are inactive");
    unsigned readySamples = 0;
    bool ready = false;
    while (WaitForSingleObject(wake, 250) == WAIT_TIMEOUT) {
        readySamples = nova::compat::RemoteWorldReady(env) ? readySamples + 1 : 0;
        // Avoid activating for a transient world during a failed join.
        if (readySamples >= 4) { ready = true; break; }
    }
    nova::runtime::SetUnloadEvent(nullptr);
    CloseHandle(wake);
    LogTo(ready ? "HeyPixel: remote world ready; starting Nova runtime"
                : "HeyPixel: waiting cancelled; runtime stays inactive");
    return ready;
}

void UnloadAfterFailure(JavaVM* vm = nullptr) {
    if (!DisposeJvmti()) {
        if (vm) vm->DetachCurrentThread();
        return;
    }
    if (vm) vm->DetachCurrentThread();
    nova::smtc::BeginShutdown();
    nova::chrome::Shutdown();
    if (!nova::smtc::WaitStopped(5000)) {
        LogTo("Base: SMTC callbacks have not drained after failed initialization; DLL left loaded");
        return;
    }
    ReleaseInstance();
    if (hostContext.loaderThread && hostContext.loaderThread != GetCurrentThreadId()) {
        HANDLE loader = OpenThread(SYNCHRONIZE, FALSE, hostContext.loaderThread);
        if (loader) {
            WaitForSingleObject(loader, 5000);
            CloseHandle(loader);
        }
    }
    MEMORY_BASIC_INFORMATION region{};
    if (!VirtualQuery(reinterpret_cast<LPCVOID>(&nova::selfunload::UnmapAndExit),
                      &region, sizeof(region)))
        return;
    HMODULE image = static_cast<HMODULE>(region.AllocationBase);
    const DWORD selfThread = GetCurrentThreadId();
    for (int attempt = 0; attempt < 100; ++attempt) {
        if (nova::selfunload::ImageIsClear(image, selfThread)) {
            LogTo("Base: unmapping failed initialization (base=%p)",
                  reinterpret_cast<void*>(image));
            nova::selfunload::UnmapAndExit(image);
            return;
        }
        Sleep(10);
    }
    LogTo("Base: failed initialization could not be unmapped safely");
}

DWORD FailAndUnload(const char* message, JavaVM* vm = nullptr) {
    ReportBootstrapFailed(message);
    UnloadAfterFailure(vm);
    return 0;
}
}

namespace nova::minecraft {
unsigned long RunHost(HostContext context) {
    hostContext=context;
    if(const char* appdata=std::getenv("APPDATA");appdata && std::strlen(appdata)+32<MAX_PATH) {
        char path[MAX_PATH];std::snprintf(path,sizeof(path),"%s\\.minecraft\\nova-base.log",appdata);
        nova::runtime::SetLogPath(path);
    }
    bool heyPixelProxy = false;
    const bool heyPixelPreload = IsHeyPixelPreload(&heyPixelProxy);
    if (!AcquireInstance())
        return FailAndUnload("Another Nova instance is already active in this process.");
    if (!nova::compat::Select(hostContext.profile)) {
        return FailAndUnload("Unknown client profile. Update the injector and DLL together.");
    }
    HMODULE jvm = nullptr;
    for (int i = 0; i < 600 && !jvm; ++i) { jvm = GetModuleHandleA("jvm.dll"); if (!jvm) Sleep(100); }
    if (!jvm) return FailAndUnload("jvm.dll was not found");
    if (heyPixelProxy) {
        // JDK 17.0.2 reports a VM from JNI_GetCreatedJavaVMs while creation is
        // still in progress; attaching at that point can crash in jvm.dll.
        // Forge's GLFW is loaded by Java only after VM startup. Nova does not
        // import GLFW, so mapping Nova early cannot satisfy this condition.
        LogTo("HeyPixel: winmm image loaded; waiting for Java graphics startup (glfw.dll)");
        HMODULE glfw = nullptr;
        for (int i = 0; i < 1200 && !glfw; ++i) {
            glfw = GetModuleHandleW(L"glfw.dll");
            if (!glfw) Sleep(250);
        }
        if (!glfw) return FailAndUnload("HeyPixel Java graphics startup timed out");
    }
    using GetVms = jint (JNICALL*)(JavaVM**, jsize, jsize*);
    auto getVms = reinterpret_cast<GetVms>(GetProcAddress(jvm, "JNI_GetCreatedJavaVMs"));
    jsize count = 0;
    if (heyPixelProxy) LogTo("HeyPixel: winmm startup; waiting for JVM creation");
    const int vmAttempts = heyPixelProxy ? 1200 : 1;
    for (int i = 0; getVms && i < vmAttempts; ++i) {
        if (getVms(&g_vm, 1, &count) == JNI_OK && count && g_vm) break;
        g_vm = nullptr;
        if (heyPixelProxy) Sleep(250);
    }
    if (!getVms || !count || !g_vm)
        return FailAndUnload("Could not resolve the JVM");
    JNIEnv* env = nullptr;
    if (heyPixelProxy) LogTo("HeyPixel: JVM enumerated; attaching native runtime");
    JavaVMAttachArgs args{JNI_VERSION_1_8, nullptr, nullptr};
    if (g_vm->AttachCurrentThreadAsDaemon(reinterpret_cast<void**>(&env), &args) != JNI_OK || !env)
        return FailAndUnload("Could not attach to the JVM");
    if (g_vm->GetEnv(reinterpret_cast<void**>(&g_jvmti), JVMTI_VERSION_1_2) != JNI_OK || !g_jvmti)
        return FailAndUnload("JVMTI is unavailable", g_vm);
    JavaVM* runtimeVm = g_vm;

    if (!(heyPixelProxy ? WaitForHeyPixelGame(env) : nova::compat::Initialize(env))) {
        nova::compat::Shutdown(env);
        return FailAndUnload(
            nova::compat::InitializationError(),
            runtimeVm);
    }

    if (heyPixelPreload && !heyPixelProxy && !WaitForHeyPixelWorld(env)) {
        nova::compat::Shutdown(env);
        DisposeJvmti();
        runtimeVm->DetachCurrentThread();
        ReleaseInstance();
        // System.load owns this image; do not manually unmap a JVM-owned library.
        return 0;
    }

    const bool lunar = nova::compat::IsLegacyLunar();
    nova::hook::SelectLunarEdits(lunar);
    nova::hook::SelectLunar1710Edits(nova::compat::IsLunar1710());
    const bool mc263 = nova::compat::UsesModernRuntime();
    nova::hook::SelectMinecraft263Edits(mc263);
    nova::hook::SelectEaseCationEdits(nova::compat::IsEaseCation());
    nova::hook::SelectNeoForge12110Edits(nova::compat::IsNeoForge12110());
    if(nova::compat::IsLunar12111()) nova::compat::lunar12111::SelectEdits(env);
    if(nova::compat::IsLunar189()) nova::minecraft::accounts::SelectLegacyAccountEdits(env);
    bool bridgeReady = false;
    if (lunar || mc263 || nova::compat::Has(nova::compat::Capability::Forge1201Events)) {
        bool installed = false;
        for (int attempt = 0; attempt < 120 && !installed; ++attempt) {
            jobject loader = nova::compat::ClassLoader(env);
            if (loader) {
                installed = nova::bridge::Install(env, loader);
                env->DeleteGlobalRef(loader);
            }
            if (!installed) Sleep(500);
        }
        if (!installed) {
            if (lunar || mc263) {
                nova::bridge::Release(env);
                nova::compat::Shutdown(env);
                return FailAndUnload("The selected client event bridge could not be installed.", runtimeVm);
            }
            LogTo("Base: the hook bridge was not installed; the game will raise no events");
        }
        bridgeReady = installed;
    }
    nova::module::RegisterBuiltinModules();
    LogTo("Base: %zu modules registered", nova::module::ModuleManager::Get().Count());

    if (bridgeReady && !nova::hook::EnableTransformer(g_jvmti, env)) {
        if (lunar || mc263) {
            OpenCircuit();
            ReportBootstrapFailed("The selected client event rules did not all apply. Runtime disabled; see nova-base.log.");
            const bool restored = nova::hook::DisableTransformer(g_jvmti, env);
            LogTo("Base: failed client event installation; restored=%d; inert image retained", restored);
            runtimeVm->DetachCurrentThread();
            return 0;
        }
        LogTo("Base: the transformer was not enabled; some game events may be unavailable");
    }

    char wakeName[64];
    std::snprintf(wakeName, sizeof(wakeName), "Local\\NovaUnload-%lu", GetCurrentProcessId());
    HANDLE wake = CreateEventA(nullptr, TRUE, FALSE, wakeName);
    if (!wake) {
        ReportBootstrapFailed("Could not create the unload event");
        runtimeVm->DetachCurrentThread();
        return 0;
    }
    nova::runtime::SetUnloadEvent(wake);
    bool hookReady = false;
    nova::minecraft::accounts::Start();
    for (int attempt = 0; attempt < 20 && !hookReady; ++attempt) {
        hookReady = nova::glhook::Install();
        if (!hookReady) Sleep(500);
    }
    if (!hookReady)
        ReportBootstrapFailed("The selected render backend could not be installed");
    // Forge may finish binding its HWND asynchronously after the swap hook is
    // installed. Only a submitted overlay frame confirms that input and GL work.
    else if (!nova::compat::UsesModernRuntime() &&
             nova::compat::Current().id != NovaClient_Forge1201)
        ReportBootstrapReady();
    else if (!nova::glhook::WaitForFirstFrame(15000)) {
        ReportBootstrapFailed("The selected client did not present a Nova frame. See nova-base.log; restart the game before retrying if teardown did not finish.");
        hookReady = false;
    }
    LogTo("Base: JNI/JVMTI runtime installed (pid=%lu); DEL unloads the DLL", GetCurrentProcessId());

    if (hookReady) {
        if (runtimeVm) runtimeVm->DetachCurrentThread();

        while (WaitForSingleObject(wake, 100) == WAIT_TIMEOUT) {}
        nova::runtime::SetUnloadEvent(nullptr);
        CloseHandle(wake);

        if (runtimeVm->AttachCurrentThreadAsDaemon(reinterpret_cast<void**>(&env), nullptr) != JNI_OK || !env) {
            LogTo("Base: the runtime thread could not re-attach; DLL left loaded");
            return 0;
        }
    } else {
        nova::runtime::SetUnloadEvent(nullptr);
        CloseHandle(wake);
    }

    OpenCircuit();
    LogTo("Base: DEL pressed; beginning full unload");
    nova::minecraft::accounts::BeginShutdown();
    nova::proxy::BeginShutdown();
    nova::smtc::BeginShutdown();
    nova::chrome::Shutdown();

    if (!nova::glhook::WaitForTeardown(5000)) {
        LogTo("Base: render thread never tore down (no frames swapped?); DLL left loaded");
        if (runtimeVm) runtimeVm->DetachCurrentThread();
        return 0;
    }

    if (!nova::java::WaitFakePlayerStopped(5000)) {
        LogTo("Base: game thread did not remove FakePlayer; DLL left loaded");
        if (runtimeVm) runtimeVm->DetachCurrentThread();
        return 0;
    }

    if (!nova::minecraft::accounts::WaitStopped(5000)) {
        LogTo("Base: account worker has not drained; DLL left loaded");
        if (runtimeVm) runtimeVm->DetachCurrentThread();
        return 0;
    }
    if (!nova::proxy::WaitStopped(5000)) {
        LogTo("Base: proxy workers have not drained; DLL left loaded");
        if(runtimeVm)runtimeVm->DetachCurrentThread();return 0;
    }
    if (!nova::smtc::WaitStopped(5000)) {
        LogTo("Base: SMTC worker or callbacks have not drained; DLL left loaded");
        if (runtimeVm) runtimeVm->DetachCurrentThread();
        return 0;
    }

    if (!nova::hook::DisableTransformer(g_jvmti, env)) {
        LogTo("Base: the game's classes could not be put back; DLL left loaded");
        if (runtimeVm) runtimeVm->DetachCurrentThread();
        return 0;
    }
    if (bridgeReady && !nova::hook::WaitForQuiescence(g_jvmti, env, 5000)) {
        LogTo("Base: Java callbacks have not drained; DLL left loaded");
        if (runtimeVm) runtimeVm->DetachCurrentThread();
        return 0;
    }

    MEMORY_BASIC_INFORMATION region{};
    if (!VirtualQuery(reinterpret_cast<LPCVOID>(&nova::selfunload::UnmapAndExit),
                      &region, sizeof(region))) {
        LogTo("Base: cannot locate the image base; DLL left loaded");
        if (runtimeVm) runtimeVm->DetachCurrentThread();
        return 0;
    }
    HMODULE image = static_cast<HMODULE>(region.AllocationBase);

    const DWORD selfThread = GetCurrentThreadId();
    bool clear = false;
    for (int attempt = 0; attempt < 40 && !clear; ++attempt) {
        clear = nova::selfunload::ImageIsClear(image, selfThread);
        if (!clear) Sleep(50);
    }
    if (!clear) {
        LogTo("Base: a thread is still executing inside the image; DLL left loaded");
        if (runtimeVm) runtimeVm->DetachCurrentThread();
        return 0;
    }

    nova::glhook::FinalizeHooks();
    if (!nova::bridge::Release(env)) {
        if (runtimeVm) runtimeVm->DetachCurrentThread();
        return 0;
    }
    nova::compat::Shutdown(env);
    nova::minecraft::proxyadapter::Shutdown(env);
    nova::java::ReleaseNetwork(env);
    if (!DisposeJvmti()) {
        if (runtimeVm) runtimeVm->DetachCurrentThread();
        return 0;
    }
    if (runtimeVm) runtimeVm->DetachCurrentThread();
    g_vm = nullptr;
    ReleaseInstance();

    LogTo("Base: unmapping (base=%p)", reinterpret_cast<void*>(image));
    nova::selfunload::UnmapAndExit(image);
    LogTo("Base: the unload stub could not be built; DLL left loaded but inert");
    return 0;
}
}
