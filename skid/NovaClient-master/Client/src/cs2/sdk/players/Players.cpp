#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <cstring>
#include "cs2/sdk/players/Players.h"
#include "cs2/sdk/players/Offsets.h"
#include "core/RuntimeControl.h"

namespace nova::cs2::sdk::players {
namespace {
std::uintptr_t clientBase = 0, engineBase = 0;
const char* status = "CS2 player reader is not initialized";
bool Copy(void*, std::uintptr_t address, void* output, std::size_t length) {
    // Leaf SEH boundary: game memory can disappear between frames. Do not put
    // C++ owners inside this boundary and never execute functions read from it.
    if (address < 0x10000 || length > 4096 || address > 0x00007FFFFFFFFFFFULL - length) return false;
    __try { std::memcpy(output, reinterpret_cast<const void*>(address), length); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool Matches(HMODULE module, unsigned expectedSize, const char* expectedHash) {
    Memory memory{nullptr, Copy};
    IMAGE_DOS_HEADER dos{}; IMAGE_NT_HEADERS64 nt{};
    const auto base = reinterpret_cast<std::uintptr_t>(module);
    if (!module || !memory.read(base, dos) || dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew <= 0 || dos.e_lfanew > 4096 ||
        !memory.read(base + dos.e_lfanew, nt) || nt.Signature != IMAGE_NT_SIGNATURE || nt.OptionalHeader.SizeOfImage != expectedSize) return false;
    wchar_t path[32768];
    const auto count = GetModuleFileNameW(module, path, static_cast<DWORD>(std::size(path)));
    if (!count || count >= std::size(path)) return false;
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE algorithm = nullptr; BCRYPT_HASH_HANDLE hash = nullptr;
    bool ok = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0;
    if (ok) ok = BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) >= 0;
    unsigned char buffer[65536], digest[32]{}; DWORD read = 0;
    while (ok) {
        if (!ReadFile(file, buffer, sizeof(buffer), &read, nullptr)) { ok = false; break; }
        if (!read) break;
        ok = BCryptHashData(hash, buffer, read, 0) >= 0;
    }
    if (ok) ok = BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0;
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file);
    constexpr char hex[] = "0123456789ABCDEF";
    for (unsigned i = 0; ok && i < 32; ++i)
        ok = hex[digest[i] >> 4] == expectedHash[i * 2] && hex[digest[i] & 15] == expectedHash[i * 2 + 1];
    return ok;
}
}
bool Initialize() {
    Shutdown();
    const auto client = GetModuleHandleW(L"client.dll"), engine = GetModuleHandleW(L"engine2.dll");
    if (!Matches(client, offsets::clientSize, offsets::clientHash) || !Matches(engine, offsets::engineSize, offsets::engineHash)) {
        status = "2DESP: unsupported CS2 module version; update player offsets";
        LogTo("%s", status); return false;
    }
    int build = 0;
    Memory memory{nullptr, Copy};
    if (!memory.read(reinterpret_cast<std::uintptr_t>(engine) + offsets::buildNumber, build) || build != offsets::build) {
        status = "2DESP: CS2 build number does not match player offsets";
        LogTo("%s", status); return false;
    }
    clientBase = reinterpret_cast<std::uintptr_t>(client); engineBase = reinterpret_cast<std::uintptr_t>(engine);
    status = "Ready";
    LogTo("CS2: player reader ready; build %d, client/engine SHA256 verified", build);
    return true;
}
void Shutdown() { clientBase = engineBase = 0; status = "CS2 player reader is not initialized"; }
bool Available() { return clientBase && engineBase; }
bool MatchesModule(const wchar_t* name, unsigned imageSize, const char* sha256) {
    return Matches(GetModuleHandleW(name), imageSize, sha256);
}
const char* Status() { return status; }
bool Capture(Snapshot& result) {
    result = {};
    return Available() && ReadSnapshot({nullptr, Copy}, clientBase, engineBase, result);
}
}
