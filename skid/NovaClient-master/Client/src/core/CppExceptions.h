#pragma once

// Compiled into ReflectiveLoader.c, but ONLY executed in the mapped image
// after imports and .pdata registration. Do not call this from raw-file code.
#ifdef WIN_X64
typedef void (__stdcall *NOVA_CXX_THROW)(void*, void*);
DLLEXPORT NOVA_CXX_THROW NovaOriginalCxxThrow = NULL;
extern IMAGE_DOS_HEADER __ImageBase;

DLLEXPORT __declspec(noreturn) void __stdcall NovaCxxThrowException(void* object, void* throwInfo) {
    ULONG_PTR base = (ULONG_PTR)&__ImageBase;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(base + __ImageBase.e_lfanew);
    SIZE_T size = nt->OptionalHeader.SizeOfImage;
    ULONG_PTR info = (ULONG_PTR)throwInfo;
    // MSVC's ordinary native ThrowInfo has four 32-bit fields. Pure/WinRT
    // exceptions and metadata owned by another DLL retain the original ABI.
    if (info >= base && size >= 16 && info - base <= size - 16 &&
        !(DEREF_32(info) & (8u | 16u))) {
        ULONG_PTR arguments[4];
        arguments[0] = 0x19930520; // EH_MAGIC_NUMBER1 (native, not pure CLR)
        arguments[1] = (ULONG_PTR)object;
        arguments[2] = info;
        arguments[3] = base;
        // vcruntime's RtlPcToFileHeader cannot discover a manually mapped PE.
        // Supplying its known base allows typed catches and RAII to unwind.
        RaiseException(0xE06D7363, EXCEPTION_NONCONTINUABLE, 4, arguments);
    } else if (NovaOriginalCxxThrow) {
        NovaOriginalCxxThrow(object,throwInfo);
    }
    __fastfail(7); // A noncontinuable throw must never return to its caller.
}

// Raw loader helper: resolve our own export without GetProcAddress/PEB lookup,
// string literals, globals, or calls through the not-yet-resolved import table.
static __forceinline ULONG_PTR NovaMappedExport(ULONG_PTR base, DWORD wanted) {
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(base + ((PIMAGE_DOS_HEADER)base)->e_lfanew);
    DWORD rva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    PIMAGE_EXPORT_DIRECTORY exports;
    DWORD *names, *functions, i;
    WORD* ordinals;
    if (!rva) return 0;
    exports = (PIMAGE_EXPORT_DIRECTORY)(base + rva);
    names = (DWORD*)(base + exports->AddressOfNames);
    functions = (DWORD*)(base + exports->AddressOfFunctions);
    ordinals = (WORD*)(base + exports->AddressOfNameOrdinals);
    for (i=0; i<exports->NumberOfNames; ++i)
        if (hash((char*)(base+names[i])) == wanted)
            return base + functions[ordinals[i]];
    return 0;
}
#endif
