#pragma once
#include <windows.h>

// Returns the initialization thread handle. On an indeterminate load timeout,
// keep remote arguments alive rather than freeing memory a thread may still use.
HANDLE LoadRemoteLibraryThread(HANDLE process, DWORD pid, const char* path,
    const void* file, DWORD size, void* bootstrap, BOOL* started,
    char* message, int messageLength);
