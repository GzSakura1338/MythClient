#include "cs2/hooks/FovHook.h"
#include "cs2/hooks/InputHook.h"
#include "core/Instance.h"
#include "core/RuntimeControl.h"
#include "MinHook.h"
#include <windows.h>
#include <atomic>
#include <intrin.h>

namespace nova::cs2::hooks::fov {
namespace {
using Resolve = std::intptr_t (__fastcall*)(void*, void*);
struct State {
    Resolve original = nullptr;
    sdk::camera::Binding binding;
    std::atomic<unsigned> settings{0}, calls{0};
    std::atomic<unsigned> frames{0}, applied{0};
    ULONGLONG nextDiagnostic = 0;
    std::atomic<bool> installed{false}, stopping{false}, faulted{false};
};
State& Get() { return Instance<State>(); }
void Adjust(State& s, void* mode, void* view, unsigned packed) {
    sdk::camera::View data;
    if (!s.binding.captureView(mode, view, data) || !data.fov) return;
    // Setup has converted the 4:3 reference FOV to the viewport aspect.
    // Normalize before applying settings, then restore horizontal projection.
    constexpr double radians = std::numbers::pi / 360.;
    if (!std::isfinite(data.aspect) || data.aspect <= 0) return;
    const float reference = static_cast<float>(std::atan(std::tan(*data.fov * radians) * (4. / 3.) / data.aspect) / radians);
    const float adjusted = cs2::fov::Adjust(reference, data.native.base, data.native.scoped,
        {(packed & 2047u) / 10.f, (packed & 2048u) != 0});
    *data.fov = static_cast<float>(std::atan(std::tan(adjusted * radians) * data.aspect / (4. / 3.)) / radians);
    ++s.applied;
    if ((packed & 4096u) && data.unshakenValid && data.angles) {
        sdk::weapons::Angle offset;
        if (input::ReadVisualOffset(data.pawn, data.pawnHandle, data.unshaken, offset)) {
            data.unshaken.pitch = std::clamp(data.unshaken.pitch + offset.pitch, -89.f, 89.f);
            data.unshaken.yaw = std::remainder(data.unshaken.yaw + offset.yaw, 360.f);
        }
        *data.angles = data.unshaken;
    }
}
void GuardedAdjust(State& s, void* mode, void* view, unsigned packed) {
    __try { Adjust(s, mode, view, packed); }
    __except(EXCEPTION_EXECUTE_HANDLER) { s.faulted.store(true); }
}
std::intptr_t __fastcall OnFov(void* mode, void* view) {
    auto& s = Get();
    s.calls.fetch_add(1);
    ++s.frames;
    std::intptr_t result = 0;
    __try {
        // Keep native interpolation/zoom bookkeeping, then apply to the final
        // pre-projection view after weapon and ClientMode overrides.
        result = s.original(mode, view);
        const auto packed = s.settings.load(std::memory_order_acquire);
        if ((packed & 8192u) && !s.stopping.load() && !s.faulted.load())
            GuardedAdjust(s, mode, view, packed);
    } __finally { s.calls.fetch_sub(1); }
    return result;
}
}
bool Install(sdk::camera::Binding binding) {
    auto& s = Get();
    if (s.binding.target || !binding.target || !binding.captureView) return false;
    s.stopping.store(false); s.faulted.store(false); s.settings.store(0); s.binding = binding;
    s.frames = 0; s.applied = 0; s.nextDiagnostic = 0;
    if (MH_CreateHook(binding.target, reinterpret_cast<void*>(&OnFov), reinterpret_cast<void**>(&s.original)) != MH_OK) {
        s.binding = {}; return false;
    }
    if (MH_EnableHook(binding.target) != MH_OK) {
        MH_RemoveHook(binding.target); s.binding = {}; s.original = nullptr; return false;
    }
    s.installed.store(true); return true;
}
bool Available() { auto& s = Get(); return s.installed.load() && !s.stopping.load() && !s.faulted.load(); }
void Publish(cs2::fov::Settings settings, bool enabled) {
    settings = cs2::fov::Normalize(settings);
    // One coherent UI/camera snapshot; the camera hook never reads properties.
    Get().settings.store(static_cast<unsigned>(std::lround(settings.degrees * 10)) | (settings.preserveZoom ? 2048u : 0u) |
        (settings.removeShake ? 4096u : 0u) | (enabled && Available() ? 8192u : 0u), std::memory_order_release);
    auto& s = Get();
    const auto now = GetTickCount64();
    if (enabled && now >= s.nextDiagnostic) {
        s.nextDiagnostic = now + 5000;
        LogTo("CS2: FOV %.1f; setup frames=%u applied=%u status=%s", settings.degrees,
            s.frames.load(), s.applied.load(), sdk::camera::LastViewStatus());
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
