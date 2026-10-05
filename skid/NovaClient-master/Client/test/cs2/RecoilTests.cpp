#include "cs2/hooks/InputHook.h"
#include "cs2/modules/combat/recoilcontrol/RecoilControl.h"
#include "cs2/sdk/weapons/Offsets.h"
#include "cs2/sdk/players/Offsets.h"
#include "MinHook.h"
#include <windows.h>
#include <intrin.h>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "Recoil test failed at %d: %s\n", __LINE__, #x); std::exit(1); } } while (false)
namespace {
using namespace nova::cs2;
using sdk::weapons::Sample;
using sdk::weapons::Angle;
void Near(float a, float b, float tolerance = .0001f) { CHECK(std::abs(a - b) < tolerance); }
Sample Baseline() {
    Sample s; s.frame = 1; s.ordinal = 1; s.time = 10;
    s.weapon.identity = {1, 2, 3, 4, 5, 6, 7, 0, false};
    s.weapon.clip = 30; s.weapon.lastShot = 9; s.view = {10, 20, 1};
    return s;
}
void Advance(Sample& s, bool shot, float pitch, float yaw = 0, double dt = .01) {
    ++s.frame; s.time += dt; s.punch = {pitch, yaw, 0};
    if (shot) { ++s.weapon.shots; --s.weapon.clip; ++s.weapon.recoilIndex; s.weapon.lastShot = static_cast<float>(s.time); }
}
void ControllerTests() {
    recoil::Controller c;
    auto s = Baseline(); c.Process(s);
    s.attack = true; Advance(s, true, 1); s.view = c.Process(s); Near(s.view.pitch, 10);
    Advance(s, true, 3, 2); s.view = c.Process(s); Near(s.view.pitch, 8); Near(s.view.yaw, 18);
    const auto repeated = c.Process(s); Near(repeated.pitch, 8); Near(repeated.yaw, 18);
    const auto second = s;
    // Preserve independently applied mouse motion, including roll.
    s.view.pitch += 5; s.view.yaw -= 7;
    Advance(s, false, 4, 3); s.view = c.Process(s); Near(s.view.pitch, 12); Near(s.view.yaw, 10); Near(s.view.roll, 1);
    auto replay = second; replay.view = {10, 20, 1};
    auto replayed = c.Process(replay); Near(replayed.pitch, 8); Near(replayed.yaw, 18);
    replay.view = replayed; Advance(replay, false, 4, 3); replay.view.pitch += 5; replay.view.yaw -= 7;
    replayed = c.Process(replay); Near(replayed.pitch, 12); Near(replayed.yaw, 10);
    s = replay; s.view = replayed; s.attack = false;
    Advance(s, false, 2); s.view = c.Process(s); Near(s.view.pitch, 12); // no return-to-start
    Advance(s, false, 6); Near(c.Process(s).pitch, 12); // idle landing doesn't compensate
    // Start=1, axis ratios, shortest yaw, final empty shot, queued burst.
    c.Configure({50, 100, 1, 0}); s = Baseline(); s.punch.yaw = 179; c.Process(s);
    s.weapon.clip = 1; Advance(s, true, 2, -179); s.view = c.Process(s);
    Near(s.view.pitch, 8); Near(s.view.yaw, 19); CHECK(s.weapon.clip == 0);
    Advance(s, false, 3, -178); Near(c.Process(s).pitch, 8);
    c.Reset(); s = Baseline(); c.Process(s); s.weapon.burst = 2;
    Advance(s, true, 1); s.view = c.Process(s); Near(s.view.pitch, 9);
    Advance(s, false, 2); s.view = c.Process(s); Near(s.view.pitch, 8);
    // R8 preparation / held trigger without a shot stays passive.
    c.Reset(); s = Baseline(); s.weapon.identity.item = 64; c.Process(s); s.attack = true;
    Advance(s, false, 5); Near(c.Process(s).pitch, 10);
    // Parameter change / identity / time rewind establish a fresh baseline.
    Advance(s, true, 6); s.view = c.Process(s); Near(s.view.pitch, 9);
    c.Configure({0, 0, 1, 0}); Advance(s, true, 8); Near(c.Process(s).pitch, 9);
    c.Configure({100, 100, 1, 0}); Advance(s, false, 9); Near(c.Process(s).pitch, 9);
    ++s.weapon.identity.weaponHandle; Advance(s, true, 10); Near(c.Process(s).pitch, 9);
    s.time -= 1; ++s.frame; Near(c.Process(s).pitch, 9);
    s.time = std::numeric_limits<double>::quiet_NaN(); Near(c.Process(s).pitch, 9);
    // Time-constant smoothing is invariant to subdivision for a fixed target.
    auto filtered = [](int steps) {
        recoil::Controller controller; controller.Configure({100, 100, 1, 50});
        auto sample = Baseline(); controller.Process(sample); sample.attack = true;
        for (int i = 0; i < steps; ++i) {
            Advance(sample, i == 0, 10, 0, .1 / steps); sample.view = controller.Process(sample);
        }
        return sample.view.pitch;
    };
    Near(filtered(10), filtered(40), .0002f); Near(filtered(10), 10 * std::exp(-2.f));
    c.Configure({100, 100, 1, 50}); s = Baseline(); c.Process(s); s.attack = true;
    Advance(s, true, 5, 0, 0); Near(c.Process(s).pitch, 10); // zero dt
    // Pitch limit feedback and disabled-axis preservation.
    c.Configure({0, 100, 1, 0}); s = Baseline(); s.view.pitch = -88; c.Process(s); s.attack = true;
    Advance(s, true, 5, 2); s.view = c.Process(s); Near(s.view.pitch, -89); Near(s.view.yaw, 20);
    s.view.pitch = -80; Advance(s, false, 5, 2); s.view = c.Process(s); Near(s.view.pitch, -80);
    c.Reset(); // Standalone no-target controller; no Team or ESP dependency.
}
struct Memory {
    std::unordered_map<std::uintptr_t, unsigned char> bytes;
    template<class T> void Put(std::uintptr_t address, T value) {
        const auto* data = reinterpret_cast<const unsigned char*>(&value);
        for (unsigned i = 0; i < sizeof(T); ++i) bytes[address + i] = data[i];
    }
    sdk::players::Memory Reader() {
        return {this, [](void* context, std::uintptr_t address, void* out, std::size_t size) {
            auto& bytes = static_cast<Memory*>(context)->bytes;
            for (std::size_t i = 0; i < size; ++i) {
                auto found = bytes.find(address + i); if (found == bytes.end()) return false;
                static_cast<unsigned char*>(out)[i] = found->second;
            }
            return true;
        }};
    }
};
void WeaponTests() {
    namespace p = sdk::players::offsets; namespace w = sdk::weapons::offsets;
    Memory m;
    constexpr std::uintptr_t client = 0x10000000, engine = 0x20000000, network = 0x30000000, world = 0x40000000;
    constexpr std::uintptr_t chunk = 0x50000000, controller = 0x60000000, pawn = 0x70000000;
    constexpr std::uintptr_t weapon = 0x71000000, service = 0x72000000, punch = 0x73000000;
    constexpr std::uint32_t pawnHandle = 0x8002, weaponHandle = 0x10003;
    m.Put(client + p::entitySystem, world); m.Put(engine + p::networkClient, network); m.Put(network + p::signOnState, 6);
    m.Put(client + p::localController, controller); m.Put(client + p::localPawn, pawn); m.Put(controller + p::controllerPawn, pawnHandle);
    m.Put(world + 0x10, chunk);
    auto identity = [&](std::uintptr_t entity, std::uint32_t handle) {
        const auto slot = chunk + (handle & p::indexMask) * p::stride;
        m.Put(slot, entity); m.Put(slot + 0x10, handle); m.Put(slot + 0x30, 0u); m.Put(entity + p::identity, slot);
    };
    identity(pawn, pawnHandle); identity(weapon, weaponHandle);
    m.Put(pawn + p::health, 100); m.Put(pawn + p::lifeState, std::uint8_t{0});
    m.Put(pawn + w::weaponServices, service); m.Put(service + w::activeWeapon, weaponHandle);
    m.Put(weapon + w::owner, pawnHandle); m.Put(weapon + w::itemDefinition, std::uint16_t{7});
    m.Put(pawn + w::punchService, punch); m.Put(punch + w::serviceOwner, pawn);
    m.Put(pawn + w::shotsFired, 2); m.Put(weapon + w::clip, 28); m.Put(weapon + w::mode, 0);
    m.Put(weapon + w::recoilIndex, 2.f); m.Put(weapon + w::lastShot, 10.f);
    m.Put(weapon + w::reload, std::uint8_t{0}); m.Put(weapon + w::silencer, std::uint8_t{0});
    m.Put(weapon + w::nextPrimaryTick, 640); m.Put(weapon + w::nextPrimaryFraction, .5f);
    m.Put(service + w::nextAttack, 9.f); m.Put(pawn + w::scoped, std::uint8_t{0});
    sdk::weapons::Weapon out;
    CHECK(sdk::weapons::ReadWeapon(m.Reader(), client, engine, out)); CHECK(out.identity.item == 7 && out.shots == 2);
    CHECK(std::abs(out.nextAttack - 10.0078125) < .00001 && !out.scoped);
    m.Put(service + w::nextAttack, 12.f); m.Put(pawn + w::scoped, std::uint8_t{1});
    CHECK(sdk::weapons::ReadWeapon(m.Reader(), client, engine, out) && out.nextAttack == 12 && out.scoped);
    m.Put(weapon + w::nextPrimaryFraction, 1.f); CHECK(!sdk::weapons::ReadWeapon(m.Reader(), client, engine, out));
    m.Put(weapon + w::nextPrimaryFraction, .5f);
    unsigned count = 0;
    for (std::uint16_t item = 0; item < 600; ++item) if (sdk::weapons::IsFirearm(item)) {
        ++count; m.Put(weapon + w::itemDefinition, item);
        m.Put(weapon + w::burstRemaining, 0);
        CHECK(sdk::weapons::ReadWeapon(m.Reader(), client, engine, out)); CHECK(out.identity.item == item);
    }
    CHECK(count == 34); // Zeus, knives, grenades and C4 excluded.
    m.Put(weapon + w::itemDefinition, std::uint16_t{31}); CHECK(!sdk::weapons::ReadWeapon(m.Reader(), client, engine, out));
    m.Put(weapon + w::itemDefinition, std::uint16_t{60}); CHECK(sdk::weapons::ReadWeapon(m.Reader(), client, engine, out));
    m.Put(service + w::activeWeapon, weaponHandle + 0x8000); CHECK(!sdk::weapons::ReadWeapon(m.Reader(), client, engine, out));
    m.Put(service + w::activeWeapon, weaponHandle); m.Put(pawn + p::health, 0);
    CHECK(!sdk::weapons::ReadWeapon(m.Reader(), client, engine, out));
    m.Put(pawn + p::health, 100); m.Put(network + p::signOnState, 3);
    CHECK(!sdk::weapons::ReadWeapon(m.Reader(), client, engine, out));
}
Sample live;
void* caller = nullptr;
Angle camera;
std::atomic<bool> block{false}, entered{false}, released{false};
bool throwOriginal = false, throwCapture = false, allowed = true;
#pragma optimize("", off)
__declspec(noinline) std::uintptr_t __fastcall GameCommit(void* context, unsigned slot, Angle* angle) {
    caller = _ReturnAddress(); camera = *angle;
    if (throwOriginal) RaiseException(0xE0424242, 0, 0, nullptr);
    if (block.load()) { entered = true; while (!released.load()) Sleep(1); }
    return reinterpret_cast<std::uintptr_t>(context) + slot + 123;
}
using Commit = std::uintptr_t (__fastcall*)(void*, unsigned, Angle*);
Commit volatile invoke = GameCommit;
__declspec(noinline) std::uintptr_t FinalInput(unsigned slot, Angle* angle) {
    const auto result = invoke(reinterpret_cast<void*>(100), slot, angle);
    return result;
}
__declspec(noinline) void OtherCaller(Angle* angle) { invoke(nullptr, 0, angle); }
#pragma optimize("", on)
bool Capture(Angle* angle, Sample& s) {
    if (throwCapture) RaiseException(0xE0424242, 0, 0, nullptr);
    s = live; s.view = *angle; return allowed;
}
bool Fault(Angle* angle) {
    __try { FinalInput(0, angle); return false; }
    __except(GetExceptionCode() == 0xE0424242 ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { return true; }
}
void HookTests() {
    CHECK(MH_Initialize() == MH_OK);
    live = Baseline(); auto angle = live.view; CHECK(FinalInput(0, &angle) == 223);
    const auto sourceReturn = caller;
    CHECK(!hooks::input::Install({}));
    CHECK(hooks::input::Install({reinterpret_cast<void*>(&GameCommit), sourceReturn, Capture}));
    modules::RecoilControl module;
    CHECK(!module.requestedEnabled() && module.settings().size() == 6);
    module.setEnabled(true); module.tick(); CHECK(hooks::input::Available());
    FinalInput(0, &angle); live.attack = true; Advance(live, true, 1); FinalInput(0, &angle); Near(angle.pitch, 10);
    Advance(live, true, 3); OtherCaller(&angle); Near(angle.pitch, 10);
    CHECK(FinalInput(1, &angle) == 224); Near(angle.pitch, 10);
    FinalInput(0, &angle); Near(angle.pitch, 8); Near(camera.pitch, 8);
    FinalInput(0, &angle); Near(angle.pitch, 8); // no duplicate feedback
    allowed = false; Advance(live, true, 8); FinalInput(0, &angle); Near(angle.pitch, 8);
    allowed = true; Advance(live, true, 10); FinalInput(0, &angle); Near(angle.pitch, 8);
    module.setEnabled(false); module.tick(); Advance(live, true, 13); FinalInput(0, &angle); Near(angle.pitch, 8);
    module.setEnabled(true); module.tick(); FinalInput(0, &angle); Near(angle.pitch, 8);
    throwOriginal = true; CHECK(Fault(&angle)); throwOriginal = false;
    block = true; entered = false; released = false;
    std::thread pending([&] { FinalInput(0, &angle); });
    for (int i = 0; i < 1000 && !entered; ++i) Sleep(1);
    CHECK(entered && !hooks::input::Stop(10)); released = true; pending.join(); block = false;
    CHECK(hooks::input::Stop(1000)); hooks::input::Finalize(); module.shutdown();
    CHECK(hooks::input::Install({reinterpret_cast<void*>(&GameCommit), sourceReturn, Capture}));
    hooks::input::Publish({}, true); throwCapture = true; FinalInput(0, &angle); throwCapture = false;
    CHECK(!hooks::input::Available()); CHECK(hooks::input::Stop(1000)); hooks::input::Finalize();
    CHECK(MH_Uninitialize() == MH_OK);
}
}
void TestRecoil() {
    ControllerTests(); WeaponTests(); HookTests();
    puts("PASS: recoil math, smoothing, shot boundaries, replay, 34 weapon IDs, handle validation, native input ABI/filtering, menu reset, exception/drain/reinstall; no game injection");
}
