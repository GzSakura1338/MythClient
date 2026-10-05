

#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../shared/Bootstrap.h"

BOOL DoInject(DWORD dwProcessId, const char* cpDllFile,
              uint32_t client, char* outMessage, int maxLen);
BOOL DoInjectRemoteThread(DWORD pid, const char* dll, uint32_t client, char* message, int length);

static int usage(const char* argv0) {
    fprintf(stderr, "usage: %s <pid> <absolute dll path> <client> [reflective|remote-thread]\nremote-thread: Forge 1.20.1 only\nclients:\n", argv0);
#define NOVA_PROFILE(symbol, value, key, label, game) fprintf(stderr, "  %s: %s\n", key, label);
#include "../shared/ClientProfiles.inc"
#undef NOVA_PROFILE
    return 2;
}

static BOOL is_target_process(DWORD pid, uint32_t client) {
    BOOL found = FALSE;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return FALSE;

    PROCESSENTRY32 entry;
    ZeroMemory(&entry, sizeof(entry));
    entry.dwSize = sizeof(entry);
    if (Process32First(snapshot, &entry)) {
        do {
            if (entry.th32ProcessID == pid) {
                found = client == NovaClient_Cs2 ? _stricmp(entry.szExeFile, "cs2.exe") == 0 :
                        (_stricmp(entry.szExeFile, "java.exe") == 0 ||
                         _stricmp(entry.szExeFile, "javaw.exe") == 0);
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return found;
}

int main(int argc, char** argv) {
    if (argc != 4 && argc != 5) return usage(argv[0]);

    uint32_t client = 0;
#define NOVA_PROFILE(symbol, value, key, label, game) if (strcmp(argv[3], key) == 0) client = value;
#include "../shared/ClientProfiles.inc"
#undef NOVA_PROFILE
    if (!client) return usage(argv[0]);
    BOOL remote = argc == 5 && strcmp(argv[4], "remote-thread") == 0;
    if (argc == 5 && !remote && strcmp(argv[4], "reflective") != 0) return usage(argv[0]);
    if (remote && client != NovaClient_Forge1201) return usage(argv[0]);

    DWORD pid = (DWORD)strtoul(argv[1], NULL, 10);
    const char* dll = argv[2];

    if (!pid) {
        fprintf(stderr, "invalid pid: %s\n", argv[1]);
        return 2;
    }
    if (strlen(dll) < 3 || dll[1] != ':') {
        fprintf(stderr, "path must be absolute (e.g. C:\\dir\\file.dll)\n");
        return 2;
    }
    if (GetFileAttributesA(dll) == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "dll not found: %s\n", dll);
        return 2;
    }
    if (!is_target_process(pid, client)) {
        fprintf(stderr, "pid %lu is not a running %s\n", (unsigned long)pid,
                client == NovaClient_Cs2 ? "cs2.exe" : "java.exe/javaw.exe");
        return 3;
    }

    char msg[1024];
    BOOL ok = remote ? DoInjectRemoteThread(pid, dll, client, msg, (int)sizeof(msg))
                     : DoInject(pid, dll, client, msg, (int)sizeof(msg));
    fprintf(stdout, "%s\n", msg);
    return ok ? 0 : 1;
}
