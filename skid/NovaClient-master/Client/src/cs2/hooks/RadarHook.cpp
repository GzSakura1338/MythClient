#include "cs2/hooks/RadarHook.h"
#include "cs2/sdk/radar/Radar.h"
#include "MinHook.h"
#include <atomic>
#include <windows.h>

namespace nova::cs2::hooks::radar {
namespace {
using Update = void (__fastcall*)(void*);
Update original = nullptr;
void* target = nullptr;
std::atomic<bool> installed{false}, enabled{false}, stopping{false};
std::atomic<unsigned> calls{0};

// Leaf SEH helpers do not swallow errors from the game's own update. No
// player pointers, spotted state, or HUD pointers survive this callback.
bool Override(void* hud, unsigned char& saved) {
    if (!hud) return false;
    __try {
        auto* flags = static_cast<unsigned char*>(hud) + sdk::radar::displayFlags;
        saved = *flags & sdk::radar::showAll;
        *flags |= sdk::radar::showAll;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
void Restore(void* hud, unsigned char saved) {
    __try {
        auto* flags = static_cast<unsigned char*>(hud) + sdk::radar::displayFlags;
        *flags = (*flags & ~sdk::radar::showAll) | saved;
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
}
void __fastcall OnUpdate(void* hud) {
    calls.fetch_add(1);
    unsigned char saved = 0;
    const bool changed = !stopping.load() && enabled.load() && Override(hud, saved);
    __try { original(hud); }
    __finally {
        if (changed) Restore(hud, saved);
        calls.fetch_sub(1);
    }
}
}
bool Install(void* verifiedTarget) {
    if (target || !verifiedTarget) return false;
    stopping.store(false); enabled.store(false);
    if (MH_CreateHook(verifiedTarget, reinterpret_cast<void*>(&OnUpdate), reinterpret_cast<void**>(&original)) != MH_OK) return false;
    target = verifiedTarget;
    if (MH_EnableHook(target) != MH_OK) {
        MH_RemoveHook(target); target = nullptr; original = nullptr; return false;
    }
    installed.store(true);
    return true;
}
bool Available() { return installed.load() && !stopping.load(); }
void SetEnabled(bool value) { enabled.store(value && Available()); }
bool Stop(unsigned timeoutMs) {
    stopping.store(true); enabled.store(false);
    if (!target) return true;
    const auto status = MH_DisableHook(target);
    if (status != MH_OK && status != MH_ERROR_DISABLED && status != MH_ERROR_NOT_CREATED) return false;
    const auto deadline = GetTickCount64() + timeoutMs;
    while (calls.load()) {
        if (GetTickCount64() >= deadline) return false;
        Sleep(1);
    }
    installed.store(false);
    return true;
}
void Finalize() {
    if (target) MH_RemoveHook(target);
    target = nullptr; original = nullptr;
    installed.store(false); enabled.store(false);
}
}
