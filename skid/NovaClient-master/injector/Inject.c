

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "LoadLibraryR.h"
#include "RemoteThreadLoad.h"
#include "../shared/Bootstrap.h"

#pragma comment(lib,"Advapi32.lib")

#define BREAK_WITH_ERROR( e ) { snprintf(outMessage, maxLen, "[-] %s. Error=%lu", e, GetLastError() ); bReturnValue = FALSE; break; }

static BOOL Inject(DWORD dwProcessId, const char* cpDllFile, uint32_t client, BOOL remoteThread, char* outMessage, int maxLen)
{
	HANDLE hFile          = NULL;
	HANDLE hModule        = NULL;
	HANDLE hProcess       = NULL;
	HANDLE hToken         = NULL;
	LPVOID lpBuffer       = NULL;
	LPVOID remoteBootstrap = NULL;
	BOOL bootstrapConsumed = FALSE;
	BOOL remoteStarted = FALSE;
	DWORD dwLength        = 0;
	DWORD dwBytesRead     = 0;
	TOKEN_PRIVILEGES priv = {0};
    BOOL bReturnValue     = TRUE;

    if (remoteThread && client != NovaClient_Forge1201) {
        snprintf(outMessage, maxLen, "[-] Remote-thread loading is only available for Forge 1.20.1."); return FALSE;
    }

	do
	{
		hFile = CreateFileA( cpDllFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if( hFile == INVALID_HANDLE_VALUE )
			BREAK_WITH_ERROR( "Failed to open the DLL file" );

		dwLength = GetFileSize( hFile, NULL );
		if( dwLength == INVALID_FILE_SIZE || dwLength == 0 )
			BREAK_WITH_ERROR( "Failed to get the DLL file size" );

		lpBuffer = HeapAlloc( GetProcessHeap(), 0, dwLength );
		if( !lpBuffer )
			BREAK_WITH_ERROR( "Failed to allocate buffer" );

		if( ReadFile( hFile, lpBuffer, dwLength, &dwBytesRead, NULL ) == FALSE )
			BREAK_WITH_ERROR( "Failed to read the DLL file" );

		if( OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
		{
			priv.PrivilegeCount           = 1;
			priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			if( LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &priv.Privileges[0].Luid ) )
				AdjustTokenPrivileges( hToken, FALSE, &priv, 0, NULL, NULL );

			CloseHandle( hToken );
		}

		hProcess = OpenProcess( PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, dwProcessId );
		if( !hProcess )
			BREAK_WITH_ERROR( "Failed to open the target process" );

        {
            USHORT processMachine = 0, nativeMachine = 0;
            if (!IsWow64Process2(hProcess, &processMachine, &nativeMachine) ||
                processMachine != IMAGE_FILE_MACHINE_UNKNOWN || nativeMachine != IMAGE_FILE_MACHINE_AMD64) {
                snprintf(outMessage, maxLen, "[-] Nova requires a Windows x64 JVM.");
                bReturnValue = FALSE;
                break;
            }
        }
        {
            NovaBootstrap boot = {0};
            boot.magic = NOVA_BOOTSTRAP_MAGIC;
            boot.size = sizeof(boot);
            boot.abi = NOVA_BOOTSTRAP_ABI;
            boot.client = client;
            remoteBootstrap = VirtualAllocEx(hProcess, NULL, sizeof(boot), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (!remoteBootstrap || !WriteProcessMemory(hProcess, remoteBootstrap, &boot, sizeof(boot), NULL))
                BREAK_WITH_ERROR("Failed to pass client profile");
        }
        if (remoteThread) {
            hModule = LoadRemoteLibraryThread(hProcess, dwProcessId, cpDllFile, lpBuffer, dwLength,
                remoteBootstrap, &remoteStarted, outMessage, maxLen);
            if (!hModule) { bReturnValue = FALSE; break; }
        } else {
            hModule = LoadRemoteLibraryR(hProcess, lpBuffer, dwLength, remoteBootstrap);
            if (!hModule) BREAK_WITH_ERROR("Failed to inject the DLL");
        }
		remoteStarted = TRUE;

        {
            const ULONGLONG deadline = GetTickCount64() + 90000;
            NovaBootstrap result = {0};
            bReturnValue = FALSE;
            snprintf(outMessage, maxLen, "[-] DLL mapped, but initialization timed out. Check nova-base.log; do not inject again until the instance is unloaded.");
            while (GetTickCount64() < deadline) {
                if (!ReadProcessMemory(hProcess, remoteBootstrap, &result, sizeof(result), NULL)) {
                    snprintf(outMessage, maxLen, "[-] Could not read initialization status (error %lu).", GetLastError());
                    break;
                }
                if (result.state == NovaBoot_Ready || result.state == NovaBoot_Failed) {
                    bootstrapConsumed = TRUE;
                    result.message[sizeof(result.message) - 1] = '\0';
                    bReturnValue = result.state == NovaBoot_Ready;
                    snprintf(outMessage, maxLen, "[%s] PID %lu: %s", bReturnValue ? "+" : "-", dwProcessId, result.message);
                    break;
                }
                Sleep(50);
            }
        }
		CloseHandle(hModule); hModule = NULL;

	} while( 0 );

	if( lpBuffer )
		HeapFree( GetProcessHeap(), 0, lpBuffer );

    if (hFile != INVALID_HANDLE_VALUE && hFile != NULL)
        CloseHandle(hFile);
    if (hModule) CloseHandle(hModule);

	if( hProcess )
	{
        if (remoteBootstrap && (!remoteStarted || bootstrapConsumed))
            VirtualFreeEx(hProcess, remoteBootstrap, 0, MEM_RELEASE);
		CloseHandle( hProcess );
	}

	return bReturnValue;
}

BOOL DoInject(DWORD pid, const char* dll, uint32_t client, char* message, int length) {
    return Inject(pid, dll, client, FALSE, message, length);
}
BOOL DoInjectRemoteThread(DWORD pid, const char* dll, uint32_t client, char* message, int length) {
    return Inject(pid, dll, client, TRUE, message, length);
}
