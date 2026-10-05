#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
#include <string>
#include "../../../shared/Bootstrap.h"

// Local subprocess only: exercise the production reflective loader without a JVM.
int main(int argc, char** argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    if (argc != 4) return 1;
    for (const char* name : {"vcruntime140.dll", "vcruntime140_1.dll", "msvcp140.dll"}) {
        if (!LoadLibraryA((std::string(argv[1]) + "/" + name).c_str())) return 2;
    }
    std::ifstream input(argv[2], std::ios::binary);
    std::vector<char> data{std::istreambuf_iterator<char>(input), {}};
    if (data.size() < sizeof(IMAGE_DOS_HEADER)) return 3;
    auto* raw = static_cast<unsigned char*>(VirtualAlloc(nullptr, data.size(), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
    if (!raw) return 4;
    std::memcpy(raw, data.data(), data.size());
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(raw + reinterpret_cast<IMAGE_DOS_HEADER*>(raw)->e_lfanew);
    auto offset = [&](DWORD rva) -> DWORD {
        auto* section = IMAGE_FIRST_SECTION(nt);
        for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            if (rva >= section[i].VirtualAddress && rva - section[i].VirtualAddress < section[i].SizeOfRawData)
                return section[i].PointerToRawData + rva - section[i].VirtualAddress;
        }
        return 0;
    };
    auto* exports = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(raw + offset(nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress));
    auto* names = reinterpret_cast<DWORD*>(raw + offset(exports->AddressOfNames));
    auto* ordinals = reinterpret_cast<WORD*>(raw + offset(exports->AddressOfNameOrdinals));
    auto* functions = reinterpret_cast<DWORD*>(raw + offset(exports->AddressOfFunctions));
    for (DWORD i = 0; i < exports->NumberOfNames; ++i) {
        if (std::strcmp(reinterpret_cast<char*>(raw + offset(names[i])), "ReflectiveLoader")) continue;
        auto loader = reinterpret_cast<ULONG_PTR(WINAPI*)(void*)>(raw + offset(functions[ordinals[i]]));
        NovaBootstrap boot{};
        boot.magic = NOVA_BOOTSTRAP_MAGIC;
        boot.size = sizeof(boot);
        boot.abi = NOVA_BOOTSTRAP_ABI;
        boot.client = static_cast<unsigned>(std::strtoul(argv[3], nullptr, 10));
        loader(&boot);
        if (boot.state != NovaBoot_Ready) { std::printf("Probe failed: %s\n", boot.message); return 5; }
        std::printf("Reflective runtime profile %u PASS\n", boot.client);
        return 0;
    }
    return 6;
}
