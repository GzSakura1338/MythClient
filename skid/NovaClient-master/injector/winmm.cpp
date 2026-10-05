#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cwchar>
#include <cstdio>
#include <cstdlib>

// The generated x64 stubs preserve every Windows ABI argument register.
#include "winmm-exports.h"
static INIT_ONCE systemOnce = INIT_ONCE_STATIC_INIT;
static FARPROC systemFunctions[NOVA_WINMM_EXPORT_COUNT]{};
static HMODULE selfModule;

static BOOL CALLBACK ResolveSystem(PINIT_ONCE, PVOID, PVOID*) {
    wchar_t path[MAX_PATH]{};
    UINT length = GetSystemDirectoryW(path, MAX_PATH);
    if (!length || length + 11 >= MAX_PATH) return FALSE;
    wcscat_s(path, L"\\winmm.dll");
    // Absolute system path prevents recursively loading this proxy.
    HMODULE system = LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!system) return FALSE;
    for (unsigned i = 0; i < NOVA_WINMM_EXPORT_COUNT; ++i)
        systemFunctions[i] = GetProcAddress(system, exportNames[i]);
    // Keep the real library alive for the lifetime of its forwarded pointers.
    return TRUE;
}

extern "C" FARPROC NovaResolveWinmm(unsigned index) {
    if (!InitOnceExecuteOnce(&systemOnce, ResolveSystem, nullptr, nullptr) ||
        index >= NOVA_WINMM_EXPORT_COUNT || !systemFunctions[index]) {
        // Returning a null jump target would silently corrupt the caller.
        RaiseFailFastException(nullptr, nullptr, 0);
        return nullptr;
    }
    return systemFunctions[index];
}

static void Log(const wchar_t* folder, const char* message, DWORD error = 0) {
    wchar_t path[32768];
    if (swprintf_s(path, L"%s\\loader-%lu.log", folder, GetCurrentProcessId()) < 0) return;
    HANDLE file = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    char line[256];
    int length = sprintf_s(line, "[%llu] %s (error=%lu)\r\n", GetTickCount64(), message, error);
    DWORD written;
    if (length > 0) WriteFile(file, line, length, &written, nullptr);
    CloseHandle(file);
}

static DWORD WINAPI StartNova(void*) {
    wchar_t host[32768], folder[32768], config[32768], game[32768], cwd[32768], client[256], payload[32768];
    DWORD n = GetModuleFileNameW(nullptr, host, _countof(host));
    if (!n || n >= _countof(host)) return 0;
    const wchar_t* name = wcsrchr(host, L'\\');
    name = name ? name + 1 : host;
    if (_wcsicmp(name, L"java.exe") && _wcsicmp(name, L"javaw.exe")) return 0;
    n = GetModuleFileNameW(selfModule, folder, _countof(folder));
    if (!n || n >= _countof(folder)) return 0;
    wchar_t* slash = wcsrchr(folder, L'\\');
    if (!slash) return 0;
    *slash = 0;
    if (wcscat_s(folder, L"\\nova-heypixel") ||
        swprintf_s(config, L"%s\\loader.ini", folder) < 0) return 0;
    if (GetPrivateProfileIntW(L"Nova", L"Enabled", 0, config) != 1) return 0;
    n = GetPrivateProfileStringW(L"Nova", L"GameDirectory", L"", game, _countof(game), config);
    if (!n || n >= _countof(game) - 1) return 0;
    n = GetCurrentDirectoryW(_countof(cwd), cwd);
    if (!n || n >= _countof(cwd)) return 0;
    // This Java runtime may also be used for other games or launcher utilities.
    if (_wcsicmp(game, cwd)) return 0;
    n = GetPrivateProfileStringW(L"Nova", L"Client", L"", client, _countof(client), config);
    if (!n || n >= _countof(client) - 1 || wcschr(client, L'\\') || wcschr(client, L'/') ||
        wcschr(client, L':') || wcsncmp(client, L"nova-client-proxy-", 18) ||
        n < 4 || _wcsicmp(client + n - 4, L".dll")) return 0;
    if (swprintf_s(payload, L"%s\\%s", folder, client) < 0) return 0;
    Log(folder, "winmm proxy: loading Nova; JVM/game readiness is handled by Nova");
    // DllMain has returned before this thread starts. Never initialize the JVM
    // or load the client while holding our DllMain loader lock.
    HMODULE nova = LoadLibraryExW(payload, nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
                                                 LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    Log(folder, nova ? "Nova image loaded; runtime initialization is asynchronous" : "Nova load failed",
        nova ? 0 : GetLastError());
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        selfModule = instance;
        DisableThreadLibraryCalls(instance);
        HANDLE thread = CreateThread(nullptr, 0, StartNova, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
