#pragma once
#include <windows.h>

// Also used before the reflective image's imports are callable: keep this
// helper inline and independent of the CRT and static data.
static __forceinline PRUNTIME_FUNCTION NovaImageFunctionTable(ULONG_PTR image, DWORD* count) {
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)image;
    PIMAGE_NT_HEADERS64 nt;
    IMAGE_DATA_DIRECTORY directory;
    PRUNTIME_FUNCTION table;
    DWORD i;
    *count = 0;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < sizeof(IMAGE_DOS_HEADER) ||
        dos->e_lfanew > 4096) return NULL;
    nt = (PIMAGE_NT_HEADERS64)(image + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXCEPTION) return NULL;
    directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
    if (!directory.VirtualAddress || directory.VirtualAddress % 4 || !directory.Size ||
        directory.Size % sizeof(RUNTIME_FUNCTION) || directory.VirtualAddress >= nt->OptionalHeader.SizeOfImage ||
        directory.Size > nt->OptionalHeader.SizeOfImage - directory.VirtualAddress) return NULL;
    table = (PRUNTIME_FUNCTION)(image + directory.VirtualAddress);
    for (i = 0; i < directory.Size / sizeof(RUNTIME_FUNCTION); ++i) {
        if (table[i].BeginAddress >= table[i].EndAddress || table[i].EndAddress > nt->OptionalHeader.SizeOfImage ||
            !table[i].UnwindData || table[i].UnwindData >= nt->OptionalHeader.SizeOfImage ||
            (i && table[i - 1].EndAddress > table[i].BeginAddress)) return NULL;
    }
    *count = directory.Size / sizeof(RUNTIME_FUNCTION);
    return table;
}
