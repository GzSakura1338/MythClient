#include "cs2/hooks/InputHook.h"
#include "cs2/input/AimPipeline.h"
#include "bind/PhysicalKey.h"
#include "core/Instance.h"
#include "MinHook.h"
#include <windows.h>
#include <intrin.h>
#include <atomic>
#include <cmath>

namespace nova::cs2::hooks::input {
namespace {
using Angle = sdk::weapons::Angle;
using Commit = std::uintptr_t (__fastcall*)(void*, unsigned, Angle*);
using Frame = void (__fastcall*)(void*, int, float);
struct PublishedFire {
    autofire::Settings settings;
    bool enabled = false, ignoreTeam = false, hold = false;
    int key = 0;
    std::uint64_t generation = 0;
};
struct PublishedAim {
    aim::Settings settings;
    bool enabled = false, ignoreTeam = false, hold = false, onlyClick = false;
    int key = 0;
    std::uint64_t generation = 0;
};
struct State {
    Commit original = nullptr;
    Frame originalFrame = nullptr;
    sdk::weapons::Binding binding;
    cs2::input::AimPipeline pipeline;
    sdk::aim::Capture aimCapture = nullptr;
    sdk::aim::ShotCapture shotCapture = nullptr;
    autofire::Controller fire;
    SRWLOCK fireLock = SRWLOCK_INIT;
    PublishedFire publishedFire;
    std::uint64_t appliedFire = 0;
    std::atomic<bool> fireFaulted{false}, frameInstalled{false};
    std::atomic<const char*> fireStatus{"idle"};
    std::atomic<const char*> fireFault{"none"}, installIssue{"input hook not installed"}, lastDeferred{"none"};
    std::atomic<unsigned> fireFrames{0}, fireSamples{0}, fireTargets{0}, fireWrites{0};
    std::atomic<unsigned> fireDeferred{0}, exceptionCode{0};
    std::atomic<std::uintptr_t> exceptionAddress{0};
    std::atomic<double> fireTime{0}, fireNextAttack{0};
    std::atomic<DWORD> frameOwner{0};
    unsigned frameDepth = 0;
    bool autoHeld = false, releasePending = false;
    struct Pending {
        Angle* angle = nullptr;
        float fraction = 0;
        std::uint64_t generation = 0;
        bool down = false, valid = false;
    } pending;
    std::atomic<std::uint64_t> settings{0};
    SRWLOCK aimLock = SRWLOCK_INIT;
    PublishedAim published;
    SRWLOCK visualLock = SRWLOCK_INIT;
    struct Visual {
        std::uintptr_t pawn = 0; std::uint32_t handle = 0;
        Angle raw, offset;
        std::uint64_t stamp = 0, aimGeneration = 0, recoilSettings = 0;
    } visual;
    std::atomic<bool> aimFaulted{false};
    std::atomic<bool> installed{false}, stopping{false}, faulted{false};
    std::atomic<unsigned> calls{0};
    std::atomic_flag processing = ATOMIC_FLAG_INIT;
    std::uint64_t appliedSettings = 0;
    std::uint64_t appliedAim = 0;
};
State& Get() { return Instance<State>(); }
void WriteVisual(State& s, State::Visual value) {
    AcquireSRWLockExclusive(&s.visualLock); s.visual = value; ReleaseSRWLockExclusive(&s.visualLock);
}
PublishedAim ReadAim(State& s) {
    AcquireSRWLockShared(&s.aimLock);
    const auto value = s.published;
    ReleaseSRWLockShared(&s.aimLock);
    return value;
}
PublishedFire ReadFire(State& s) {
    AcquireSRWLockShared(&s.fireLock); const auto value = s.publishedFire; ReleaseSRWLockShared(&s.fireLock); return value;
}
bool Active(const PublishedFire& p) { return p.enabled && (!p.hold || bind::IsPhysicalBindingDown(p.key)); }
int RecordException(State& s, const char* stage, EXCEPTION_POINTERS* exception) {
    s.fireFault.store(stage);
    s.exceptionCode.store(exception->ExceptionRecord->ExceptionCode);
    s.exceptionAddress.store(reinterpret_cast<std::uintptr_t>(exception->ExceptionRecord->ExceptionAddress));
    return EXCEPTION_EXECUTE_HANDLER;
}
bool DecideFire(State& s, const PublishedFire& p, const sdk::weapons::Sample& sample, Angle view) {
    __try {
        sdk::weapons::FireState weapon;
        std::uint32_t target = 0;
        s.fireTime.store(sample.time); s.fireNextAttack.store(sample.weapon.nextAttack);
        if (!s.binding.fireState(sample, weapon) || !weapon.valid) { s.fireStatus.store("weapon data"); s.fire.Reset(); return false; }
        if (!weapon.allowed) { s.fireStatus.store("native input blocked"); s.fire.Reset(); return false; }
        if (!s.shotCapture(sample, view, weapon.range, p.ignoreTeam, target)) { s.fireStatus.store("ray unavailable"); s.fire.Reset(); return false; }
        if (target) s.fireTargets.fetch_add(1);
        const bool down = s.fire.Process(sample, weapon, target);
        s.fireStatus.store(down ? "request" : !target ? "no target" : sample.time < sample.weapon.nextAttack ? "cooldown" : "delay/scope/release");
        return down;
    } __except(RecordException(s, "fire capture exception", GetExceptionInformation())) {
        s.fireStatus.store("capture exception"); s.fireFaulted.store(true); s.fire.Reset(); return false;
    }
}
bool CaptureAim(State& s, const sdk::weapons::Sample& sample, sdk::aim::Query query, sdk::aim::Snapshot& snapshot) {
    __try { return s.aimCapture(sample, query, snapshot); }
    __except(EXCEPTION_EXECUTE_HANDLER) { s.aimFaulted.store(true); snapshot = {}; return false; }
}
bool Apply(State& s, Angle* angle) {
    const auto packed = s.settings.load(std::memory_order_acquire);
    const auto config = ReadAim(s);
    const auto fireConfig = ReadFire(s);
    const bool inFrame = s.frameOwner.load() == GetCurrentThreadId() && s.frameDepth == 1;
    const bool firing = inFrame && s.frameInstalled.load() && !s.fireFaulted.load() && Active(fireConfig);
    if (s.appliedFire != fireConfig.generation) { s.fire.Reset(); s.appliedFire = fireConfig.generation; }
    if (!firing) s.fire.Reset();
    s.fire.Configure(fireConfig.settings);
    if (packed != s.appliedSettings) {
        s.pipeline.Reset();
        s.appliedSettings = packed;
    }
    if (config.generation != s.appliedAim) { s.pipeline.ResetAim(); s.appliedAim = config.generation; }
    const auto& aimSettings = config.settings;
    const bool rcs = (packed & (1ULL << 25)) != 0;
    const bool assist = config.enabled && s.aimCapture && !s.aimFaulted.load() &&
        (!config.hold || bind::IsPhysicalBindingDown(config.key)) &&
        (!config.onlyClick || bind::IsPhysicalBindingDown(-1));
    s.pipeline.Configure({static_cast<unsigned>(packed & 127), static_cast<unsigned>((packed >> 7) & 127),
        static_cast<unsigned>((packed >> 14) & 15), static_cast<unsigned>((packed >> 18) & 127)}, rcs, aimSettings, assist);
    if (!rcs && !assist && !firing) { s.pipeline.Reset(); WriteVisual(s, {}); return true; }
    sdk::weapons::Sample sample;
    if (!s.binding.capture(angle, sample)) { if (firing) s.fireStatus.store("input sample unavailable"); s.pipeline.Reset(); s.fire.Reset(); WriteVisual(s, {}); return true; }
    if (firing) s.fireSamples.fetch_add(1);
    sample.attack = sample.attack || s.autoHeld;
    struct Targets { State& state; const PublishedAim& config; } targets{s, config};
    const Angle result = s.pipeline.Process(sample, &targets, [](void* context, const sdk::weapons::Sample& source,
        Angle reference, sdk::aim::Snapshot& snapshot) {
        const auto& t = *static_cast<Targets*>(context);
        const auto& settings = t.config.settings;
        return CaptureAim(t.state, source, {settings.point, t.config.ignoreTeam, t.state.pipeline.Target(),
            settings.fov, reference}, snapshot);
    });
    *angle = result;
    WriteVisual(s, {sample.weapon.identity.pawn, sample.weapon.identity.pawnHandle, result, s.pipeline.VisualOffset(),
        GetTickCount64(), config.generation, packed});
    if (firing) {
        const double ticks = sample.time * 64.;
        const auto fraction = (std::min)(static_cast<float>(ticks - std::floor(ticks)), std::nextafter(1.f, 0.f));
        s.pending = {angle, fraction, fireConfig.generation, DecideFire(s, fireConfig, sample, result), true};
    }
    return true;
}
bool GuardedApply(State& s, Angle* angle) {
    __try { return Apply(s, angle); }
    __except(RecordException(s, "shared input exception", GetExceptionInformation())) { return false; }
}
std::uintptr_t __fastcall OnCommit(void* context, unsigned slot, Angle* angle) {
    auto& s = Get();
    s.calls.fetch_add(1);
    const bool matching = _ReturnAddress() == s.binding.sourceReturn && slot == 0;
    bool acquired = false;
    std::uintptr_t result = 0;
    __try {
        if (matching && !s.stopping.load() && !s.faulted.load()) {
            const bool owned = s.frameOwner.load() == GetCurrentThreadId();
            if (!owned) acquired = !s.processing.test_and_set(std::memory_order_acquire);
            if ((acquired || (owned && s.frameDepth == 1)) && !GuardedApply(s, angle)) s.faulted.store(true);
        }
        // Same working angle is consumed by native Pawn eye angles and later
        // copied into frame history. Other callers of this helper pass through.
        result = s.original(context, slot, angle);
    } __finally {
        if (acquired) s.processing.clear(std::memory_order_release);
        s.calls.fetch_sub(1);
    }
    return result;
}
void FinishFrame(State& s, void* input) {
    const auto config = ReadFire(s);
    const bool down = !s.releasePending && s.pending.valid && s.pending.down && !s.stopping.load() && !s.faulted.load() &&
        !s.fireFaulted.load() && config.generation == s.pending.generation && Active(config);
    const bool previous = s.autoHeld || s.releasePending;
    if (!down && !previous) return;
    using Result = sdk::weapons::AttackResult;
    Result result = Result::WriteFault;
    __try { result = s.binding.attack(input, {s.pending.angle, s.pending.fraction, down, previous}); }
    __except(RecordException(s, "input write exception", GetExceptionInformation())) {}
    const char* status = sdk::weapons::AttackStatus(result);
    s.fireStatus.store(status);
    if (result == Result::WriteFault) {
        s.fireFault.store(status); s.fireFaulted.store(true); s.fire.Reset();
        s.autoHeld = false; s.releasePending = false;
    } else if (result != Result::Applied && result != Result::Released) {
        s.lastDeferred.store(status); s.fireDeferred.fetch_add(1); s.fire.Reset();
        // Nothing was written. Don't reuse a pending press or tell RCS it fired.
        // If we previously held, finish the release in a fresh sample before rearming.
        s.autoHeld = false; s.releasePending = previous;
    } else {
        s.autoHeld = result == Result::Applied; s.releasePending = false;
        if (s.autoHeld) s.fireWrites.fetch_add(1);
    }
    if (!s.autoHeld) s.fire.Reset();
}
void __fastcall OnFrame(void* input, int slot, float time) {
    auto& s = Get(); s.calls.fetch_add(1);
    const bool nested = s.frameOwner.load() == GetCurrentThreadId();
    bool acquired = false;
    if (nested) ++s.frameDepth;
    else if (slot == 0 && !s.stopping.load()) acquired = !s.processing.test_and_set(std::memory_order_acquire);
    if (acquired) {
        s.fireFrames.fetch_add(1);
        s.pending = {}; s.frameDepth = 1;
        s.frameOwner.store(GetCurrentThreadId());
    }
    __try {
        s.originalFrame(input, slot, time);
        if (acquired) FinishFrame(s, input);
    } __finally {
        if (nested) --s.frameDepth;
        if (acquired) {
            s.pending = {}; s.frameDepth = 0; s.frameOwner.store(0);
            s.processing.clear(std::memory_order_release);
        }
        s.calls.fetch_sub(1);
    }
}
}
bool Install(sdk::weapons::Binding binding, sdk::aim::Capture aimCapture, sdk::aim::ShotCapture shotCapture) {
    auto& s = Get();
    if (s.binding.target) return false;
    s.stopping.store(false); s.faulted.store(false); s.installed.store(false); s.settings.store(0); s.appliedSettings = 0;
    s.pipeline.Reset(); s.processing.clear();
    s.aimCapture = aimCapture; s.aimFaulted.store(false); s.appliedAim = 0;
    s.shotCapture = shotCapture; s.fireFaulted.store(false); s.appliedFire = 0; s.fire.Reset();
    s.fireStatus.store("idle"); s.fireFrames.store(0); s.fireSamples.store(0); s.fireTargets.store(0); s.fireWrites.store(0);
    s.fireTime.store(0); s.fireNextAttack.store(0);
    s.fireFault.store("none"); s.lastDeferred.store("none"); s.fireDeferred.store(0); s.exceptionCode.store(0); s.exceptionAddress.store(0);
    s.installIssue.store("input hook initializing");
    s.frameOwner.store(0); s.frameDepth = 0; s.autoHeld = false; s.releasePending = false; s.pending = {}; s.frameInstalled.store(false);
    AcquireSRWLockExclusive(&s.fireLock); s.publishedFire = {}; ReleaseSRWLockExclusive(&s.fireLock);
    AcquireSRWLockExclusive(&s.aimLock); s.published = {}; ReleaseSRWLockExclusive(&s.aimLock);
    if (!binding.target || !binding.sourceReturn || !binding.capture) {
        s.installIssue.store("shared input binding unavailable"); return false;
    }
    s.binding = binding;
    if (MH_CreateHook(binding.target, reinterpret_cast<void*>(&OnCommit), reinterpret_cast<void**>(&s.original)) != MH_OK) {
        s.installIssue.store("shared input hook creation failed"); s.binding = {}; return false;
    }
    if (MH_EnableHook(binding.target) != MH_OK) {
        s.installIssue.store("shared input hook activation failed");
        MH_RemoveHook(binding.target); s.binding = {}; s.original = nullptr; return false;
    }
    if (!binding.frameTarget || !binding.fireState || !binding.attack) s.installIssue.store("fire input binding unavailable");
    else if (!shotCapture) s.installIssue.store("player trace binding unavailable");
    else if (MH_CreateHook(binding.frameTarget, reinterpret_cast<void*>(&OnFrame), reinterpret_cast<void**>(&s.originalFrame)) != MH_OK)
        s.installIssue.store("fire input hook creation failed");
    else {
        if (MH_EnableHook(binding.frameTarget) == MH_OK) { s.frameInstalled.store(true); s.installIssue.store(""); }
        else { s.installIssue.store("fire input hook activation failed"); MH_RemoveHook(binding.frameTarget); s.originalFrame = nullptr; }
    }
    s.installed.store(true);
    return true;
}
bool Available() { auto& s = Get(); return s.installed.load() && !s.stopping.load() && !s.faulted.load(); }
bool AimAvailable() { auto& s = Get(); return Available() && s.aimCapture && !s.aimFaulted.load(); }
bool FireAvailable() { auto& s = Get(); return Available() && s.frameInstalled.load() && !s.fireFaulted.load(); }
const char* FireUnavailableReason() {
    auto& s = Get();
    if (s.stopping.load()) return "input hook stopping";
    if (s.faulted.load() || s.fireFaulted.load()) return s.fireFault.load();
    if (!s.installed.load() || !s.frameInstalled.load()) return s.installIssue.load();
    return "";
}
FireDebug ReadFireDebug() {
    auto& s = Get(); return {s.fireStatus.load(), s.lastDeferred.load(), s.fireFrames.load(), s.fireSamples.load(), s.fireTargets.load(), s.fireWrites.load(),
        s.fireDeferred.load(), s.exceptionCode.load(), s.exceptionAddress.load(), s.fireTime.load(), s.fireNextAttack.load()};
}
bool ReadVisualOffset(std::uintptr_t pawn, std::uint32_t handle, Angle raw, Angle& offset) {
    offset = {}; auto& s = Get();
    if (!Available()) return false;
    AcquireSRWLockShared(&s.visualLock); const auto v = s.visual; ReleaseSRWLockShared(&s.visualLock);
    if (!pawn || v.pawn != pawn || v.handle != handle || GetTickCount64() - v.stamp > 100 ||
        v.aimGeneration != ReadAim(s).generation || v.recoilSettings != s.settings.load() ||
        raw.pitch != v.raw.pitch || raw.yaw != v.raw.yaw || raw.roll != v.raw.roll) return false;
    offset = v.offset; return true;
}
void Publish(recoil::Settings settings, bool enabled) {
    auto& s = Get(); settings = recoil::Normalize(settings);
    const std::uint64_t value = settings.horizontal | (static_cast<std::uint64_t>(settings.vertical) << 7) |
        (static_cast<std::uint64_t>(settings.startBullet) << 14) | (static_cast<std::uint64_t>(settings.smoothMs) << 18) |
        (static_cast<std::uint64_t>(enabled && Available()) << 25);
    // Single atomic word: no mixed UI/input settings, including rapid off/on
    // transitions between input callbacks. High bits identify every change.
    auto previous = s.settings.load();
    while ((previous & 0xFFFFFFFFULL) != value && !s.settings.compare_exchange_weak(previous,
        (((previous >> 32) + 1) << 32) | value)) {}
}
void PublishAim(aim::Settings settings, bool enabled, bool ignoreTeam, bool hold, int key, bool onlyClick) {
    auto& s = Get(); settings = aim::Normalize(settings);
    if (key < -5 || key > 255) key = 0;
    enabled = enabled && AimAvailable();
    // Publish a coherent snapshot; release the lock before any engine calls.
    AcquireSRWLockExclusive(&s.aimLock);
    const auto& p = s.published;
    if (p.settings != settings || p.enabled != enabled || p.ignoreTeam != ignoreTeam ||
        p.hold != hold || p.key != key || p.onlyClick != onlyClick)
        s.published = {settings, enabled, ignoreTeam, hold, onlyClick, key, p.generation + 1};
    ReleaseSRWLockExclusive(&s.aimLock);
}
void PublishFire(autofire::Settings settings, bool enabled, bool ignoreTeam, bool hold, int key) {
    auto& s = Get(); settings = autofire::Normalize(settings);
    if (key < -5 || key > 255) key = 0;
    enabled = enabled && FireAvailable();
    AcquireSRWLockExclusive(&s.fireLock);
    const auto& p = s.publishedFire;
    if (p.settings != settings || p.enabled != enabled || p.ignoreTeam != ignoreTeam || p.hold != hold || p.key != key)
        s.publishedFire = {settings, enabled, ignoreTeam, hold, key, p.generation + 1};
    ReleaseSRWLockExclusive(&s.fireLock);
}
bool Stop(unsigned timeoutMs) {
    auto& s = Get(); s.stopping.store(true);
    if (!s.binding.target) return true;
    if (s.originalFrame) {
        const auto status = MH_DisableHook(s.binding.frameTarget);
        if (status != MH_OK && status != MH_ERROR_DISABLED && status != MH_ERROR_NOT_CREATED) return false;
    }
    const auto status = MH_DisableHook(s.binding.target);
    if (status != MH_OK && status != MH_ERROR_DISABLED && status != MH_ERROR_NOT_CREATED) return false;
    const auto deadline = GetTickCount64() + timeoutMs;
    while (s.calls.load()) {
        if (GetTickCount64() >= deadline) return false;
        Sleep(1);
    }
    s.installed.store(false); s.frameInstalled.store(false); return true;
}
void Finalize() {
    auto& s = Get();
    if (s.originalFrame) MH_RemoveHook(s.binding.frameTarget);
    if (s.binding.target) MH_RemoveHook(s.binding.target);
    s.binding = {}; s.original = nullptr; s.settings.store(0); s.installed.store(false);
    s.pipeline.Reset(); s.aimCapture = nullptr;
    s.originalFrame = nullptr; s.shotCapture = nullptr; s.autoHeld = false; s.releasePending = false; s.fire.Reset(); s.frameInstalled.store(false);
    AcquireSRWLockExclusive(&s.fireLock); s.publishedFire = {}; ReleaseSRWLockExclusive(&s.fireLock);
    AcquireSRWLockExclusive(&s.aimLock); s.published = {}; ReleaseSRWLockExclusive(&s.aimLock);
}
}
