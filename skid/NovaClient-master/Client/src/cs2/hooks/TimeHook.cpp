#include "cs2/hooks/TimeHook.h"
#include "core/Instance.h"
#include "core/RuntimeControl.h"
#include "MinHook.h"
#include <windows.h>
#include <intrin.h>
#include <atomic>

namespace nova::cs2::hooks::time {
namespace {
using Power = float (__cdecl*)(float, float);
struct State {
    Power original = nullptr;
    sdk::environment::Binding binding;
    std::atomic<unsigned> settings{0}, calls{0};
    std::atomic<unsigned> matched{0}, applied{0};
    ULONGLONG nextDiagnostic = 0;
    std::atomic<bool> installed{false}, stopping{false}, faulted{false};
};
State& Get() { return Instance<State>(); }
bool GuardedInGame(State& s) {
    __try { return s.binding.inGame(); }
    __except(EXCEPTION_EXECUTE_HANDLER) { s.faulted.store(true); return false; }
}
float __cdecl OnPower(float base, float exponent) {
    auto& s = Get();
    s.calls.fetch_add(1);
    float result = 0;
    const bool matching = _ReturnAddress() == s.binding.sourceReturn;
    if (matching) ++s.matched;
    __try {
        result = s.original(base, exponent);
        const auto packed = s.settings.load(std::memory_order_acquire);
        if (matching && base == 2.f && packed && !s.stopping.load() && !s.faulted.load() && GuardedInGame(s)) {
            result = sdk::environment::Adjust(result, packed / 1000.f);
            ++s.applied;
        }
    } __finally { s.calls.fetch_sub(1); }
    return result;
}
}
bool Install(sdk::environment::Binding binding) {
    auto& s = Get();
    if (s.binding.target || !binding.target || !binding.sourceReturn || !binding.inGame) return false;
    s.stopping.store(false); s.faulted.store(false); s.settings.store(0); s.binding = binding;
    s.matched = 0; s.applied = 0; s.nextDiagnostic = 0;
    if (MH_CreateHook(binding.target, reinterpret_cast<void*>(&OnPower), reinterpret_cast<void**>(&s.original)) != MH_OK) {
        s.binding = {}; return false;
    }
    if (MH_EnableHook(binding.target) != MH_OK) {
        MH_RemoveHook(binding.target); s.binding = {}; s.original = nullptr; return false;
    }
    s.installed.store(true); return true;
}
bool Available() { auto& s = Get(); return s.installed.load() && !s.stopping.load() && !s.faulted.load(); }
void Publish(sdk::environment::Settings settings, bool enabled) {
    Get().settings.store(enabled && Available() ? static_cast<unsigned>(std::lround(sdk::environment::Scale(settings) * 1000)) : 0,
        std::memory_order_release);
    auto& s = Get(); const auto now = GetTickCount64();
    if (enabled && now >= s.nextDiagnostic) {
        s.nextDiagnostic = now + 5000;
        LogTo("CS2: Time scale=%.2f; main pass=%u applied=%u", sdk::environment::Scale(settings),
            s.matched.load(), s.applied.load());
    }
}
bool Stop(unsigned timeoutMs) {
    auto& s = Get(); s.stopping.store(true); s.settings.store(0);
    if (!s.binding.target) return true;
    const auto status = MH_DisableHook(s.binding.target);
    if (status != MH_OK && status != MH_ERROR_DISABLED && status != MH_ERROR_NOT_CREATED) return false;
    const auto deadline = GetTickCount64() + timeoutMs;
    while (s.calls.load()) {
        if (GetTickCount64() >= deadline) return false;
        Sleep(1);
    }
    s.installed.store(false); return true;
}
void Finalize() {
    auto& s = Get();
    if (s.binding.target) MH_RemoveHook(s.binding.target);
    s.binding = {}; s.original = nullptr; s.settings.store(0); s.installed.store(false);
}
}
