#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cstdio>
#include "core/Runtime.h"
#include "core/RuntimeControl.h"
#include "core/Hosts.h"
#include "core/NativeRuntimeImports.h"
#include "../../../shared/Bootstrap.h"
#include "../../../shared/RemoteLoad.h"

namespace {
std::atomic<NovaBootstrap*> bootstrap{nullptr};
nova::HostContext context{NovaClient_Forge1201,0,false};
const nova::GameHost* selectedHost=nullptr;
HANDLE instanceMutex=nullptr;
SRWLOCK unloadLock=SRWLOCK_INIT;
HANDLE unloadEvent=nullptr;
std::atomic<bool> runtimeStarted{false};
void Report(int state,const char* message) {
    auto* boot=bootstrap.exchange(nullptr,std::memory_order_acq_rel);
    if(!boot)return;
    std::snprintf(boot->message,sizeof(boot->message),"%s",message);
    InterlockedExchange(reinterpret_cast<volatile LONG*>(&boot->state),state);
}
DWORD WINAPI RuntimeThread(LPVOID) {
    selectedHost=nova::SelectGameHost(context.profile);
    if(!selectedHost) {ReportBootstrapFailed("Unknown client profile. Update the injector and DLL together.");return 0;}
    return selectedHost->run(context);
}
}
namespace nova::runtime {
bool AcquireInstance() {
    char name[64];std::snprintf(name,sizeof(name),"Local\\NovaClient-%lu",GetCurrentProcessId());
    HANDLE mutex=CreateMutexA(nullptr,FALSE,name);
    if(!mutex)return false;
    if(GetLastError()==ERROR_ALREADY_EXISTS) {CloseHandle(mutex);return false;}
    instanceMutex=mutex;return true;
}
void ReleaseInstance() {
    if(instanceMutex)CloseHandle(instanceMutex);
    instanceMutex=nullptr;
}
void SetUnloadEvent(void* event) {
    AcquireSRWLockExclusive(&unloadLock);unloadEvent=static_cast<HANDLE>(event);ReleaseSRWLockExclusive(&unloadLock);
}
}
void ReportBootstrapReady() {Report(NovaBoot_Ready,selectedHost?selectedHost->label:"Ready");}
void ReportBootstrapFailed(const char* message) {
    LogTo("Profile: initialization failed: %s",message);Report(NovaBoot_Failed,message);
}
void RequestUnload() {
    // Clear and close cannot race a request into a recycled Win32 handle.
    AcquireSRWLockShared(&unloadLock);
    if(unloadEvent)SetEvent(unloadEvent);
    ReleaseSRWLockShared(&unloadLock);
}
extern "C" __declspec(dllexport) unsigned NovaBootstrapVersion() {return NOVA_BOOTSTRAP_ABI;}
extern "C" __declspec(dllexport) unsigned NovaHeyPixelProxyABI_v1() {return 1;}
static void StartBase(void* parameters, bool reflective) {
    if(parameters) {
        auto* boot=static_cast<NovaBootstrap*>(parameters);
        if(boot->magic!=NOVA_BOOTSTRAP_MAGIC)return;
        if(boot->size!=sizeof(NovaBootstrap) || boot->abi!=NOVA_BOOTSTRAP_ABI) {
            std::snprintf(boot->message,sizeof(boot->message),"%s","Injector and DLL bootstrap versions do not match.");
            InterlockedExchange(reinterpret_cast<volatile LONG*>(&boot->state),NovaBoot_Failed);return;
        }
        if (runtimeStarted.exchange(true)) {
            std::snprintf(boot->message,sizeof(boot->message),"Another Nova instance is active in this process.");
            InterlockedExchange(reinterpret_cast<volatile LONG*>(&boot->state),NovaBoot_Failed);return;
        }
        context.profile=boot->client;context.reflective=reflective;bootstrap.store(boot,std::memory_order_release);
    } else if (runtimeStarted.exchange(true)) {
        return;
    }
    context.loaderThread=GetCurrentThreadId();
    HANDLE thread=CreateThread(nullptr,0,RuntimeThread,nullptr,0,nullptr);
    if(thread)CloseHandle(thread);else ReportBootstrapFailed("Could not create the runtime thread");
}

extern "C" void InitializeBase(void* parameters) {
    if (!parameters) {
        // LoadLibraryW runs DllMain before a caller can pass the client profile.
        // Defer only the marked remote load; HeyPixel's prelaunch path stays automatic.
        char name[64];std::snprintf(name,sizeof(name),NOVA_REMOTE_LOAD_EVENT,GetCurrentProcessId());
        HANDLE deferred=OpenEventA(SYNCHRONIZE,FALSE,name);
        if (deferred) {CloseHandle(deferred);return;}
    }
    StartBase(parameters,parameters != nullptr);
}

extern "C" IMAGE_DOS_HEADER __ImageBase;
extern "C" __declspec(dllexport) DWORD WINAPI NovaInitializeRemoteThread(void* parameters) {
    auto* boot=static_cast<NovaBootstrap*>(parameters);
    if (!boot || boot->magic!=NOVA_BOOTSTRAP_MAGIC || boot->size!=sizeof(*boot) ||
        boot->abi!=NOVA_BOOTSTRAP_ABI) return ERROR_INVALID_PARAMETER;
    if (boot->client!=NovaClient_Forge1201 ||
        !NovaRebindNativeRuntime(reinterpret_cast<HMODULE>(&__ImageBase),boot)) {
        std::snprintf(boot->message,sizeof(boot->message),"The remote load could not bind compatible runtime imports.");
        InterlockedExchange(reinterpret_cast<volatile LONG*>(&boot->state),NovaBoot_Failed);
        FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(&__ImageBase),ERROR_DLL_INIT_FAILED);
    }
    StartBase(parameters,false);
    return ERROR_SUCCESS;
}
