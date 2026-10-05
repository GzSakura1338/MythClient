#include "cs2/sdk/weapons/Weapons.h"
#include "cs2/sdk/weapons/AimPunch.h"
#include "cs2/sdk/weapons/Offsets.h"
#include "cs2/sdk/players/Offsets.h"
#include "cs2/sdk/players/EntityAccess.h"
#include "cs2/input/AttackButtons.h"
#include "ui/clickgui/ClickGui.h"
#include <windows.h>
#include <cmath>
#include <cstring>
#include <numbers>
#include <atomic>

namespace nova::cs2::sdk::weapons {
namespace {
namespace p = players::offsets;
std::atomic<const char*> fireStatus{"not sampled"};
using Address = std::uintptr_t;
bool Copy(void*, Address address, void* output, std::size_t size) {
    if (address < 0x10000 || address > 0x00007FFFFFFFFFFFULL - size) return false;
    __try { std::memcpy(output, reinterpret_cast<const void*>(address), size); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool Resolve(const players::Memory& memory, Address world, std::uint32_t handle, Address& entity) {
    return players::ResolveEntity(memory, world, handle, entity);
}
bool Held(const players::Memory& memory, Address input, bool revolver, bool& held) {
    held = false;
    // Read the same registered kbutton state as B20960, without calling its
    // side effects (it also marks pressed/released bits in the working sample).
    for (unsigned i = 0; i < 64; ++i) {
        Address button = 0; std::uint64_t mask = 0; std::uint8_t down = 0;
        if (!memory.read(input + offsets::buttons + i * 8, button)) return false;
        if (!button) continue;
        if (!memory.read(button, mask)) return false;
        if (mask != 1 && !(revolver && mask == 2048)) continue;
        if (!memory.read(button + offsets::buttonHeld, down)) return false;
        held |= down != 0;
    }
    return true;
}
}
bool IsFirearm(std::uint16_t item) {
    switch (item) {
    case 1: case 2: case 3: case 4: case 7: case 8: case 9: case 10: case 11:
    case 13: case 14: case 16: case 17: case 19: case 23: case 24: case 25:
    case 26: case 27: case 28: case 29: case 30: case 32: case 33: case 34:
    case 35: case 36: case 38: case 39: case 40: case 60: case 61: case 63: case 64: return true;
    default: return false;
    }
}
bool IsSniper(std::uint16_t item) { return item == 9 || item == 11 || item == 38 || item == 40; }
bool ReadWeapon(const players::Memory& m, Address client, Address engine, Weapon& out) {
    out = {};
    auto& id = out.identity;
    Address controller = 0, pawn = 0, services = 0, servicePawn = 0;
    std::uint32_t weaponOwner = 0;
    int signOn = 0, health = 0; std::uint8_t life = 0, reload = 0, silencer = 0;
    if (!m.read(engine + p::networkClient, id.network) || !id.network ||
        !m.read(id.network + p::signOnState, signOn) || signOn != 6 ||
        !m.read(client + p::entitySystem, id.world) || !id.world ||
        !m.read(client + p::localController, controller) || !controller ||
        !m.read(controller + p::controllerPawn, id.pawnHandle) || !Resolve(m, id.world, id.pawnHandle, id.pawn) ||
        !m.read(client + p::localPawn, pawn) || pawn != id.pawn ||
        !m.read(pawn + p::health, health) || health <= 0 || health > 10000 ||
        !m.read(pawn + p::lifeState, life) || life ||
        !m.read(pawn + offsets::weaponServices, services) || !services ||
        !m.read(services + offsets::activeWeapon, id.weaponHandle) || !Resolve(m, id.world, id.weaponHandle, id.weapon) ||
        !m.read(id.weapon + offsets::owner, weaponOwner) || weaponOwner != id.pawnHandle ||
        !m.read(id.weapon + offsets::itemDefinition, id.item) || !IsFirearm(id.item) ||
        !m.read(pawn + offsets::punchService, out.punchService) || !out.punchService ||
        !m.read(out.punchService + offsets::serviceOwner, servicePawn) || servicePawn != pawn ||
        !m.read(pawn + offsets::shotsFired, out.shots) || out.shots < 0 || out.shots > 100000 ||
        !m.read(id.weapon + offsets::clip, out.clip) || out.clip < 0 || out.clip > 1000 ||
        !m.read(id.weapon + offsets::mode, id.mode) || id.mode < 0 || id.mode > 1 ||
        !m.read(id.weapon + offsets::recoilIndex, out.recoilIndex) || !std::isfinite(out.recoilIndex) || out.recoilIndex < 0 ||
        !m.read(id.weapon + offsets::lastShot, out.lastShot) || !std::isfinite(out.lastShot) ||
        !m.read(id.weapon + offsets::reload, reload) || !m.read(id.weapon + offsets::silencer, silencer)) return false;
    out.reload = reload != 0; id.silencer = silencer != 0;
    int nextTick = 0; float fraction = 0, nextAttack = 0; std::uint8_t scoped = 0;
    if (!m.read(id.weapon + offsets::nextPrimaryTick, nextTick) || nextTick < 0 ||
        !m.read(id.weapon + offsets::nextPrimaryFraction, fraction) || !std::isfinite(fraction) || fraction < 0 || fraction >= 1 ||
        !m.read(services + offsets::nextAttack, nextAttack) || !std::isfinite(nextAttack) ||
        !m.read(pawn + offsets::scoped, scoped) || scoped > 1) return false;
    out.nextAttack = (std::max)((nextTick + static_cast<double>(fraction)) * offsets::interval, static_cast<double>(nextAttack));
    out.scoped = scoped != 0;
    // Shotguns aren't C_CSWeaponBaseGun. Only burst-capable IDs read this field.
    if ((id.item == 4 || id.item == 10) &&
        (!m.read(id.weapon + offsets::burstRemaining, out.burst) || out.burst < 0 || out.burst > 3)) return false;
    Address check = 0;
    std::uint32_t active = 0;
    return Resolve(m, id.world, id.pawnHandle, check) && check == pawn &&
        Resolve(m, id.world, id.weaponHandle, check) && check == id.weapon &&
        m.read(services + offsets::activeWeapon, active) && active == id.weaponHandle &&
        m.read(client + p::entitySystem, check) && check == id.world &&
        m.read(engine + p::networkClient, check) && check == id.network;
}
bool Capture(Angle* angle, Sample& out) {
    out = {};
    DWORD foreground = 0; GetWindowThreadProcessId(GetForegroundWindow(), &foreground);
    if (foreground != GetCurrentProcessId() || clickgui::CapturesInput()) return false;
    const auto client = reinterpret_cast<Address>(GetModuleHandleW(L"client.dll"));
    const auto engine = reinterpret_cast<Address>(GetModuleHandleW(L"engine2.dll"));
    const players::Memory m{nullptr, Copy};
    Address input = 0, samples = 0, globals = 0; int count = 0, suppressed = 0;
    if (!client || !engine || !m.read(client + offsets::inputSuppression, suppressed) || suppressed != 0 ||
        !m.read(client + offsets::inputPointer, input) || !input ||
        !m.read(input + offsets::sampleCount, count) || count < 1 || count > 4096 ||
        !m.read(input + offsets::samples, samples) || !samples ||
        reinterpret_cast<Address>(angle) != samples + (count - 1) * offsets::sampleStride + offsets::sampleAngle ||
        !m.read(client + offsets::globalsPointer, globals) || !globals ||
        !m.read(globals + 4, out.frame) || !m.read(reinterpret_cast<Address>(angle), out.view) ||
        !ReadWeapon(m, client, engine, out.weapon) ||
        !Held(m, input, out.weapon.identity.item == 64, out.attack)) return false;
    out.ordinal = count;
    Time time;
    if (!EvaluatePunch(client, out.weapon.punchService, time, out.punch)) return false;
    out.time = (static_cast<double>(time.tick) + time.fraction) * offsets::interval;
    // Native getters return a scalar float in XMM0. The optional inaccuracy
    // outputs (movement/jump contributions) are null; call on the input thread.
    const float inaccuracy = reinterpret_cast<float (__fastcall*)(Address, float*, float*)>(client + offsets::inaccuracy)(out.weapon.identity.weapon, nullptr, nullptr);
    const float spread = reinterpret_cast<float (__fastcall*)(Address)>(client + offsets::spread)(out.weapon.identity.weapon);
    out.weapon.accuracyValid = std::isfinite(inaccuracy) && std::isfinite(spread) && inaccuracy >= 0 && spread >= 0 && inaccuracy <= 1 && spread <= 1;
    if (out.weapon.accuracyValid) out.weapon.cone = std::atan(inaccuracy + spread) * 180.f / std::numbers::pi_v<float>;
    if (out.weapon.identity.item == 60 || out.weapon.identity.item == 61) {
        float end = 0;
        if (!m.read(out.weapon.identity.weapon + offsets::silencerEnd, end) || !std::isfinite(end)) return false;
        out.weapon.switchingSilencer = end > out.time;
    }
    return true;
}
bool ReadFireState(const players::Memory& m, const Sample& sample, FireState& out) {
    out = {};
    const auto& id = sample.weapon.identity;
    Address data = 0, services = 0, check = 0;
    std::uint32_t active = 0; std::uint8_t full = 0, revolver = 0, wait = 0;
    if (!IsFirearm(id.item) || !players::ResolveEntity(m, id.world, id.pawnHandle, check) || check != id.pawn ||
        !players::ResolveEntity(m, id.world, id.weaponHandle, check) || check != id.weapon ||
        !m.read(id.pawn + offsets::weaponServices, services) || !services ||
        !m.read(services + offsets::activeWeapon, active) || active != id.weaponHandle ||
        !m.read(id.weapon + offsets::weaponData, data) || !data ||
        !m.read(data + offsets::fullAuto, full) || full > 1 ||
        !m.read(data + offsets::revolver, revolver) || revolver > 1 ||
        !m.read(data + offsets::range, out.range) || !std::isfinite(out.range) || out.range <= 0 || out.range > 100000 ||
        !m.read(id.pawn + offsets::waitForNoAttack, wait) || wait > 1) return false;
    out.valid = true; out.allowed = !wait; out.fullAuto = full != 0; out.revolver = revolver != 0;
    return true;
}
namespace {
bool NativeFireAllowed(const players::Memory& m, Address client) {
    Address input = 0, table = 0, gate = 0;
    return client && m.read(client + offsets::inputPointer, input) && input && m.read(input, table) && table &&
        m.read(table + 0xB8, gate) && gate == client + offsets::mouseInputAllowed &&
        reinterpret_cast<bool (__fastcall*)(Address)>(gate)(input);
}
bool CaptureFireState(const Sample& s, FireState& out) {
    const players::Memory m{nullptr, Copy};
    if (!ReadFireState(m, s, out)) { fireStatus.store("invalid VData/weapon identity"); return false; }
    if (!out.allowed) { fireStatus.store("wait for no attack"); return true; }
    out.allowed = NativeFireAllowed(m, reinterpret_cast<Address>(GetModuleHandleW(L"client.dll")));
    fireStatus.store(out.allowed ? "ready" : "mouse input gate");
    return true;
}
AttackResult WriteAttack(void* context, const AttackRequest& request) {
    const auto client = reinterpret_cast<Address>(GetModuleHandleW(L"client.dll"));
    const players::Memory m{nullptr, Copy};
    Address input = 0, samples = 0; int count = 0, suppressed = 0;
    DWORD foreground = 0; GetWindowThreadProcessId(GetForegroundWindow(), &foreground);
    bool down = request.down && foreground == GetCurrentProcessId() && !clickgui::CapturesInput();
    if (!client || !m.read(client + offsets::inputPointer, input) || !input || input != reinterpret_cast<Address>(context) ||
        !m.read(input + offsets::sampleCount, count) || count < 1 || count > 4096 ||
        !m.read(input + offsets::samples, samples) || !samples ||
        !m.read(client + offsets::inputSuppression, suppressed)) return AttackResult::NoSample;
    if (suppressed) down = false;
    if (down && !NativeFireAllowed(m, client)) down = false;
    const auto address = samples + (count - 1) * offsets::sampleStride;
    if (down && reinterpret_cast<Address>(request.angle) != address + offsets::sampleAngle) return AttackResult::StaleSample;
    input::InputSample sample;
    if (!m.read(address, sample)) return AttackResult::NoSample;
    if (!input::MergeAttack(sample, down, request.previous, request.fraction)) return AttackResult::SampleRejected;
    // Only the current native sample is owned; no retained engine/OS button.
    // FinishFrame guards this write and records any actual exception separately.
    std::memcpy(reinterpret_cast<void*>(address), &sample, sizeof(sample));
    return down ? AttackResult::Applied : AttackResult::Released;
}
}
const char* FireStatus() { return fireStatus.load(); }
Binding ResolveBinding() {
    if (!players::Available()) return {};
    auto* client = reinterpret_cast<unsigned char*>(GetModuleHandleW(L"client.dll"));
    if (!client) return {};
    constexpr unsigned char commitBytes[]{0x40,0x53,0x48,0x83,0xEC,0x20,0x8B,0xCA,0x49,0x8B,0xD8};
    constexpr unsigned char callBytes[]{0xE8,0x28,0x3A,0x07,0x00};
    constexpr unsigned char timeBytes[]{0x40,0x56,0x57,0x48,0x83,0xEC,0x28,0x48,0x83,0x79,0x38,0x00};
    constexpr unsigned char punchBytes[]{0x4C,0x8B,0xDC,0x49,0x89,0x5B,0x08,0x49,0x89,0x6B,0x10};
    constexpr unsigned char accuracyBytes[]{0x48,0x89,0x5C,0x24,0x10,0x55,0x56,0x57,0x48,0x81,0xEC,0xB0,0x00,0x00,0x00,0x44};
    constexpr unsigned char spreadBytes[]{0x48,0x63,0x91,0x00,0x1A,0x00,0x00,0x48,0x8B,0x81,0x88,0x03,0x00,0x00};
    constexpr unsigned char frameBytes[]{0x85,0xD2,0x0F,0x85,0x14,0x05,0x00,0x00,0x48,0x8B,0xC4,0x55,0x53,0x41,0x57};
    constexpr unsigned char mouseBytes[]{0x40,0x53,0x48,0x83,0xEC,0x20,0x80,0xB9,0xBC,0x0B,0x00,0x00,0x00,0x48,0x8B,0xD9};
    bool fireMatches = false;
    __try {
        if (std::memcmp(client + offsets::commit, commitBytes, sizeof(commitBytes)) ||
            std::memcmp(client + offsets::commitReturn - 5, callBytes, sizeof(callBytes)) ||
            std::memcmp(client + offsets::punchTime, timeBytes, sizeof(timeBytes)) ||
            std::memcmp(client + offsets::effectivePunch, punchBytes, sizeof(punchBytes)) ||
            std::memcmp(client + offsets::inaccuracy, accuracyBytes, sizeof(accuracyBytes)) ||
            std::memcmp(client + offsets::spread, spreadBytes, sizeof(spreadBytes))) return {};
        // After ProcessInput returns, no consumer has run: the final callback
        // is verified to be a no-op in both CClientInput and CCSGOInput tables.
        Address input = 0, table = 0, finalize = 0, mouse = 0;
        const players::Memory m{nullptr, Copy};
        fireMatches = !std::memcmp(client + offsets::inputFrame, frameBytes, sizeof(frameBytes)) &&
            !std::memcmp(client + offsets::mouseInputAllowed, mouseBytes, sizeof(mouseBytes)) &&
            m.read(reinterpret_cast<Address>(client) + offsets::inputPointer, input) && input &&
            m.read(input, table) && table && m.read(table + 248, finalize) &&
            finalize == reinterpret_cast<Address>(client + offsets::inputFinalize) &&
            client[offsets::inputFinalize] == 0xC2 && !client[offsets::inputFinalize + 1] && !client[offsets::inputFinalize + 2] &&
            m.read(table + 0xB8, mouse) && mouse == reinterpret_cast<Address>(client + offsets::mouseInputAllowed);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return {}; }
    return {client + offsets::commit, client + offsets::commitReturn, Capture,
        fireMatches ? client + offsets::inputFrame : nullptr,
        fireMatches ? CaptureFireState : nullptr, fireMatches ? WriteAttack : nullptr};
}
}
