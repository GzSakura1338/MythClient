#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdint.h>
#include "RemoteThreadLoad.h"
#include "../shared/RemoteLoad.h"

static BOOL ModuleByName(DWORD pid, const wchar_t* name, BOOL fullPath, ULONG_PTR* base) {
    HANDLE snapshot = INVALID_HANDLE_VALUE;
    for (int attempt = 0; attempt < 10; ++attempt) {
        snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snapshot != INVALID_HANDLE_VALUE || GetLastError() != ERROR_BAD_LENGTH) break;
        Sleep(10);
    }
    if (snapshot == INVALID_HANDLE_VALUE) return FALSE;
    MODULEENTRY32W entry = {0}; entry.dwSize = sizeof(entry);
    BOOL found = FALSE;
    for (BOOL more = Module32FirstW(snapshot, &entry); more; more = Module32NextW(snapshot, &entry)) {
        if (_wcsicmp(fullPath ? entry.szExePath : entry.szModule, name) == 0) {
            *base = (ULONG_PTR)entry.modBaseAddr; found = TRUE; break;
        }
    }
    CloseHandle(snapshot);
    return found;
}

static LPTHREAD_START_ROUTINE RemoteApi(DWORD pid, const char* name) {
    FARPROC function = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), name);
    HMODULE owner = NULL;
    wchar_t path[MAX_PATH];
    if (!function || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)function, &owner) ||
        !GetModuleFileNameW(owner, path, MAX_PATH)) return NULL;
    const wchar_t* leaf = wcsrchr(path, L'\\');
    ULONG_PTR remote = 0;
    if (!ModuleByName(pid, leaf ? leaf + 1 : path, FALSE, &remote)) return NULL;
    return (LPTHREAD_START_ROUTINE)(remote + (ULONG_PTR)function - (ULONG_PTR)owner);
}

static DWORD ValidImageSize(const void* data, DWORD size) {
    const BYTE* bytes = (const BYTE*)data;
    if (size < sizeof(IMAGE_DOS_HEADER)) return 0;
    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)data;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 ||
        (DWORD)dos->e_lfanew > size || sizeof(IMAGE_NT_HEADERS64) > size - (DWORD)dos->e_lfanew) return 0;
    const IMAGE_NT_HEADERS64* nt = (const IMAGE_NT_HEADERS64*)(bytes + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) return 0;
    // Map locally without executing DllMain or resolving dependencies only after
    // the caller has validated this file. This function checks the image header;
    // the actual exported RVA is obtained from that inert mapping below.
    return nt->OptionalHeader.SizeOfImage;
}

HANDLE LoadRemoteLibraryThread(HANDLE process, DWORD pid, const char* path,
    const void* file, DWORD size, void* bootstrap, BOOL* started, char* message, int messageLength) {
    HANDLE result = NULL, thread = NULL, marker = NULL;
    HMODULE local = NULL;
    void* remotePath = NULL;
    ULONG_PTR loaded = 0;
    BOOL loaderFinished = FALSE, initialized = FALSE;
    wchar_t absolute[MAX_PATH], input[MAX_PATH];
    char markerName[64];
    const DWORD imageSize = ValidImageSize(file, size);
    if (!imageSize || !MultiByteToWideChar(CP_ACP, 0, path, -1, input, MAX_PATH)) {
        snprintf(message, messageLength, "[-] Remote loading requires a valid x64 DLL path."); return NULL;
    }
    DWORD length = GetFullPathNameW(input, MAX_PATH, absolute, NULL);
    if (!length || length >= MAX_PATH) {
        snprintf(message, messageLength, "[-] The DLL path is too long."); return NULL;
    }
    if (ModuleByName(pid, absolute, TRUE, &loaded)) {
        snprintf(message, messageLength, "[-] This DLL is already loaded; unload Nova before retrying."); return NULL;
    }
    do {
        local = LoadLibraryExW(absolute, NULL, DONT_RESOLVE_DLL_REFERENCES);
        FARPROC entry = local ? GetProcAddress(local, NOVA_REMOTE_LOAD_EXPORT) : NULL;
        if (!entry || (ULONG_PTR)entry < (ULONG_PTR)local || (ULONG_PTR)entry - (ULONG_PTR)local >= imageSize) {
            snprintf(message, messageLength, "[-] This DLL does not support remote-thread initialization. Update the DLL."); break;
        }
        const ULONG_PTR entryRva = (ULONG_PTR)entry - (ULONG_PTR)local;
        LPTHREAD_START_ROUTINE load = RemoteApi(pid, "LoadLibraryW");
        if (!load) { snprintf(message, messageLength, "[-] Could not resolve target LoadLibraryW."); break; }
        snprintf(markerName, sizeof(markerName), NOVA_REMOTE_LOAD_EVENT, pid);
        marker = CreateEventA(NULL, TRUE, FALSE, markerName);
        if (!marker || GetLastError() == ERROR_ALREADY_EXISTS) {
            snprintf(message, messageLength, "[-] Another remote load is pending in this process."); break;
        }
        const SIZE_T pathBytes = (wcslen(absolute) + 1) * sizeof(wchar_t);
        remotePath = VirtualAllocEx(process, NULL, pathBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remotePath || !WriteProcessMemory(process, remotePath, absolute, pathBytes, NULL)) {
            snprintf(message, messageLength, "[-] Could not pass the DLL path (error %lu).", GetLastError()); break;
        }
        thread = CreateRemoteThread(process, NULL, 0, load, remotePath, 0, NULL);
        if (!thread) { snprintf(message, messageLength, "[-] Could not start LoadLibraryW (error %lu).", GetLastError()); break; }
        *started = TRUE;
        if (WaitForSingleObject(thread, 90000) != WAIT_OBJECT_0) {
            snprintf(message, messageLength, "[-] LoadLibraryW did not finish. Restart the target before retrying."); break;
        }
        loaderFinished = TRUE;
        CloseHandle(thread); thread = NULL;
        // A remote thread exit code is only 32 bits. Enumerate the loaded image
        // instead of truncating an x64 HMODULE returned by LoadLibraryW.
        if (!ModuleByName(pid, absolute, TRUE, &loaded)) {
            *started = FALSE;
            snprintf(message, messageLength, "[-] LoadLibraryW failed; check the DLL dependencies and runtime."); break;
        }
        thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)(loaded + entryRva), bootstrap, 0, NULL);
        if (!thread) {
            *started = FALSE;
            snprintf(message, messageLength, "[-] Could not start Nova initialization (error %lu).", GetLastError()); break;
        }
        initialized = TRUE;
        result = thread; thread = NULL;
    } while (0);
    if (!initialized && loaded && loaderFinished) {
        LPTHREAD_START_ROUTINE release = RemoteApi(pid, "FreeLibrary");
        HANDLE cleanup = release ? CreateRemoteThread(process, NULL, 0, release, (void*)loaded, 0, NULL) : NULL;
        if (cleanup) { WaitForSingleObject(cleanup, 5000); CloseHandle(cleanup); }
    }
    if (remotePath && (!*started || loaderFinished)) VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
    if (thread) CloseHandle(thread);
    if (marker) CloseHandle(marker);
    if (local) FreeLibrary(local);
    return result;
}
