#include "cs2/sdk/environment/Environment.h"
#include "cs2/sdk/players/Offsets.h"
#include <windows.h>
#include <cstring>

namespace nova::cs2::sdk::environment {
namespace {
bool Copy(void*, std::uintptr_t address, void* out, std::size_t size) {
    if (address < 0x10000 || address > 0x00007FFFFFFFFFFFULL - size) return false;
    SIZE_T copied = 0;
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(address), out, size, &copied) && copied == size;
}
bool InGameNative() {
    return InGame({nullptr, Copy}, reinterpret_cast<std::uintptr_t>(GetModuleHandleW(L"client.dll")),
        reinterpret_cast<std::uintptr_t>(GetModuleHandleW(L"engine2.dll")));
}
}
bool InGame(const players::Memory& m, std::uintptr_t client, std::uintptr_t engine) {
    namespace p = players::offsets;
    std::uintptr_t network = 0, world = 0, check = 0; int signOn = 0;
    return client && engine && m.read(engine + p::networkClient, network) && network &&
        m.read(network + p::signOnState, signOn) && signOn == 6 &&
        m.read(client + p::entitySystem, world) && world &&
        m.read(engine + p::networkClient, check) && check == network &&
        m.read(network + p::signOnState, signOn) && signOn == 6;
}
Binding ResolveBinding() {
    if (!players::Available()) return {};
    auto* client = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"client.dll"));
    const auto tier = GetModuleHandleW(L"tier0.dll");
    if (!client || !tier) return {};
    // Build14184 main post-processing pass: powf(2, exposureBias).
    // Only this call is adjusted; all other math calls keep their native result.
    constexpr unsigned char call[]{0xF3,0x41,0x0F,0x10,0x8D,0xEC,0,0,0,0xF3,0x0F,0x10,0x05,
        0x22,0xBE,0x8D,0,0xFF,0x15,0xCC,0x32,0x8C,0};
    void* target = nullptr;
    const auto exported = reinterpret_cast<void*>(GetProcAddress(tier, "V_powf"));
    __try {
        if (std::memcmp(client + 0x11E4255, call, sizeof(call))) return {};
        target = *reinterpret_cast<void**>(client + 0x1AA7538);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return {}; }
    if (!target || target != exported) return {};
    return {target, client + 0x11E426C, InGameNative};
}
}
