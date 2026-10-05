#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <stdexcept>
#include <exception>
#include <thread>
#include "ReflectiveUnwindProbe.h"
#include "core/SelfUnload.h"

using Copy = void* (*)(void*, const void*, size_t);
Copy volatile copyBytes = std::memcpy;
__declspec(noinline) int GuardedRead(const void* address) {
    unsigned long long value = 0;
    __try { copyBytes(&value, address, sizeof(value)); return static_cast<int>(value); }
    __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return 42;
    }
}
bool CheckProtectedRead() {
    void* inaccessible = VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_NOACCESS);
    if (!inaccessible) return false;
    const bool caught = GuardedRead(inaccessible) == 42;
    VirtualFree(inaccessible, 0, MEM_RELEASE);
    return caught;
}
// Runs from the CRT entry point, before InitializeBase.
bool constructorCaught = CheckProtectedRead();

__declspec(noinline) void ProtectedCallback(bool* drained) {
    __try { RaiseException(0xE0424242, 0, 0, nullptr); }
    __finally { *drained = true; }
}
bool CheckCallbackUnwind() {
    bool drained = false;
    __try { ProtectedCallback(&drained); }
    __except(GetExceptionCode() == 0xE0424242 ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return drained;
    }
    return false;
}
// RtlAddFunctionTable is necessary for SEH, but does not make this DLL visible
// to _CxxThrowException's RtlPcToFileHeader lookup. Exercise actual C++ types.
bool CheckConstructorCpp() {
    try {throw std::string("constructor");}
    catch(const std::string& message){return message=="constructor";}
}
bool constructorCppCaught=CheckConstructorCpp();
bool CheckCppUnwind() {
    int destroyed = 0;
    struct Guard {int& count;~Guard(){++count;}};
    try {
        try {Guard guard{destroyed};throw std::string("Browser sign-in timed out. Try again.");}
        catch(const std::string& message) {
            if(message!="Browser sign-in timed out. Try again."||destroyed!=1)return false;
            throw;
        }
    } catch(const std::string& message) {if(message.empty())return false;}
    bool caught=false;
    try {Guard guard{destroyed};throw std::runtime_error("account operation failed");}
    catch(const std::exception& error){caught=std::strcmp(error.what(),"account operation failed")==0;}
    if(!caught||destroyed!=2)return false;
    // Library-originated exception metadata must still use its own runtime.
    try {std::string value="x";(void)value.at(99);return false;}
    catch(const std::out_of_range&){}
    std::exception_ptr pending;
    try {throw std::runtime_error("deferred");}
    catch(...){pending=std::current_exception();}
    try {std::rethrow_exception(pending);}
    catch(const std::runtime_error& error){if(std::strcmp(error.what(),"deferred"))return false;}
    bool workerCaught=false;
    std::thread worker([&]{
        try {throw std::string("Sign-in cancelled.");}
        catch(const std::string& message){workerCaught=message=="Sign-in cancelled.";}
    });
    worker.join();
    return workerCaught;
}
extern "C" __declspec(dllexport) unsigned ProbeExercise() {
    DWORD64 base = 0;
    if (!constructorCaught || !constructorCppCaught || !RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(&GuardedRead), &base, nullptr) ||
        !CheckProtectedRead()) return 1;
    if(!CheckCallbackUnwind())return 2;
    return CheckCppUnwind() ? 0 : 3;
}
extern "C" __declspec(dllexport) DWORD WINAPI ProbeUnload(void* image) {
    nova::selfunload::UnmapAndExit(static_cast<HMODULE>(image));
    return 2; // Successful teardown exits from the external stub.
}
extern "C" void InitializeBase(void* parameter) {
    if (!parameter) return; // Normal LoadLibrary lifecycle is tested separately.
    auto* context = static_cast<UnwindProbeContext*>(parameter);
    MEMORY_BASIC_INFORMATION region{};
    VirtualQuery(reinterpret_cast<void*>(&GuardedRead), &region, sizeof(region));
    context->image = reinterpret_cast<std::uintptr_t>(region.AllocationBase);
    context->guardedRead = reinterpret_cast<std::uintptr_t>(&GuardedRead);
    context->boot.state = ProbeExercise() ? NovaBoot_Failed : NovaBoot_Ready;
}
#ifdef PROBE_REJECT_ENTRY
BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID parameter) {
    if (reason == DLL_PROCESS_ATTACH) { InitializeBase(parameter); return FALSE; }
    return TRUE;
}
#endif
