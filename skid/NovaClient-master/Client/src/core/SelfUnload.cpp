#include "core/SelfUnload.h"
#include "core/UnwindTable.h"

#include <cstdint>
#include <cstring>
#include <tlhelp32.h>

namespace nova::selfunload {
namespace {

constexpr size_t kCallSize = 22;

size_t emitCall(unsigned char* out, void* fn, uint64_t arg) {
    out[0] = 0x48; out[1] = 0xB9;
    std::memcpy(out + 2, &arg, sizeof(arg));
    out[10] = 0x48; out[11] = 0xB8;
    std::memcpy(out + 12, &fn, sizeof(fn));
    out[20] = 0xFF; out[21] = 0xD0;
    return kCallSize;
}

size_t emitCall3(unsigned char* out, void* fn, uint64_t a, uint64_t b, uint64_t c) {
    out[0] = 0x48; out[1] = 0xB9;
    std::memcpy(out + 2, &a, sizeof(a));
    out[10] = 0x48; out[11] = 0xBA;
    std::memcpy(out + 12, &b, sizeof(b));
    out[20] = 0x49; out[21] = 0xB8;
    std::memcpy(out + 22, &c, sizeof(c));
    out[30] = 0x48; out[31] = 0xB8;
    std::memcpy(out + 32, &fn, sizeof(fn));
    out[40] = 0xFF; out[41] = 0xD0;
    return 42;
}

size_t emitDrainLoop(unsigned char* out, void* freeLibrary, void* virtualFree,
                     void* exitThread, void* deleteFunctionTable, uint64_t table, uint64_t image) {
    unsigned char* const start = out;

    if (table) {
        // Execute outside the image when removing its unwind metadata. No
        // subsequent call may return to code in the image being released.
        out += emitCall(out, deleteFunctionTable, table);
        *out++ = 0x84; *out++ = 0xC0; // BOOLEAN result is in AL, not EAX.
        *out++ = 0x75; *out++ = static_cast<unsigned char>(kCallSize);
        out += emitCall(out, exitThread, 1); // Keep the image mapped on failure.
        out += emitCall3(out, virtualFree, image, 0, MEM_RELEASE);
    } else {
        // Windows owns the unwind metadata of a normally loaded DLL.
        out += emitCall(out, freeLibrary, image);
    }
    out += emitCall(out, exitThread, 0);
    return static_cast<size_t>(out - start);
}

}

bool ImageIsClear(HMODULE image, DWORD selfThreadId) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return false;

    const DWORD pid = GetCurrentProcessId();
    bool clear = true;

    THREADENTRY32 entry{};
    entry.dwSize = sizeof(entry);
    for (BOOL more = Thread32First(snapshot, &entry); more; more = Thread32Next(snapshot, &entry)) {
        if (!clear)
            break;
        if (entry.th32OwnerProcessID != pid || entry.th32ThreadID == selfThreadId)
            continue;

        HANDLE thread = OpenThread(
            THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
            FALSE, entry.th32ThreadID);
        if (!thread)
            continue;
        if (SuspendThread(thread) == static_cast<DWORD>(-1)) {
            CloseHandle(thread);
            continue;
        }

        CONTEXT context{};
        context.ContextFlags = CONTEXT_CONTROL;
        if (GetThreadContext(thread, &context)) {
            MEMORY_BASIC_INFORMATION region{};
            if (VirtualQuery(reinterpret_cast<LPCVOID>(context.Rip), &region, sizeof(region)) &&
                region.AllocationBase == image) {
                clear = false;
            }
        }

        ResumeThread(thread);
        CloseHandle(thread);
    }

    CloseHandle(snapshot);
    return clear;
}

bool UnmapAndExit(HMODULE image) {
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    if (!kernel32)
        return false;
    void* freeLibrary = reinterpret_cast<void*>(GetProcAddress(kernel32, "FreeLibrary"));
    void* virtualFree = reinterpret_cast<void*>(GetProcAddress(kernel32, "VirtualFree"));
    void* exitThread = reinterpret_cast<void*>(GetProcAddress(kernel32, "ExitThread"));
    void* deleteFunctionTable = reinterpret_cast<void*>(GetProcAddress(kernel32, "RtlDeleteFunctionTable"));
    if (!freeLibrary || !virtualFree || !exitThread)
        return false;

    MEMORY_BASIC_INFORMATION region{};
    if (!VirtualQuery(image, &region, sizeof(region)) || region.AllocationBase != image) return false;
    PRUNTIME_FUNCTION table = nullptr;
    if (region.Type == MEM_PRIVATE) {
        DWORD count = 0;
        table = NovaImageFunctionTable(reinterpret_cast<ULONG_PTR>(image), &count);
        if (!table || !deleteFunctionTable) return false;
    } else if (region.Type != MEM_IMAGE) return false;

    unsigned char* stub = static_cast<unsigned char*>(
        VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!stub)
        return false;

    size_t at = 0;
    stub[at++] = 0x48; stub[at++] = 0x83; stub[at++] = 0xEC; stub[at++] = 0x28;

    at += emitDrainLoop(stub + at, freeLibrary, virtualFree, exitThread, deleteFunctionTable,
                        reinterpret_cast<uint64_t>(table),
                        reinterpret_cast<uint64_t>(image));
    stub[at++] = 0xEB; stub[at++] = 0xFE;

    DWORD previous = 0;
    if (!VirtualProtect(stub, 4096, PAGE_EXECUTE_READ, &previous)) {
        VirtualFree(stub, 0, MEM_RELEASE);
        return false;
    }
    FlushInstructionCache(GetCurrentProcess(), stub, at);

    reinterpret_cast<void (*)()>(stub)();
    return false;
}

}
