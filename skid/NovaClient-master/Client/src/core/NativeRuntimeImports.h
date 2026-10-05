#pragma once
#include "core/RuntimeImports.h"

// A normally loaded image bypasses ReflectiveLoader. Rebind only its own CRT
// imports before the runtime worker uses any locks; leave the JVM's IAT alone.
inline bool NovaRebindNativeRuntime(HMODULE image, const NovaBootstrap* boot) {
    auto* base = reinterpret_cast<unsigned char*>(image);
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (!base || dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0) return false;
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) return false;
    const SIZE_T size = nt->OptionalHeader.SizeOfImage;
    auto valid = [size](SIZE_T rva, SIZE_T bytes) { return rva < size && bytes <= size - rva; };
    const auto directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!valid(directory.VirtualAddress, directory.Size) || directory.Size < sizeof(IMAGE_IMPORT_DESCRIPTOR)) return false;
    HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
    for (SIZE_T offset = 0; offset + sizeof(IMAGE_IMPORT_DESCRIPTOR) <= directory.Size; offset += sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
        auto* import = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + directory.VirtualAddress + offset);
        if (!import->Name) return true;
        if (!valid(import->Name, 1)) return false;
        const char* name = reinterpret_cast<const char*>(base + import->Name);
        if (!NovaUsesSystemRuntime(boot, name)) continue;
        HMODULE runtime = NovaLoadProfileImport(boot, name, kernel, LoadLibraryA, GetProcAddress);
        if (!runtime || !import->OriginalFirstThunk || !import->FirstThunk) return false;
        for (SIZE_T index = 0;; ++index) {
            const SIZE_T original = import->OriginalFirstThunk + index * sizeof(IMAGE_THUNK_DATA64);
            const SIZE_T target = import->FirstThunk + index * sizeof(IMAGE_THUNK_DATA64);
            if (!valid(original, sizeof(IMAGE_THUNK_DATA64)) || !valid(target, sizeof(IMAGE_THUNK_DATA64))) return false;
            const auto entry = reinterpret_cast<IMAGE_THUNK_DATA64*>(base + original)->u1.AddressOfData;
            if (!entry) break;
            LPCSTR symbol;
            if (IMAGE_SNAP_BY_ORDINAL64(entry)) symbol = reinterpret_cast<LPCSTR>(IMAGE_ORDINAL64(entry));
            else {
                if (!valid(entry, sizeof(IMAGE_IMPORT_BY_NAME))) return false;
                symbol = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + entry)->Name;
            }
            FARPROC function = GetProcAddress(runtime, symbol);
            if (!function) return false;
            auto* slot = &reinterpret_cast<IMAGE_THUNK_DATA64*>(base + target)->u1.Function;
            DWORD previous = 0, ignored = 0;
            if (!VirtualProtect(slot, sizeof(*slot), PAGE_READWRITE, &previous)) return false;
            *slot = reinterpret_cast<ULONGLONG>(function);
            if (!VirtualProtect(slot, sizeof(*slot), previous, &ignored)) return false;
        }
    }
    return false;
}
