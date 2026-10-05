
#include "ReflectiveLoader.h"
#include "RuntimeImports.h"
#include "UnwindTable.h"
#include "CppExceptions.h"

extern void InitializeBase(void* parameters);

HINSTANCE hAppInstance = NULL;

static __forceinline FARPROC NovaLoaderExport(HMODULE module, GETPROCADDRESS resolve, DWORD wanted) {
    ULONG_PTR base = (ULONG_PTR)module;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(base + ((PIMAGE_DOS_HEADER)base)->e_lfanew);
    PIMAGE_EXPORT_DIRECTORY exports = (PIMAGE_EXPORT_DIRECTORY)(base +
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
    DWORD* names = (DWORD*)(base + exports->AddressOfNames);
    DWORD i;
    for (i = 0; i < exports->NumberOfNames; ++i) {
        const char* name = (const char*)(base + names[i]);
        if (hash((char*)name) == wanted) return resolve(module, name);
    }
    return NULL;
}

// Raw-file execution cannot reference string literals. Volatile byte stores
// prevent the compiler from folding this message into an unmapped constant.
// Codes: 1=API resolution, 2=allocation, 3=unwind data, 4=registration,
// 5=entry, 6=relocations, 7=C++ exception import.
static __forceinline void NovaLoaderFailure(NovaBootstrap* boot, char code) {
    volatile char* message;
    if (!boot || boot->magic != NOVA_BOOTSTRAP_MAGIC || boot->abi != NOVA_BOOTSTRAP_ABI ||
        boot->size != sizeof(NovaBootstrap)) return;
    message = boot->message;
    message[0]='L'; message[1]='o'; message[2]='a'; message[3]='d'; message[4]='e'; message[5]='r';
    message[6]=' '; message[7]='e'; message[8]='r'; message[9]='r'; message[10]='o'; message[11]='r';
    message[12]=' '; message[13]=code; message[14]=0;
    boot->state = NovaBoot_Failed;
}

#if !defined(_MSC_VER) && !defined(_ReturnAddress)
#define _ReturnAddress() __builtin_return_address(0)
#endif
#pragma intrinsic( _ReturnAddress )

__declspec(noinline) ULONG_PTR caller(VOID) { return (ULONG_PTR) _ReturnAddress(); }

#ifdef REFLECTIVEDLLINJECTION_VIA_LOADREMOTELIBRARYR
DLLEXPORT ULONG_PTR WINAPI ReflectiveLoader(LPVOID lpParameter)
#else
DLLEXPORT ULONG_PTR WINAPI ReflectiveLoader(VOID)
#endif
{

	LOADLIBRARYA pLoadLibraryA = NULL;
	GETPROCADDRESS pGetProcAddress = NULL;
	VIRTUALALLOC pVirtualAlloc = NULL;
	NTFLUSHINSTRUCTIONCACHE pNtFlushInstructionCache = NULL;
	typedef BOOL (WINAPI *VIRTUALFREE)(LPVOID, SIZE_T, DWORD);
	VIRTUALFREE pVirtualFree = NULL;
#ifdef WIN_X64
	typedef BOOLEAN (WINAPI *ADDFUNCTIONTABLE)(PRUNTIME_FUNCTION, DWORD, DWORD64);
	typedef BOOLEAN (WINAPI *DELETEFUNCTIONTABLE)(PRUNTIME_FUNCTION);
	ADDFUNCTIONTABLE pRtlAddFunctionTable = NULL;
	DELETEFUNCTIONTABLE pRtlDeleteFunctionTable = NULL;
	PRUNTIME_FUNCTION functionTable = NULL;
	DWORD functionCount = 0;
#endif
	BOOL attached;
	HMODULE kernelModule = NULL;
	NovaBootstrap* runtimeBootstrap = NULL;
#ifdef REFLECTIVEDLLINJECTION_VIA_LOADREMOTELIBRARYR
	runtimeBootstrap = (NovaBootstrap*)lpParameter;
#endif

	USHORT usCounter;

	ULONG_PTR uiLibraryAddress;

	ULONG_PTR uiBaseAddress;

	ULONG_PTR uiAddressArray;
	ULONG_PTR uiNameArray;
	ULONG_PTR uiExportDir;
	ULONG_PTR uiNameOrdinals;
	DWORD dwHashValue;

	ULONG_PTR uiHeaderValue;
	ULONG_PTR uiValueA;
	ULONG_PTR uiValueB;
	ULONG_PTR uiValueC;
	ULONG_PTR uiValueD;
	ULONG_PTR uiValueE;

	uiLibraryAddress = caller();

	while (TRUE) {
		if (((PIMAGE_DOS_HEADER) uiLibraryAddress)->e_magic == IMAGE_DOS_SIGNATURE) {
			uiHeaderValue = ((PIMAGE_DOS_HEADER) uiLibraryAddress)->e_lfanew;

			if (uiHeaderValue >= sizeof(IMAGE_DOS_HEADER) && uiHeaderValue < 1024) {
				uiHeaderValue += uiLibraryAddress;

				if (((PIMAGE_NT_HEADERS) uiHeaderValue)->Signature == IMAGE_NT_SIGNATURE)
					break;
			}
		}
		uiLibraryAddress--;
	}

#ifdef WIN_X64
	uiBaseAddress = __readgsqword(0x60);
#else
#ifdef WIN_X86
	uiBaseAddress = __readfsdword(0x30);
#else WIN_ARM
	uiBaseAddress = *(DWORD *) ((BYTE *) _MoveFromCoprocessor(15, 0, 13, 0, 2) + 0x30);
#endif
#endif

	uiBaseAddress = (ULONG_PTR) ((_PPEB) uiBaseAddress)->pLdr;

	uiValueA = (ULONG_PTR) ((PPEB_LDR_DATA) uiBaseAddress)->InMemoryOrderModuleList.Flink;
	while (uiValueA) {

		uiValueB = (ULONG_PTR) ((PLDR_DATA_TABLE_ENTRY) uiValueA)->BaseDllName.pBuffer;

		usCounter = ((PLDR_DATA_TABLE_ENTRY) uiValueA)->BaseDllName.Length;

		uiValueC = 0;

		do {
			uiValueC = ror((DWORD) uiValueC);

			if (*((BYTE *) uiValueB) >= 'a')
				uiValueC += *((BYTE *) uiValueB) - 0x20;
			else
				uiValueC += *((BYTE *) uiValueB);
			uiValueB++;
		} while (--usCounter);

		if ((DWORD) uiValueC == KERNEL32DLL_HASH) {

			uiBaseAddress = (ULONG_PTR) ((PLDR_DATA_TABLE_ENTRY) uiValueA)->DllBase;
			kernelModule = (HMODULE)uiBaseAddress;

			uiExportDir = uiBaseAddress + ((PIMAGE_DOS_HEADER) uiBaseAddress)->e_lfanew;

			uiNameArray = (ULONG_PTR) &((PIMAGE_NT_HEADERS) uiExportDir)->OptionalHeader.DataDirectory[
				IMAGE_DIRECTORY_ENTRY_EXPORT];

			uiExportDir = (uiBaseAddress + ((PIMAGE_DATA_DIRECTORY) uiNameArray)->VirtualAddress);

			uiNameArray = (uiBaseAddress + ((PIMAGE_EXPORT_DIRECTORY) uiExportDir)->AddressOfNames);

			uiNameOrdinals = (uiBaseAddress + ((PIMAGE_EXPORT_DIRECTORY) uiExportDir)->AddressOfNameOrdinals);

			usCounter = 3;

			while (usCounter > 0) {

				dwHashValue = hash((char *) (uiBaseAddress + DEREF_32(uiNameArray)));

				if (dwHashValue == LOADLIBRARYA_HASH || dwHashValue == GETPROCADDRESS_HASH || dwHashValue ==
				    VIRTUALALLOC_HASH) {

					uiAddressArray = (uiBaseAddress + ((PIMAGE_EXPORT_DIRECTORY) uiExportDir)->AddressOfFunctions);

					uiAddressArray += (DEREF_16(uiNameOrdinals) * sizeof(DWORD));

					if (dwHashValue == LOADLIBRARYA_HASH)
						pLoadLibraryA = (LOADLIBRARYA) (uiBaseAddress + DEREF_32(uiAddressArray));
					else if (dwHashValue == GETPROCADDRESS_HASH)
						pGetProcAddress = (GETPROCADDRESS) (uiBaseAddress + DEREF_32(uiAddressArray));
					else if (dwHashValue == VIRTUALALLOC_HASH)
						pVirtualAlloc = (VIRTUALALLOC) (uiBaseAddress + DEREF_32(uiAddressArray));

					usCounter--;
				}

				uiNameArray += sizeof(DWORD);

				uiNameOrdinals += sizeof(WORD);
			}
		} else if ((DWORD) uiValueC == NTDLLDLL_HASH) {

			uiBaseAddress = (ULONG_PTR) ((PLDR_DATA_TABLE_ENTRY) uiValueA)->DllBase;

			uiExportDir = uiBaseAddress + ((PIMAGE_DOS_HEADER) uiBaseAddress)->e_lfanew;

			uiNameArray = (ULONG_PTR) &((PIMAGE_NT_HEADERS) uiExportDir)->OptionalHeader.DataDirectory[
				IMAGE_DIRECTORY_ENTRY_EXPORT];

			uiExportDir = (uiBaseAddress + ((PIMAGE_DATA_DIRECTORY) uiNameArray)->VirtualAddress);

			uiNameArray = (uiBaseAddress + ((PIMAGE_EXPORT_DIRECTORY) uiExportDir)->AddressOfNames);

			uiNameOrdinals = (uiBaseAddress + ((PIMAGE_EXPORT_DIRECTORY) uiExportDir)->AddressOfNameOrdinals);

			usCounter = 1;

			while (usCounter > 0) {

				dwHashValue = hash((char *) (uiBaseAddress + DEREF_32(uiNameArray)));

				if (dwHashValue == NTFLUSHINSTRUCTIONCACHE_HASH) {

					uiAddressArray = (uiBaseAddress + ((PIMAGE_EXPORT_DIRECTORY) uiExportDir)->AddressOfFunctions);

					uiAddressArray += (DEREF_16(uiNameOrdinals) * sizeof(DWORD));

					if (dwHashValue == NTFLUSHINSTRUCTIONCACHE_HASH)
						pNtFlushInstructionCache = (NTFLUSHINSTRUCTIONCACHE) (uiBaseAddress + DEREF_32(uiAddressArray));

					usCounter--;
				}

				uiNameArray += sizeof(DWORD);

				uiNameOrdinals += sizeof(WORD);
			}
		}

		if (pLoadLibraryA && pGetProcAddress && pVirtualAlloc && pNtFlushInstructionCache)
			break;

		uiValueA = DEREF(uiValueA);
	}

	pVirtualFree = (VIRTUALFREE)NovaLoaderExport(kernelModule, pGetProcAddress, 0x030633ac);
#ifdef WIN_X64
	pRtlAddFunctionTable = (ADDFUNCTIONTABLE)NovaLoaderExport(kernelModule, pGetProcAddress, 0x22fc1661);
	pRtlDeleteFunctionTable = (DELETEFUNCTIONTABLE)NovaLoaderExport(kernelModule, pGetProcAddress, 0xe0262df4);
	if (!pRtlAddFunctionTable || !pRtlDeleteFunctionTable || !pVirtualFree) {
		NovaLoaderFailure(runtimeBootstrap, '1');
		return 0;
	}
#endif
	uiHeaderValue = uiLibraryAddress + ((PIMAGE_DOS_HEADER) uiLibraryAddress)->e_lfanew;

	uiBaseAddress = (ULONG_PTR) pVirtualAlloc(NULL, ((PIMAGE_NT_HEADERS) uiHeaderValue)->OptionalHeader.SizeOfImage,
	                                          MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!uiBaseAddress) { NovaLoaderFailure(runtimeBootstrap, '2'); return 0; }

	uiValueA = ((PIMAGE_NT_HEADERS) uiHeaderValue)->OptionalHeader.SizeOfHeaders;
	uiValueB = uiLibraryAddress;
	uiValueC = uiBaseAddress;

	while (uiValueA--)
		*(BYTE *) uiValueC++ = *(BYTE *) uiValueB++;

	uiValueA = ((ULONG_PTR) &((PIMAGE_NT_HEADERS) uiHeaderValue)->OptionalHeader + ((PIMAGE_NT_HEADERS) uiHeaderValue)->
	            FileHeader.SizeOfOptionalHeader);

	uiValueE = ((PIMAGE_NT_HEADERS) uiHeaderValue)->FileHeader.NumberOfSections;
	while (uiValueE--) {

		uiValueB = (uiBaseAddress + ((PIMAGE_SECTION_HEADER) uiValueA)->VirtualAddress);

		uiValueC = (uiLibraryAddress + ((PIMAGE_SECTION_HEADER) uiValueA)->PointerToRawData);

		uiValueD = ((PIMAGE_SECTION_HEADER) uiValueA)->SizeOfRawData;

		while (uiValueD--)
			*(BYTE *) uiValueB++ = *(BYTE *) uiValueC++;

		uiValueA += sizeof(IMAGE_SECTION_HEADER);
	}

	uiValueB = (ULONG_PTR) &((PIMAGE_NT_HEADERS) uiHeaderValue)->OptionalHeader.DataDirectory[
		IMAGE_DIRECTORY_ENTRY_IMPORT];

	uiValueC = (uiBaseAddress + ((PIMAGE_DATA_DIRECTORY) uiValueB)->VirtualAddress);

	while (((PIMAGE_IMPORT_DESCRIPTOR) uiValueC)->Name) {

		const char* importName = (LPCSTR)(uiBaseAddress + ((PIMAGE_IMPORT_DESCRIPTOR)uiValueC)->Name);
		uiLibraryAddress = (ULONG_PTR)NovaLoadProfileImport(runtimeBootstrap, importName,
			kernelModule, pLoadLibraryA, pGetProcAddress);
		if (!uiLibraryAddress && NovaUsesSystemRuntime(runtimeBootstrap, importName)) {
			NovaReportRuntimeImportFailure(runtimeBootstrap, importName);
			pVirtualFree((LPVOID)uiBaseAddress, 0, MEM_RELEASE);
			return 0;
		}

		uiValueD = (uiBaseAddress + ((PIMAGE_IMPORT_DESCRIPTOR) uiValueC)->OriginalFirstThunk);

		uiValueA = (uiBaseAddress + ((PIMAGE_IMPORT_DESCRIPTOR) uiValueC)->FirstThunk);

		while (DEREF(uiValueA)) {

			if (uiValueD && ((PIMAGE_THUNK_DATA) uiValueD)->u1.Ordinal & IMAGE_ORDINAL_FLAG) {

				uiExportDir = uiLibraryAddress + ((PIMAGE_DOS_HEADER) uiLibraryAddress)->e_lfanew;

				uiNameArray = (ULONG_PTR) &((PIMAGE_NT_HEADERS) uiExportDir)->OptionalHeader.DataDirectory[
					IMAGE_DIRECTORY_ENTRY_EXPORT];

				uiExportDir = (uiLibraryAddress + ((PIMAGE_DATA_DIRECTORY) uiNameArray)->VirtualAddress);

				uiAddressArray = (uiLibraryAddress + ((PIMAGE_EXPORT_DIRECTORY) uiExportDir)->AddressOfFunctions);

				uiAddressArray += (
					(IMAGE_ORDINAL(((PIMAGE_THUNK_DATA)uiValueD)->u1.Ordinal) - ((PIMAGE_EXPORT_DIRECTORY) uiExportDir)
					 ->Base) * sizeof(DWORD));

				DEREF(uiValueA) = (uiLibraryAddress + DEREF_32(uiAddressArray));
			} else {

				uiValueB = (uiBaseAddress + DEREF(uiValueA));

				DEREF(uiValueA) = (ULONG_PTR) pGetProcAddress((HMODULE) uiLibraryAddress,
				                                              (LPCSTR) ((PIMAGE_IMPORT_BY_NAME) uiValueB)->Name);
#ifdef WIN_X64
				if (hash((char*)((PIMAGE_IMPORT_BY_NAME)uiValueB)->Name) == 0x9d37cb84) { // _CxxThrowException
					ULONG_PTR shim = NovaMappedExport(uiBaseAddress,0x9ab8a999);
					ULONG_PTR original = NovaMappedExport(uiBaseAddress,0xf2d5dbd9);
					if (!shim || !original || !DEREF(uiValueA)) {
						NovaLoaderFailure(runtimeBootstrap,'7');
						pVirtualFree((LPVOID)uiBaseAddress,0,MEM_RELEASE);
						return 0;
					}
					DEREF(original) = DEREF(uiValueA);
					DEREF(uiValueA) = shim;
				}
#endif
				if (!DEREF(uiValueA) && NovaUsesSystemRuntime(runtimeBootstrap, importName)) {
					NovaReportRuntimeImportFailure(runtimeBootstrap, (LPCSTR)((PIMAGE_IMPORT_BY_NAME)uiValueB)->Name);
					pVirtualFree((LPVOID)uiBaseAddress, 0, MEM_RELEASE);
					return 0;
				}
			}

			uiValueA += sizeof(ULONG_PTR);
			if (uiValueD)
				uiValueD += sizeof(ULONG_PTR);
		}

		uiValueC += sizeof(IMAGE_IMPORT_DESCRIPTOR);
	}

	uiLibraryAddress = uiBaseAddress - ((PIMAGE_NT_HEADERS) uiHeaderValue)->OptionalHeader.ImageBase;

	uiValueB = (ULONG_PTR) &((PIMAGE_NT_HEADERS) uiHeaderValue)->OptionalHeader.DataDirectory[
		IMAGE_DIRECTORY_ENTRY_BASERELOC];

	if (((PIMAGE_DATA_DIRECTORY) uiValueB)->Size) {
		SIZE_T imageSize = ((PIMAGE_NT_HEADERS)uiHeaderValue)->OptionalHeader.SizeOfImage;
		SIZE_T remaining = ((PIMAGE_DATA_DIRECTORY)uiValueB)->Size;
		DWORD directoryRva = ((PIMAGE_DATA_DIRECTORY)uiValueB)->VirtualAddress;
		if (directoryRva > imageSize || remaining > imageSize - directoryRva)
			goto invalid_relocations;

		uiValueC = (uiBaseAddress + ((PIMAGE_DATA_DIRECTORY) uiValueB)->VirtualAddress);

		// PE relocation directories are size-delimited, not zero-terminated.
		while (remaining) {
			DWORD blockSize;
			DWORD pageRva;
			if (remaining < sizeof(IMAGE_BASE_RELOCATION)) goto invalid_relocations;
			blockSize = ((PIMAGE_BASE_RELOCATION)uiValueC)->SizeOfBlock;
			pageRva = ((PIMAGE_BASE_RELOCATION)uiValueC)->VirtualAddress;
			if (blockSize < sizeof(IMAGE_BASE_RELOCATION) || blockSize > remaining ||
				(blockSize - sizeof(IMAGE_BASE_RELOCATION)) % sizeof(IMAGE_RELOC) || pageRva > imageSize)
				goto invalid_relocations;

			uiValueA = (uiBaseAddress + ((PIMAGE_BASE_RELOCATION) uiValueC)->VirtualAddress);

			uiValueB = (((PIMAGE_BASE_RELOCATION) uiValueC)->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(
				           IMAGE_RELOC);

			uiValueD = uiValueC + sizeof(IMAGE_BASE_RELOCATION);

			while (uiValueB--) {
				WORD type = ((PIMAGE_RELOC)uiValueD)->type;
				SIZE_T offset = ((PIMAGE_RELOC)uiValueD)->offset;
				SIZE_T width = 0;
				if (type == IMAGE_REL_BASED_DIR64) width = sizeof(ULONG_PTR);
				else if (type == IMAGE_REL_BASED_HIGHLOW) width = sizeof(DWORD);
				else if (type == IMAGE_REL_BASED_HIGH || type == IMAGE_REL_BASED_LOW) width = sizeof(WORD);
#ifdef WIN_ARM
				else if (type == IMAGE_REL_BASED_ARM_MOV32T) width = 2 * sizeof(DWORD);
#endif
				else if (type != IMAGE_REL_BASED_ABSOLUTE) goto invalid_relocations;
				if (width && (offset > imageSize - pageRva || width > imageSize - pageRva - offset))
					goto invalid_relocations;

				if (((PIMAGE_RELOC) uiValueD)->type == IMAGE_REL_BASED_DIR64)
					*(ULONG_PTR *) (uiValueA + ((PIMAGE_RELOC) uiValueD)->offset) += uiLibraryAddress;
				else if (((PIMAGE_RELOC) uiValueD)->type == IMAGE_REL_BASED_HIGHLOW)
					*(DWORD *) (uiValueA + ((PIMAGE_RELOC) uiValueD)->offset) += (DWORD) uiLibraryAddress;
#ifdef WIN_ARM

				else if (((PIMAGE_RELOC) uiValueD)->type == IMAGE_REL_BASED_ARM_MOV32T) {
					register DWORD dwInstruction;
					register DWORD dwAddress;
					register WORD wImm;

					dwInstruction = *(DWORD *) (uiValueA + ((PIMAGE_RELOC) uiValueD)->offset + sizeof(DWORD));

					dwInstruction = MAKELONG(HIWORD(dwInstruction), LOWORD(dwInstruction));

					if ((dwInstruction & ARM_MOV_MASK) == ARM_MOVT) {

						wImm = (WORD) (dwInstruction & 0x000000FF);
						wImm |= (WORD) ((dwInstruction & 0x00007000) >> 4);
						wImm |= (WORD) ((dwInstruction & 0x04000000) >> 15);
						wImm |= (WORD) ((dwInstruction & 0x000F0000) >> 4);

						dwAddress = ((WORD) HIWORD(uiLibraryAddress) + wImm) & 0xFFFF;

						dwInstruction = (DWORD) (dwInstruction & ARM_MOV_MASK2);

						dwInstruction |= (DWORD) (dwAddress & 0x00FF);
						dwInstruction |= (DWORD) (dwAddress & 0x0700) << 4;
						dwInstruction |= (DWORD) (dwAddress & 0x0800) << 15;
						dwInstruction |= (DWORD) (dwAddress & 0xF000) << 4;

						*(DWORD *) (uiValueA + ((PIMAGE_RELOC) uiValueD)->offset + sizeof(DWORD)) = MAKELONG(
							HIWORD(dwInstruction), LOWORD(dwInstruction));
					}
				}
#endif
				else if (((PIMAGE_RELOC) uiValueD)->type == IMAGE_REL_BASED_HIGH)
					*(WORD *) (uiValueA + ((PIMAGE_RELOC) uiValueD)->offset) += HIWORD(uiLibraryAddress);
				else if (((PIMAGE_RELOC) uiValueD)->type == IMAGE_REL_BASED_LOW)
					*(WORD *) (uiValueA + ((PIMAGE_RELOC) uiValueD)->offset) += LOWORD(uiLibraryAddress);

				uiValueD += sizeof(IMAGE_RELOC);
			}

			uiValueC += blockSize;
			remaining -= blockSize;
		}
	}
	goto relocations_complete;
invalid_relocations:
	NovaLoaderFailure(runtimeBootstrap, '6');
	pVirtualFree((LPVOID)uiBaseAddress, 0, MEM_RELEASE);
	return 0;
relocations_complete:

#ifdef WIN_X64
	// Register before the CRT entry point, constructors, or worker threads can
	// execute. Without this Windows cannot find our SEH handlers after mapping.
	functionTable = NovaImageFunctionTable(uiBaseAddress, &functionCount);
	if (!functionTable || !pRtlAddFunctionTable(functionTable, functionCount, (DWORD64)uiBaseAddress)) {
		NovaLoaderFailure(runtimeBootstrap, functionTable ? '4' : '3');
		pVirtualFree((LPVOID)uiBaseAddress, 0, MEM_RELEASE);
		return 0;
	}
#endif
	uiValueA = (uiBaseAddress + ((PIMAGE_NT_HEADERS) uiHeaderValue)->OptionalHeader.AddressOfEntryPoint);

	pNtFlushInstructionCache((HANDLE) -1, NULL, 0);

#ifdef REFLECTIVEDLLINJECTION_VIA_LOADREMOTELIBRARYR

	attached = ((DLLMAIN) uiValueA)((HINSTANCE) uiBaseAddress, DLL_PROCESS_ATTACH, lpParameter);
#else

	attached = ((DLLMAIN) uiValueA)((HINSTANCE) uiBaseAddress, DLL_PROCESS_ATTACH, NULL);
#endif
	if (!attached) {
		NovaLoaderFailure(runtimeBootstrap, '5');
#ifdef WIN_X64
		// Retain memory if removal fails; never leave the OS pointing at freed data.
		if (!pRtlDeleteFunctionTable(functionTable)) return 0;
#endif
		pVirtualFree((LPVOID)uiBaseAddress, 0, MEM_RELEASE);
		return 0;
	}

	return uiValueA;
}

#ifndef REFLECTIVEDLLINJECTION_CUSTOM_DLLMAIN

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpReserved) {
	BOOL bReturnValue = TRUE;
	switch (dwReason) {
		case DLL_QUERY_HMODULE:
			if (lpReserved != NULL)
				*(HMODULE *) lpReserved = hAppInstance;
			break;
		case DLL_PROCESS_ATTACH:
			hAppInstance = hinstDLL;
			InitializeBase(lpReserved);
			break;
		case DLL_PROCESS_DETACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
	}
	return bReturnValue;
}

#endif
