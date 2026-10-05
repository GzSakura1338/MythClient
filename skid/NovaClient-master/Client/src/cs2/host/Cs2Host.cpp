#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <cwchar>
#include <cstdio>
#include "cs2/host/Cs2Host.h"
#include "cs2/host/Presentation.h"
#include "cs2/hooks/Dx11Hooks.h"
#include "cs2/sdk/Interfaces.h"
#include "core/Runtime.h"
#include "core/RuntimeControl.h"
#include "core/SelfUnload.h"
#include "chrome/BrowserService.h"
#include "smtc/SmtcService.h"
#include "bind/UnloadKey.h"

namespace nova::cs2 {
namespace {
void Unmap(HostContext context) {
    if (!context.reflective) return;
    if (context.loaderThread && context.loaderThread != GetCurrentThreadId()) {
        if (auto thread = OpenThread(SYNCHRONIZE, FALSE, context.loaderThread)) {
            const auto result = WaitForSingleObject(thread, 5000); CloseHandle(thread);
            if (result != WAIT_OBJECT_0) return;
        }
    }
    MEMORY_BASIC_INFORMATION region{};
    if (!VirtualQuery(reinterpret_cast<void*>(&Unmap), &region, sizeof(region))) return;
    auto image = static_cast<HMODULE>(region.AllocationBase);
    for (unsigned i = 0; i < 100; ++i) {
        if (selfunload::ImageIsClear(image, GetCurrentThreadId())) {
            LogTo("CS2: callbacks drained; unmapping runtime");
            selfunload::UnmapAndExit(image);
            return;
        }
        Sleep(20);
    }
    LogTo("CS2: runtime retained because callbacks could not be proven drained");
}
}
unsigned long RunHost(HostContext context) {
    wchar_t local[MAX_PATH]{};
    DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", local, MAX_PATH);
    std::filesystem::path directory = length && length < MAX_PATH ? std::filesystem::path(local) : std::filesystem::temp_directory_path();
    directory /= L"Nova"; directory /= L"cs2";
    std::error_code error; std::filesystem::create_directories(directory, error);
    runtime::SetLogPath((directory / L"nova-cs2.log").string().c_str());
    if (!runtime::AcquireInstance()) { ReportBootstrapFailed("Another Nova instance is active in this process."); Unmap(context); return 0; }
    wchar_t executable[MAX_PATH]{};
    GetModuleFileNameW(nullptr, executable, MAX_PATH);
    const auto filename = std::filesystem::path(executable).filename().wstring();
    if (_wcsicmp(filename.c_str(), L"cs2.exe")) {
        ReportBootstrapFailed("CS2 profile requires cs2.exe."); runtime::ReleaseInstance(); Unmap(context); return 0;
    }
    char eventName[64]; std::snprintf(eventName, sizeof(eventName), "Local\\NovaUnload-%lu", GetCurrentProcessId());
    const auto wake = CreateEventA(nullptr, TRUE, FALSE, eventName);
    if (!wake) { ReportBootstrapFailed("Could not create the CS2 unload event."); runtime::ReleaseInstance(); Unmap(context); return 0; }
    runtime::SetUnloadEvent(wake);
    const auto deadline = GetTickCount64() + 15000;
    bool modulesReady = false;
    do {
        modulesReady = GetModuleHandleW(L"client.dll") && GetModuleHandleW(L"engine2.dll") &&
            GetModuleHandleW(L"schemasystem.dll") && GetModuleHandleW(L"inputsystem.dll") &&
            GetModuleHandleW(L"rendersystemdx11.dll") && GetModuleHandleW(L"SDL3.dll");
    } while (!modulesReady && GetTickCount64() < deadline && WaitForSingleObject(wake, 100) == WAIT_TIMEOUT);
    bool installed = false;
    if (!modulesReady) ReportBootstrapFailed("CS2 DirectX 11 modules are unavailable. Start the game without -vulkan.");
    else if (!sdk::Initialize()) ReportBootstrapFailed("CS2 engine or SDL interfaces are unavailable. See nova-cs2.log.");
    else {
        presentation::Configure(directory / L"configs");
        installed = hooks::Install(presentation::Callbacks());
        if (!installed) ReportBootstrapFailed("Could not install the CS2 DirectX 11 hooks.");
        else {
            const auto frameDeadline = GetTickCount64() + 15000;
            while (!hooks::Ready() && !hooks::Failed() && GetTickCount64() < frameDeadline &&
                   WaitForSingleObject(wake, 20) == WAIT_TIMEOUT) {}
            if (!hooks::Ready() || hooks::Failed()) ReportBootstrapFailed("CS2 did not render the shared UI. See nova-cs2.log.");
            else {
                ReportBootstrapReady();
                LogTo("CS2: runtime ready; unload binding=%d", bind::unload::Key());
                while (!hooks::Failed() && WaitForSingleObject(wake, 20) == WAIT_TIMEOUT) {
                    if (bind::unload::Poll()) {
                        LogTo("CS2: foreground unload binding detected by host; requesting full unload");
                        RequestUnload();
                    }
                }
            }
        }
    }
    LogTo("CS2: teardown starting (pid=%lu)", GetCurrentProcessId());
    OpenCircuit();
    smtc::BeginShutdown();
    chrome::Shutdown();
    // Even a partially failed installation must drain before unloading.
    bool waitingLogged = false;
    while (!hooks::Stop(1000)) {
        if (!waitingLogged) {
            LogTo("CS2: waiting for render/window callbacks; teardown will retry");
            waitingLogged = true;
        }
        Sleep(20);
    }
    LogTo("CS2: render and window callbacks stopped");
    waitingLogged = false;
    while (!smtc::WaitStopped(1000)) {
        if (!waitingLogged) { LogTo("CS2: waiting for media shutdown; teardown will retry"); waitingLogged = true; }
        Sleep(20);
    }
    sdk::Shutdown();
    hooks::Finalize();
    runtime::SetUnloadEvent(nullptr); CloseHandle(wake);
    // Release ownership only after hooks, input and services are inactive.
    // Keeping an inert image mapped must not masquerade as a running instance.
    runtime::ReleaseInstance();
    LogTo("CS2: teardown complete; instance lock released (pid=%lu)", GetCurrentProcessId());
    Unmap(context);
    return 0;
}
}
