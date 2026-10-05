#pragma once

#include "ReflectiveLoader.h"
#include "../../../shared/Bootstrap.h"

// These helpers run from the unmapped reflective loader: no CRT, static data,
// string literals, or out-of-line calls are allowed here.
static __forceinline void NovaReportRuntimeImportFailure(NovaBootstrap* boot, const char* name) {
    UINT i = 0;
    while (name[i] && i < sizeof(boot->message) - 1) {
        boot->message[i] = name[i];
        ++i;
    }
    boot->message[i] = 0;
    boot->state = NovaBoot_Failed;
}

static __forceinline BOOL NovaUsesSystemRuntime(const NovaBootstrap* boot, const char* name) {
    DWORD value = 0;
    // These launchers bundle MSVC 2015 runtimes that cannot lock the mutex
    // layout emitted by the current compiler. Redirect only Nova's imports.
    if (!boot || boot->magic != NOVA_BOOTSTRAP_MAGIC || boot->abi != NOVA_BOOTSTRAP_ABI ||
        boot->size != sizeof(NovaBootstrap) ||
        (boot->client != NovaClient_EaseCation && boot->client != NovaClient_Forge1201))
        return FALSE;
    while (*name) {
        unsigned char c = (unsigned char)*name++;
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        value = _rotr(value, 13) + c;
    }
    return value == 0x8404d7a3 || // msvcp140.dll
        value == 0x0892b765 ||   // msvcp140_1.dll
        value == 0x0892c765 ||   // msvcp140_2.dll
        value == 0xe515ef09 ||   // msvcp140_atomic_wait.dll
        value == 0xf7a67b55 ||   // msvcp140_codecvt_ids.dll
        value == 0xc70801a2 ||   // vcruntime140.dll
        value == 0xc95d3735;     // vcruntime140_1.dll
}

static __forceinline HMODULE NovaLoadProfileImport(const NovaBootstrap* boot, const char* name,
    HMODULE kernel, LOADLIBRARYA loadLibrary, GETPROCADDRESS getProcAddress) {
    typedef UINT (WINAPI *GETSYSTEMDIRECTORYA)(LPSTR, UINT);
    GETSYSTEMDIRECTORYA getDirectory = NULL;
    char path[MAX_PATH];
    UINT length;
    UINT index;
    ULONG_PTR base;
    PIMAGE_NT_HEADERS nt;
    PIMAGE_EXPORT_DIRECTORY exports;
    DWORD* names;
    if (!NovaUsesSystemRuntime(boot, name)) return loadLibrary(name);

    // Resolve by export name so forwarded kernel32 exports work too.
    base = (ULONG_PTR)kernel;
    nt = (PIMAGE_NT_HEADERS)(base + ((PIMAGE_DOS_HEADER)base)->e_lfanew);
    exports = (PIMAGE_EXPORT_DIRECTORY)(base + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
    names = (DWORD*)(base + exports->AddressOfNames);
    for (index = 0; index < exports->NumberOfNames; ++index) {
        char* exported = (char*)(base + names[index]);
        if (hash(exported) == 0xb8e579c1) { // GetSystemDirectoryA
            getDirectory = (GETSYSTEMDIRECTORYA)getProcAddress(kernel, exported);
            break;
        }
    }
    if (!getDirectory) return NULL;
    length = getDirectory(path, MAX_PATH);
    if (!length || length >= MAX_PATH - 2) return NULL;
    path[length++] = '\\';
    for (index = 0; name[index]; ++index) {
        if (length >= MAX_PATH - 1) return NULL;
        path[length++] = name[index];
    }
    path[length] = 0;
    // An absolute path loads the system copy even when a same-named, old
    // launcher runtime is already loaded. Only Nova's IAT uses this handle.
    // Never fall back to the known-incompatible launcher copy on failure.
    return loadLibrary(path);
}
