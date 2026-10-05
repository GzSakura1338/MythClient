#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
#include "ReflectiveUnwindProbe.h"
#include "core/UnwindTable.h"
#include "MinHook.h"

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "Unwind test failed at %d: %s\n", __LINE__, #x); std::exit(1); } } while (false)
namespace {
using Address = std::uintptr_t;
IMAGE_NT_HEADERS64* Headers(void* image) {
    return reinterpret_cast<IMAGE_NT_HEADERS64*>(static_cast<unsigned char*>(image) +
        static_cast<IMAGE_DOS_HEADER*>(image)->e_lfanew);
}
DWORD RawOffset(void* image, DWORD rva) {
    auto* nt = Headers(image);
    if (rva < nt->OptionalHeader.SizeOfHeaders) return rva;
    auto* sections = IMAGE_FIRST_SECTION(nt);
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i)
        if (rva >= sections[i].VirtualAddress && rva - sections[i].VirtualAddress < sections[i].SizeOfRawData)
            return sections[i].PointerToRawData + rva - sections[i].VirtualAddress;
    CHECK(false); return 0;
}
void* Export(void* image, const char* name, bool raw = false) {
    auto* base = static_cast<unsigned char*>(image);
    auto address = [&](DWORD rva) { return base + (raw ? RawOffset(image, rva) : rva); };
    auto* exports = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(address(
        Headers(image)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress));
    auto* names = reinterpret_cast<DWORD*>(address(exports->AddressOfNames));
    auto* ordinals = reinterpret_cast<WORD*>(address(exports->AddressOfNameOrdinals));
    auto* functions = reinterpret_cast<DWORD*>(address(exports->AddressOfFunctions));
    for (DWORD i = 0; i < exports->NumberOfNames; ++i)
        if (!std::strcmp(reinterpret_cast<const char*>(address(names[i])), name)) return address(functions[ordinals[i]]);
    CHECK(false); return nullptr;
}
bool HasFunction(Address address) { DWORD64 base = 0; return RtlLookupFunctionEntry(address, &base, nullptr) != nullptr; }
bool IsFreed(Address image) {
    MEMORY_BASIC_INFORMATION region{};
    CHECK(VirtualQuery(reinterpret_cast<void*>(image), &region, sizeof(region)));
    return region.State == MEM_FREE;
}
struct RawImage {
    void* data;
    explicit RawImage(const char* path) {
        std::ifstream file(path, std::ios::binary);
        std::vector<char> bytes{std::istreambuf_iterator<char>(file), {}};
        CHECK(bytes.size() > sizeof(IMAGE_DOS_HEADER));
        data = VirtualAlloc(nullptr, bytes.size(), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        CHECK(data); std::memcpy(data, bytes.data(), bytes.size());
    }
    ~RawImage() { CHECK(VirtualFree(data, 0, MEM_RELEASE)); }
    Address Load(UnwindProbeContext& context) {
        auto loader = reinterpret_cast<ULONG_PTR(WINAPI*)(void*)>(Export(data, "ReflectiveLoader", true));
        return loader(&context);
    }
};
void Unload(Address image, DWORD expectedExit) {
    auto entry = reinterpret_cast<LPTHREAD_START_ROUTINE>(Export(reinterpret_cast<void*>(image), "ProbeUnload"));
    HANDLE thread = CreateThread(nullptr, 0, entry, reinterpret_cast<void*>(image), 0, nullptr);
    CHECK(thread && WaitForSingleObject(thread, 10000) == WAIT_OBJECT_0);
    DWORD result = 999; CHECK(GetExitCodeThread(thread, &result) && result == expectedExit); CloseHandle(thread);
}
Address refusedImage = 0;
BOOLEAN WINAPI RejectRegistration(PRUNTIME_FUNCTION, DWORD, DWORD64 image) { refusedImage = image; return FALSE; }
void RegistrationFailure(const char* path) {
    auto target = reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "RtlAddFunctionTable"));
    CHECK(target && MH_Initialize() == MH_OK);
    CHECK(MH_CreateHook(target, reinterpret_cast<void*>(&RejectRegistration), nullptr) == MH_OK);
    CHECK(MH_EnableHook(target) == MH_OK);
    RawImage raw(path); UnwindProbeContext context;
    CHECK(raw.Load(context) == 0);
    CHECK(context.boot.state == NovaBoot_Failed && !std::strcmp(context.boot.message, "Loader error 4"));
    CHECK(!context.image && refusedImage && IsFreed(refusedImage));
    CHECK(MH_DisableHook(target) == MH_OK && MH_RemoveHook(target) == MH_OK && MH_Uninitialize() == MH_OK);
    std::puts("PASS: failed registration rejects startup and releases the image");
}
using AddTable = BOOLEAN (WINAPI*)(PRUNTIME_FUNCTION, DWORD, DWORD64);
AddTable originalAdd = nullptr;
Address productionImage = 0, productionFunction = 0;
BOOLEAN WINAPI ObserveRegistration(PRUNTIME_FUNCTION table, DWORD count, DWORD64 image) {
    const auto result = originalAdd(table, count, image);
    if (result) { productionImage = image; productionFunction = image + table[0].BeginAddress; }
    return result;
}
struct LoadRequest { RawImage* raw; UnwindProbeContext* context; Address result = 0; };
DWORD WINAPI LoadWorker(void* parameter) {
    auto& request = *static_cast<LoadRequest*>(parameter);
    request.result = request.raw->Load(*request.context);
    return 0;
}
void ProductionLifecycle(const char* path) {
    // Runs only in this local host. The CS2 profile must reject our executable
    // before installing hooks, then follow its real failed-startup teardown.
    auto target = reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "RtlAddFunctionTable"));
    CHECK(MH_Initialize() == MH_OK);
    CHECK(MH_CreateHook(target, reinterpret_cast<void*>(&ObserveRegistration), reinterpret_cast<void**>(&originalAdd)) == MH_OK);
    CHECK(MH_EnableHook(target) == MH_OK);
    RawImage raw(path); UnwindProbeContext context;
    LoadRequest request{&raw, &context};
    HANDLE thread = CreateThread(nullptr, 0, LoadWorker, &request, 0, nullptr);
    CHECK(thread && WaitForSingleObject(thread, 10000) == WAIT_OBJECT_0); CloseHandle(thread);
    CHECK(request.result && productionImage);
    CHECK(MH_DisableHook(target) == MH_OK && MH_RemoveHook(target) == MH_OK && MH_Uninitialize() == MH_OK);
    const auto deadline = GetTickCount64() + 10000;
    while (GetTickCount64() < deadline && !IsFreed(productionImage)) Sleep(10);
    CHECK(IsFreed(productionImage) && !HasFunction(productionFunction));
    CHECK(context.boot.state == NovaBoot_Failed && std::strstr(context.boot.message, "requires cs2.exe"));
    std::puts("PASS: production DLL reflective startup, wrong-host rejection, thread drain and self-unload");
}
}
int main(int argc, char** argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    CHECK(argc == 3 || argc == 4);
    for (int mode = 0; mode < 6; ++mode) {
        RawImage raw(argv[1]); UnwindProbeContext context;
        auto* nt = Headers(raw.data);
        auto& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        auto* first = reinterpret_cast<IMAGE_BASE_RELOCATION*>(static_cast<unsigned char*>(raw.data) +
            RawOffset(raw.data, directory.VirtualAddress));
        if (mode == 0) directory.VirtualAddress = nt->OptionalHeader.SizeOfImage - 4;
        if (mode == 1) directory.Size = 4;
        if (mode == 2) first->SizeOfBlock = 6;
        if (mode == 3) first->SizeOfBlock = directory.Size + 2;
        if (mode == 4) first->SizeOfBlock = 9;
        if (mode == 5) {
            first->VirtualAddress = nt->OptionalHeader.SizeOfImage - 1;
            reinterpret_cast<WORD*>(first + 1)[0] = IMAGE_REL_BASED_DIR64 << 12;
        }
        CHECK(raw.Load(context) == 0 && context.boot.state == NovaBoot_Failed && !context.image);
        CHECK(!std::strcmp(context.boot.message, "Loader error 6"));
    }
    std::puts("PASS: invalid relocation directories, blocks and targets reject startup");
    {
        RawImage raw(argv[1]); UnwindProbeContext context;
        auto* nt = Headers(raw.data);
        auto& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        DWORD consumed = 0;
        IMAGE_BASE_RELOCATION* last = nullptr;
        while (consumed < directory.Size) {
            last = reinterpret_cast<IMAGE_BASE_RELOCATION*>(static_cast<unsigned char*>(raw.data) +
                RawOffset(raw.data, directory.VirtualAddress + consumed));
            CHECK(last->SizeOfBlock >= sizeof(*last) && last->SizeOfBlock <= directory.Size - consumed);
            consumed += last->SizeOfBlock;
        }
        auto* sections = IMAGE_FIRST_SECTION(nt);
        auto& section = sections[nt->FileHeader.NumberOfSections - 1];
        CHECK(directory.VirtualAddress >= section.VirtualAddress);
        const DWORD end = nt->OptionalHeader.SizeOfImage;
        const DWORD oldEnd = directory.VirtualAddress + directory.Size;
        CHECK(oldEnd <= end && !(end - oldEnd & 1));
        // Extend the final block with ABSOLUTE padding up to the allocation end.
        // No terminator exists and reading another header crosses the image.
        for (DWORD rva = oldEnd; rva < section.VirtualAddress + section.SizeOfRawData; ++rva)
            static_cast<unsigned char*>(raw.data)[RawOffset(raw.data, rva)] = 0;
        last->SizeOfBlock += end - oldEnd;
        directory.Size = end - directory.VirtualAddress;
        CHECK(raw.Load(context) && context.boot.state == NovaBoot_Ready && context.image);
        auto exercise = reinterpret_cast<unsigned(*)()>(Export(reinterpret_cast<void*>(context.image), "ProbeExercise"));
        CHECK(exercise() == 0);
        Unload(context.image, 0);
        CHECK(IsFreed(context.image));
    }
    std::puts("PASS: relocation table ending at image boundary loads without a terminator");
    RegistrationFailure(argv[1]);
    for (int mode = 0; mode < 4; ++mode) {
        RawImage raw(argv[1]); UnwindProbeContext context;
        auto* nt = Headers(raw.data);
        auto& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
        if (mode == 0) directory = {};
        if (mode == 1) --directory.Size;
        if (mode == 2) directory.VirtualAddress = nt->OptionalHeader.SizeOfImage - 4;
        if (mode == 3) {
            auto* table = reinterpret_cast<PRUNTIME_FUNCTION>(static_cast<unsigned char*>(raw.data) + RawOffset(raw.data, directory.VirtualAddress));
            table[0].EndAddress = table[0].BeginAddress;
        }
        CHECK(raw.Load(context) == 0 && context.boot.state == NovaBoot_Failed && !context.image);
        CHECK(!std::strcmp(context.boot.message, "Loader error 3"));
    }
    std::puts("PASS: missing, truncated, out-of-bounds and invalid unwind metadata reject startup");
    {
        RawImage raw(argv[2]); UnwindProbeContext context;
        CHECK(raw.Load(context) == 0 && context.boot.state == NovaBoot_Failed && context.image);
        CHECK(!std::strcmp(context.boot.message, "Loader error 5"));
        CHECK(IsFreed(context.image) && !HasFunction(context.guardedRead));
    }
    std::puts("PASS: rejected DLL entry removes the registered table before freeing the image");
    for (int cycle = 0; cycle < 16; ++cycle) {
        RawImage raw(argv[1]); UnwindProbeContext context;
        CHECK(raw.Load(context) && context.boot.state == NovaBoot_Ready && context.image);
        CHECK(HasFunction(context.guardedRead));
        const auto exercise = reinterpret_cast<unsigned(*)()>(Export(reinterpret_cast<void*>(context.image), "ProbeExercise"));
        CHECK(exercise() == 0);
        Unload(context.image, 0);
        CHECK(IsFreed(context.image) && !HasFunction(context.guardedRead));
    }
    std::puts("PASS: 16 reflective load/SEH/C++/finally/unload cycles, including constructors, typed catches, RAII, rethrow and worker exceptions");
    {
        RawImage raw(argv[1]); UnwindProbeContext context;
        CHECK(raw.Load(context) && context.boot.state == NovaBoot_Ready);
        DWORD count = 0;
        CHECK(RtlDeleteFunctionTable(NovaImageFunctionTable(context.image, &count)));
        Unload(context.image, 1); // Failed deletion must never free a mapped image.
        CHECK(!IsFreed(context.image));
        CHECK(VirtualFree(reinterpret_cast<void*>(context.image), 0, MEM_RELEASE));
    }
    std::puts("PASS: table-removal failure retains the inert image");
    {
        HMODULE image = LoadLibraryA(argv[1]); CHECK(image);
        auto exercise = reinterpret_cast<unsigned(*)()>(GetProcAddress(image, "ProbeExercise"));
        CHECK(exercise && exercise() == 0 && HasFunction(reinterpret_cast<Address>(exercise)));
        Unload(reinterpret_cast<Address>(image), 0);
        CHECK(IsFreed(reinterpret_cast<Address>(image)) && !HasFunction(reinterpret_cast<Address>(exercise)));
    }
    std::puts("PASS: normal LoadLibrary/FreeLibrary retains Windows-owned unwind handling");
    if (argc == 4) ProductionLifecycle(argv[3]);
    return 0;
}
