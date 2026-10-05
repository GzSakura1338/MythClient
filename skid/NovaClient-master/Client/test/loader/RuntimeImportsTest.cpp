#include "core/RuntimeImports.h"
#include <cstdio>
#include <mutex>

static HMODULE WINAPI MissingLibrary(LPCSTR) { return nullptr; }

int main(int argc, char** argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    if (argc != 2) return 1;
    HMODULE old = LoadLibraryA(argv[1]);
    if (!old) { std::printf("Old runtime load failed: %lu\n", GetLastError()); return 2; }
    HMODULE kernel = GetModuleHandleA("kernel32.dll");
    NovaBootstrap boot{};
    boot.magic = NOVA_BOOTSTRAP_MAGIC;
    boot.size = sizeof(boot);
    boot.abi = NOVA_BOOTSTRAP_ABI;
    const unsigned hostProfiles[]{0, NovaClient_Lunar189, NovaClient_Minecraft263,
        NovaClient_Lunar1710, NovaClient_Cs2, NovaClient_Lunar12111, NovaClient_NeoForge12110};
    for (unsigned profile : hostProfiles) {
        boot.client = profile;
        if (NovaLoadProfileImport(&boot, "msvcp140.dll", kernel, LoadLibraryA, GetProcAddress) != old)
            return 3;
    }
    for (unsigned profile : {NovaClient_Forge1201, NovaClient_EaseCation}) {
        boot.client = profile;
        if (NovaUsesSystemRuntime(nullptr, "msvcp140.dll") ||
            NovaUsesSystemRuntime(&boot, "kernel32.dll") ||
            !NovaUsesSystemRuntime(&boot, "MSVCP140.dll")) return 4;
        if (NovaLoadProfileImport(&boot, "msvcp140.dll", kernel, MissingLibrary, GetProcAddress)) return 9;
        boot.abi = NOVA_BOOTSTRAP_ABI + 1;
        if (NovaUsesSystemRuntime(&boot, "msvcp140.dll")) return 10;
        boot.abi = NOVA_BOOTSTRAP_ABI;
        HMODULE current = NovaLoadProfileImport(&boot, "msvcp140.dll", kernel, LoadLibraryA, GetProcAddress);
        if (!current || current == old) { std::printf("System runtime load failed: %lu\n", GetLastError()); return 5; }
        auto lock = reinterpret_cast<decltype(&_Mtx_lock)>(GetProcAddress(current, "_Mtx_lock"));
        auto unlock = reinterpret_cast<decltype(&_Mtx_unlock)>(GetProcAddress(current, "_Mtx_unlock"));
        if (!lock || !unlock) return 6;
        _Mtx_internal_imp_t storage{};
        storage._Thread_id = -1;
        storage._Type = _Mtx_try;
        for (int i = 0; i < 1000; ++i) {
            if (lock(&storage) != _Thrd_result::_Success || unlock(&storage) != _Thrd_result::_Success) return 7;
        }
        if (GetModuleHandleA("msvcp140.dll") != old) return 8;
    }
    boot.size -= 1;
    if (NovaUsesSystemRuntime(&boot, "msvcp140.dll")) return 11;
    boot.size = sizeof(boot);
    boot.magic ^= 1;
    if (NovaUsesSystemRuntime(&boot, "msvcp140.dll")) return 12;
    std::puts("PASS: Forge/EaseCation mutex; other profiles retain host runtime; host runtime unchanged; invalid bootstrap rejected");
}
