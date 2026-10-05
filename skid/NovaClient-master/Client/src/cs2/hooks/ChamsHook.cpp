#include "cs2/hooks/ChamsHook.h"
#include "core/Instance.h"
#include "core/RuntimeControl.h"
#include "MinHook.h"
#include <windows.h>
#include <atomic>

namespace nova::cs2::hooks::chams {
namespace {
using Mesh = sdk::chams::Mesh;
using Draw = std::uintptr_t (__fastcall*)(void*, void*, Mesh*, int, void*, void*, void*);
struct State {
    Draw original = nullptr;
    sdk::chams::Binding binding;
    sdk::chams::Materials materials;
    sdk::chams::Settings settings;
    SRWLOCK lock = SRWLOCK_INIT;
    std::atomic<unsigned> calls{0};
    std::atomic<unsigned> draws{0}, examined{0}, selected{0}, applied{0};
    ULONGLONG nextDiagnostic = 0;
    std::atomic<bool> installed{false}, stopping{false}, faulted{false};
};
State& Get() { return Instance<State>(); }
sdk::chams::Settings Snapshot(State& s) {
    AcquireSRWLockShared(&s.lock);
    const auto value = s.settings;
    ReleaseSRWLockShared(&s.lock);
    return value;
}
struct Saved { void* material; std::uint32_t color; bool selected; };
bool Prepare(State& s, Mesh* mesh, int count, Saved* saved) {
    bool any = false;
    __try {
        for (int i = 0; i < count; ++i) {
            saved[i] = {mesh[i].material, mesh[i].color, s.binding.enemy(mesh[i].scene)};
            ++s.examined;
            if (saved[i].selected) ++s.selected;
            any |= saved[i].selected;
        }
        return any;
    } __except(EXCEPTION_EXECUTE_HANDLER) { s.faulted.store(true); return false; }
}
void Restore(State& s, Mesh* mesh, const Saved* saved, int count) {
    __try {
        for (int i = 0; i < count; ++i) if (saved[i].selected) {
            mesh[i].material = saved[i].material; mesh[i].color = saved[i].color;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) { s.faulted.store(true); }
}
bool Apply(State& s, Mesh* mesh, const Saved* saved, int count, void* material, std::uint32_t color) {
    __try {
        for (int i = 0; i < count; ++i) if (saved[i].selected) {
            mesh[i].material = material; mesh[i].color = color;
            ++s.applied;
        }
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) { s.faulted.store(true); return false; }
}
std::uintptr_t Render(State& s, void* desc, void* dx, Mesh* mesh, int count, void* view, void* layer, void* context,
    sdk::chams::Settings settings) {
    constexpr int limit = 1024;
    Saved saved[limit];
    if (!mesh || count < 1 || count > limit || !Prepare(s, mesh, count, saved))
        return s.original(desc, dx, mesh, count, view, layer, context);
    const int index = settings.illuminate ? 2 : 0;
    std::uintptr_t result = 0;
    __try {
        if (settings.throughWalls && !s.stopping.load() &&
            Apply(s, mesh, saved, count, s.materials.slots[index + 1].data, settings.hidden)) {
            // Only selected runs get the depth-disabled pass, even when the
            // native draw batch mixes enemies, teammates and world objects.
            for (int i = 0; i < count;) {
                if (!saved[i].selected) { ++i; continue; }
                const int first = i;
                while (i < count && saved[i].selected) ++i;
                if (!s.stopping.load()) s.original(desc, dx, mesh + first, i - first, view, layer, context);
            }
        }
        Restore(s, mesh, saved, count);
        if (!s.stopping.load() && !s.faulted.load() &&
            !Apply(s, mesh, saved, count, s.materials.slots[index].data, settings.visible))
            Restore(s, mesh, saved, count);
        result = s.original(desc, dx, mesh, count, view, layer, context);
    } __finally { Restore(s, mesh, saved, count); }
    return result;
}
std::uintptr_t __fastcall OnDraw(void* desc, void* dx, Mesh* mesh, int count, void* view, void* layer, void* context) {
    auto& s = Get(); s.calls.fetch_add(1);
    ++s.draws;
    std::uintptr_t result = 0;
    __try {
        const auto settings = Snapshot(s);
        if (settings.enabled && !s.stopping.load() && !s.faulted.load())
            result = Render(s, desc, dx, mesh, count, view, layer, context, settings);
        else result = s.original(desc, dx, mesh, count, view, layer, context);
    } __finally { s.calls.fetch_sub(1); }
    return result;
}
bool Create(State& s) {
    __try { return s.binding.create(s.materials); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
void Release(State& s) {
    if (!s.binding.release) return;
    __try { s.binding.release(s.materials); }
    __except(EXCEPTION_EXECUTE_HANDLER) { s.faulted.store(true); }
}
}
bool Install(sdk::chams::Binding binding) {
    auto& s = Get();
    if (s.binding.target || !binding.target || !binding.enemy || !binding.create || !binding.release) return false;
    s.stopping.store(false); s.faulted.store(false); s.settings = {}; s.binding = binding;
    s.draws = 0; s.examined = 0; s.selected = 0; s.applied = 0; s.nextDiagnostic = 0;
    bool ready = Create(s);
    for (const auto& material : s.materials.slots) ready &= material.data != nullptr;
    if (!ready) { Release(s); s.materials = {}; s.binding = {}; return false; }
    if (MH_CreateHook(binding.target, reinterpret_cast<void*>(&OnDraw), reinterpret_cast<void**>(&s.original)) != MH_OK) {
        Release(s); s.materials = {}; s.binding = {}; return false;
    }
    if (MH_EnableHook(binding.target) != MH_OK) {
        MH_RemoveHook(binding.target); Release(s); s.materials = {}; s.binding = {}; s.original = nullptr; return false;
    }
    s.installed.store(true); return true;
}
bool Available() { auto& s = Get(); return s.installed.load() && !s.stopping.load() && !s.faulted.load(); }
void Publish(sdk::chams::Settings settings) {
    auto& s = Get(); settings.enabled &= Available();
    AcquireSRWLockExclusive(&s.lock); s.settings = settings; ReleaseSRWLockExclusive(&s.lock);
    const auto now = GetTickCount64();
    if (settings.enabled && now >= s.nextDiagnostic) {
        s.nextDiagnostic = now + 5000;
        LogTo("CS2: Chams draws=%u meshes=%u selected=%u applied=%u status=%s visible=%08X hidden=%08X walls=%d",
            s.draws.load(), s.examined.load(), s.selected.load(), s.applied.load(),
            sdk::chams::LastSelectionStatus(), settings.visible, settings.hidden, settings.throughWalls);
    }
}
bool Stop(unsigned timeoutMs) {
    auto& s = Get(); s.stopping.store(true); Publish({});
    if (!s.binding.target) return true;
    const auto status = MH_DisableHook(s.binding.target);
    if (status != MH_OK && status != MH_ERROR_DISABLED && status != MH_ERROR_NOT_CREATED) return false;
    const auto deadline = GetTickCount64() + timeoutMs;
    while (s.calls.load()) { if (GetTickCount64() >= deadline) return false; Sleep(1); }
    s.installed.store(false); return true;
}
void Finalize() {
    auto& s = Get();
    if (s.binding.target) MH_RemoveHook(s.binding.target);
    Release(s); s.materials = {}; s.binding = {}; s.original = nullptr; s.settings = {}; s.installed.store(false);
}
}
