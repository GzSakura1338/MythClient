#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <stdexcept>
#include "core/NativeRuntimeImports.h"
#include "../../shared/RemoteLoad.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;
extern "C" decltype(&_Mtx_lock) __imp__Mtx_lock;
static bool deferred = false;

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, void*) {
    if (reason == DLL_PROCESS_ATTACH) {
        char name[64];std::snprintf(name,sizeof(name),NOVA_REMOTE_LOAD_EVENT,GetCurrentProcessId());
        HANDLE marker=OpenEventA(SYNCHRONIZE,FALSE,name);
        deferred=marker!=nullptr;
        if(marker)CloseHandle(marker);
    }
    return TRUE;
}

extern "C" __declspec(dllexport) DWORD WINAPI NovaInitializeRemoteThread(void* parameter) {
    auto* boot=static_cast<NovaBootstrap*>(parameter);
    if(!deferred || boot->client!=NovaClient_Forge1201 ||
        !NovaRebindNativeRuntime(reinterpret_cast<HMODULE>(&__ImageBase),boot))return 1;
    HMODULE imported=nullptr;
    if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(__imp__Mtx_lock),&imported) || imported==GetModuleHandleA("msvcp140.dll"))return 2;
    std::mutex mutex;
    std::unique_lock lock(mutex);
    std::condition_variable condition;
    if(condition.wait_for(lock,std::chrono::milliseconds(1))!=std::cv_status::timeout)return 3;
    lock.unlock();
    unsigned count=0;
    auto increment=[&]{for(unsigned i=0;i<1000;++i){std::lock_guard guard(mutex);++count;}};
    std::thread worker(increment);increment();worker.join();
    if(count!=2000)return 4;
    try {std::lock_guard guard(mutex);throw std::runtime_error("normal load RAII probe");}
    catch(const std::runtime_error&) {}
    if(!mutex.try_lock())return 5;
    mutex.unlock();
    std::snprintf(boot->message,sizeof(boot->message),"remote LoadLibraryW, deferred entry, compatible CRT, mutex/threads/exceptions PASS");
    InterlockedExchange(reinterpret_cast<volatile LONG*>(&boot->state),NovaBoot_Ready);
    return 0;
}
