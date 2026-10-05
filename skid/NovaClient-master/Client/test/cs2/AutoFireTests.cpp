#include "cs2/hooks/InputHook.h"
#include "cs2/input/AttackButtons.h"
#include "cs2/modules/combat/autofire/AutoFire.h"
#include "cs2/modules/ModuleManager.h"
#include "cs2/sdk/weapons/Offsets.h"
#include "cs2/sdk/players/Offsets.h"
#include "ui/notifications/Notifications.h"
#include "MinHook.h"
#include <windows.h>
#include <intrin.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <atomic>
#include <thread>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "Auto Fire test failed at %d: %s\n", __LINE__, #x); std::exit(1); } } while (false)
namespace {
using namespace nova::cs2;
using sdk::weapons::Sample;
using sdk::weapons::Angle;
using sdk::weapons::FireState;
using sdk::weapons::AttackResult;
namespace traceTest {
unsigned stage = 0;
float worldFraction = .5f, playerFraction = .25f;
bool solid = false;
constexpr std::uintptr_t worldEntity = 0x10000, playerEntity = 0x20000;
void Result(void* bytes, std::uintptr_t entity, float fraction) {
    auto* out = static_cast<unsigned char*>(bytes);
    std::memcpy(out + 8, &entity, sizeof(entity));
    std::memcpy(out + 0xAC, &fraction, sizeof(fraction));
    out[0xBB] = solid; out[0xBC] = 1;
}
std::uintptr_t Filter(void* bytes, std::uintptr_t ignored, std::uint64_t mask, unsigned char layer, unsigned char flags) {
    CHECK(ignored == 0x30000 && mask == 0x1C300B && flags == 15);
    CHECK((stage == 0 && layer == 3) || (stage == 2 && layer == 4));
    ++stage; static_cast<unsigned char*>(bytes)[56] = layer; return reinterpret_cast<std::uintptr_t>(bytes);
}
void World(std::uintptr_t manager, const sdk::aim::Vec3* start, const sdk::aim::Vec3* end, void* filter, void* out) {
    CHECK(stage++ == 1 && manager == 0x40000 && static_cast<unsigned char*>(filter)[56] == 3);
    CHECK(start->x == 0 && end->x == 8192);
    Result(out, worldEntity, worldFraction);
}
void Players(const sdk::aim::Vec3* start, const sdk::aim::Vec3* end, void* filter, void* out, float minimum, float maximum, float fraction) {
    CHECK(stage++ == 3 && static_cast<unsigned char*>(filter)[56] == 4);
    CHECK(start->x == 0 && end->x == 8192 && minimum == 0 && maximum == 60 && fraction == -1);
    // Emulate the verified native comparison: use the existing world fraction.
    float cutoff = 0; std::memcpy(&cutoff, static_cast<unsigned char*>(out) + 0xAC, sizeof(cutoff));
    if (playerFraction < cutoff) Result(out, playerEntity, playerFraction);
}
void Run() {
    const sdk::aim::ShotTraceBinding binding{Filter, World, Players};
    const auto trace = [&] { stage = 0; return sdk::aim::TracePlayers(binding, 0x40000, 0x30000, {}, {8192, 0, 0}); };
    auto result = trace(); CHECK(stage == 4 && result.hit == playerEntity && !result.startSolid);
    worldFraction = .1f; result = trace(); CHECK(stage == 4 && result.hit == worldEntity); // Wall stays first.
    worldFraction = 1; playerFraction = 1; result = trace(); CHECK(result.fraction == 1); // Miss.
    solid = true; result = trace(); CHECK(stage == 2 && result.startSolid); // No clipping past a solid start.
    solid = false; worldFraction = std::numeric_limits<float>::quiet_NaN();
    result = trace(); CHECK(stage == 2 && !std::isfinite(result.fraction));
    CHECK(!std::isfinite(sdk::aim::TracePlayers({}, 0x40000, 0x30000, {}, {8192, 0, 0}).fraction));
}
}
Sample Baseline() {
    Sample s; s.frame = 1; s.ordinal = 1; s.time = 10;
    s.weapon.identity = {0x10000, 0x20000, 0x30000, 0x40000, 0x8001, 0x8002, 7, 0, false};
    s.weapon.clip = 30; s.weapon.lastShot = 9; return s;
}
void Next(Sample& s, double dt = .02) { ++s.frame; s.time += dt; }
void Shot(Sample& s) { ++s.weapon.shots; --s.weapon.clip; s.weapon.lastShot = static_cast<float>(s.time); }
void ControllerTests() {
    autofire::Controller c; auto s = Baseline(); FireState w{true, true, true, false, 8192};
    CHECK(!c.Process(s, w, 0)); Next(s); CHECK(c.Process(s, w, 7)); CHECK(c.Process(s, w, 7));
    Next(s); Shot(s); s.weapon.nextAttack = s.time + .1;
    CHECK(c.Process(s, w, 7)); // Automatic holds between rounds.
    Next(s); CHECK(!c.Process(s, w, 0)); Next(s); CHECK(!c.Process(s, w, 7)); // Initial cooldown.
    Next(s, .1); CHECK(c.Process(s, w, 7));
    c.Configure({100, false}); s = Baseline();
    CHECK(!c.Process(s, w, 7)); const auto first = s;
    Next(s, .06); CHECK(!c.Process(s, w, 7));
    Next(s, .04); CHECK(c.Process(s, w, 7));
    CHECK(!c.Process(first, w, 7)); // Rewind doesn't borrow elapsed delay from future samples.
    s = first; Next(s, .08); CHECK(!c.Process(s, w, 7)); Next(s, .02); CHECK(c.Process(s, w, 7));
    Next(s); CHECK(!c.Process(s, w, 8)); Next(s, .1); CHECK(c.Process(s, w, 8));
    Next(s); w.allowed = false; CHECK(!c.Process(s, w, 8)); w.allowed = true;
    Next(s); CHECK(!c.Process(s, w, 8)); Next(s, .1); CHECK(c.Process(s, w, 8));
    c.Configure({0, false}); c.Reset(); s = Baseline(); w.fullAuto = false; s.weapon.identity.item = 1;
    CHECK(c.Process(s, w, 8)); Next(s); Shot(s); CHECK(!c.Process(s, w, 8));
    CHECK(!c.Process(s, w, 8)); // Replay cannot skip the release.
    Next(s, .001); CHECK(!c.Process(s, w, 8)); Next(s); CHECK(c.Process(s, w, 8));
    // R8 is held through native cocking even when its cooldown changes.
    c.Reset(); s = Baseline(); s.weapon.identity.item = 64; w.revolver = true;
    CHECK(c.Process(s, w, 8)); Next(s); s.weapon.nextAttack = s.time + .2; CHECK(c.Process(s, w, 8));
    Next(s, .1); CHECK(c.Process(s, w, 8)); Next(s, .12); Shot(s); CHECK(!c.Process(s, w, 8));
    Next(s); CHECK(c.Process(s, w, 8));
    // Burst follow-ups belong to the engine; scoped automatic rifles aren't semi-auto.
    c.Reset(); s = Baseline(); s.weapon.identity.item = 10; s.weapon.identity.mode = 1; w.revolver = false; w.fullAuto = true;
    CHECK(c.Process(s, w, 8)); Next(s); Shot(s); s.weapon.burst = 2; CHECK(!c.Process(s, w, 8));
    Next(s); CHECK(!c.Process(s, w, 8)); Next(s); s.weapon.burst = 0; CHECK(c.Process(s, w, 8));
    c.Reset(); s.weapon.identity.item = 8; CHECK(c.Process(s, w, 8)); Next(s); Shot(s); CHECK(c.Process(s, w, 8));
    c.Configure({0, true}); s = Baseline(); s.weapon.identity.item = 9; CHECK(!c.Process(s, w, 8));
    Next(s); s.weapon.scoped = true; CHECK(c.Process(s, w, 8));
    for (int condition = 0; condition < 6; ++condition) {
        c.Reset(); s = Baseline(); CHECK(c.Process(s, w, 8)); Next(s);
        if (condition == 0) s.weapon.clip = 0;
        if (condition == 1) s.weapon.reload = true;
        if (condition == 2) s.weapon.switchingSilencer = true;
        if (condition == 3) s.weapon.identity.item = 42;
        if (condition == 4) s.time = std::numeric_limits<double>::quiet_NaN();
        if (condition == 5) w.valid = false;
        CHECK(!c.Process(s, w, 8)); w.valid = true;
    }
    c.Configure({100, false}); s = Baseline(); CHECK(!c.Process(s, w, 8));
    Next(s, .1); CHECK(c.Process(s, w, 8)); Next(s); ++s.weapon.identity.weaponHandle;
    CHECK(!c.Process(s, w, 8)); // Switch resets acquisition.
    Next(s, .3); CHECK(!c.Process(s, w, 8));
}
void MergeTests() {
    input::InputSample s;
    s.angle = {10, 20, 0}; s.buttons = {8, 8, 2048};
    s.count = 1; s.moves[0] = {.2f, 0, 8, true};
    CHECK(input::MergeAttack(s, true, false, .4f));
    CHECK(s.buttons.held == 9 && s.buttons.pressed == 9 && s.buttons.released == 2048 && s.count == 2);
    CHECK(s.moves[1].mask == 1 && s.moves[1].down && s.moves[1].fraction == .4f && s.moves[1].yaw == 20);
    s = {}; CHECK(input::MergeAttack(s, true, true, .5f)); CHECK(s.buttons.held == 1 && s.count == 0);
    // A physical release while Auto Fire holds cannot release the combined input.
    s = {}; s.buttons.released = 1; s.count = 1; s.moves[0] = {.2f, 0, 1, false};
    CHECK(input::MergeAttack(s, true, true, .5f)); CHECK(s.buttons.held == 1 && !s.buttons.released && !s.count);
    s = {}; CHECK(input::MergeAttack(s, false, true, .6f));
    CHECK(s.buttons.released == 1 && s.count == 1 && !s.moves[0].down);
    // Taking over an already held manual input doesn't create another press.
    s = {}; s.buttons.held = 1; CHECK(input::MergeAttack(s, true, false, .5f));
    CHECK(!s.buttons.pressed && !s.count);
    s = {}; s.buttons = {1, 1, 0}; CHECK(input::MergeAttack(s, false, true, .5f));
    CHECK(s.buttons.held == 1 && !s.buttons.released && !s.buttons.pressed);
    s = {}; s.buttons = {0, 1, 1}; const auto original = s;
    CHECK(input::MergeAttack(s, false, false, .5f)); CHECK(!std::memcmp(&s, &original, sizeof(s)));
    s.count = 2; s.moves[0] = {.1f, 0, 1, true}; s.moves[1] = {.2f, 0, 1, false};
    CHECK(input::MergeAttack(s, true, false, .5f));
    CHECK(s.count == 3 && s.moves[0].fraction == .1f && s.moves[1].fraction == .2f && s.moves[2].down);
    CHECK(s.buttons.held == 1 && s.buttons.pressed == 1 && s.buttons.released == 1); // Preserve manual tap before takeover.
    s = {}; s.count = 32;
    for (auto& move : s.moves) move = {.1f, 0, 8, true};
    const auto full = s; CHECK(!input::MergeAttack(s, true, false, .5f)); CHECK(!std::memcmp(&s, &full, sizeof(s)));
    s = {}; CHECK(!input::MergeAttack(s, true, false, std::numeric_limits<float>::quiet_NaN()));
}
struct Memory {
    std::unordered_map<std::uintptr_t, unsigned char> bytes;
    template<class T> void Put(std::uintptr_t address, T value) {
        const auto* p = reinterpret_cast<const unsigned char*>(&value);
        for (unsigned i = 0; i < sizeof(T); ++i) bytes[address + i] = p[i];
    }
    sdk::players::Memory Reader() {
        return {this, [](void* context, std::uintptr_t address, void* out, std::size_t size) {
            const auto& m = *static_cast<Memory*>(context); auto* data = static_cast<unsigned char*>(out);
            for (std::size_t i = 0; i < size; ++i) { const auto at = m.bytes.find(address + i); if (at == m.bytes.end()) return false; data[i] = at->second; }
            return true;
        }};
    }
};
void AdapterTests() {
    namespace p = sdk::players::offsets; namespace w = sdk::weapons::offsets;
    Memory m; auto s = Baseline(); const auto& id = s.weapon.identity;
    const std::uintptr_t chunk = 0x50000, data = 0x60000, service = 0x70000;
    m.Put(id.world + 0x10, chunk);
    auto entity = [&](std::uintptr_t address, std::uint32_t handle) {
        const auto slot = chunk + (handle & p::indexMask) * p::stride;
        m.Put(slot, address); m.Put(slot + 0x10, handle); m.Put(slot + 0x30, 0u); m.Put(address + p::identity, slot);
    };
    entity(id.pawn, id.pawnHandle); entity(id.weapon, id.weaponHandle);
    m.Put(id.pawn + w::weaponServices, service); m.Put(service + w::activeWeapon, id.weaponHandle);
    m.Put(id.weapon + w::weaponData, data); m.Put(data + w::fullAuto, std::uint8_t{1});
    m.Put(data + w::revolver, std::uint8_t{0}); m.Put(data + w::range, 8192.f); m.Put(id.pawn + w::waitForNoAttack, std::uint8_t{0});
    FireState out; CHECK(sdk::weapons::ReadFireState(m.Reader(), s, out) && out.fullAuto && out.allowed && out.range == 8192);
    m.Put(id.pawn + w::waitForNoAttack, std::uint8_t{1}); CHECK(sdk::weapons::ReadFireState(m.Reader(), s, out) && !out.allowed);
    m.Put(data + w::fullAuto, std::uint8_t{2}); CHECK(!sdk::weapons::ReadFireState(m.Reader(), s, out));
    m.Put(data + w::fullAuto, std::uint8_t{1}); m.Put(service + w::activeWeapon, id.weaponHandle + 0x8000);
    CHECK(!sdk::weapons::ReadFireState(m.Reader(), s, out));
    sdk::aim::Vec3 end;
    CHECK(sdk::aim::ShotEnd({1, 2, 3}, {-10, -20, 0}, {10, 20, 0}, 100, end));
    CHECK(std::abs(end.x - 101) < .001 && std::abs(end.y - 2) < .001 && std::abs(end.z - 3) < .001);
    CHECK(sdk::aim::ShotEnd({}, {0, 90, 0}, {}, 100, end) && std::abs(end.y - 100) < .001);
    CHECK(!sdk::aim::ShotEnd({}, {}, {}, 0, end)); CHECK(!sdk::aim::ResolveShotCapture());
}
Sample live;
input::InputSample native;
input::AttackButtons manual;
Angle traced;
void* caller = nullptr;
bool ready = false, captureAllowed = true, hit = true, throwing = false, disableDuringFrame = false, ignoredTeam = false;
bool fullHistory = false, writeException = false;
AttackResult writeResult = AttackResult::Applied;
unsigned writes = 0;
std::atomic<bool> blocking{false}, entered{false}, released{false};
#pragma optimize("", off)
__declspec(noinline) std::uintptr_t __fastcall GameCommit(void*, unsigned, Angle*) { caller = _ReturnAddress(); return 12; }
using Commit = std::uintptr_t (__fastcall*)(void*, unsigned, Angle*);
Commit volatile invokeCommit = GameCommit;
__declspec(noinline) void FinalInput() { invokeCommit(nullptr, 0, &native.angle); }
__declspec(noinline) void __fastcall GameFrame(void*, int slot, float) {
    native = {}; native.angle = live.view; ready = false;
    if (!slot) FinalInput();
    // Match the game: its held-mask assignment happens AFTER the angle hook.
    native.buttons = manual; ready = true;
    if (fullHistory) {
        native.count = 32;
        for (auto& move : native.moves) move = {.1f, 0, 8, true};
    }
    if (disableDuringFrame) hooks::input::PublishFire({}, false, false, false, 0);
    if (blocking.load()) { entered = true; while (!released.load()) Sleep(1); }
}
using Frame = void (__fastcall*)(void*, int, float);
Frame volatile invokeFrame = GameFrame;
void Run(int slot = 0) { invokeFrame(reinterpret_cast<void*>(0x100), slot, .02f); }
#pragma optimize("", on)
bool Capture(Angle* angle, Sample& out) { out = live; out.view = *angle; return captureAllowed; }
bool FireInfo(const Sample&, FireState& out) { out = {true, true, true, false, 8192}; return true; }
bool ShotRay(const Sample&, Angle view, float range, bool ignoreTeam, std::uint32_t& target) {
    if (throwing) RaiseException(0xE0424242, 0, 0, nullptr);
    traced = view; ignoredTeam = ignoreTeam; CHECK(range == 8192); target = hit ? 0x8010 : 0; return true;
}
bool AimTargets(const Sample&, sdk::aim::Query, sdk::aim::Snapshot& out) {
    out = {}; out.count = 1; out.targets[0] = {0x8010, {100, 20, 0}}; return true;
}
AttackResult Write(void* input, const sdk::weapons::AttackRequest& request) {
    CHECK(ready && input == reinterpret_cast<void*>(0x100)); ++writes;
    if (writeException) RaiseException(0xE0424243, 0, 0, nullptr);
    if (writeResult != AttackResult::Applied) return writeResult;
    CHECK(!request.down || request.angle == &native.angle);
    if (!input::MergeAttack(native, request.down, request.previous, request.fraction)) return AttackResult::SampleRejected;
    return request.down ? AttackResult::Applied : AttackResult::Released;
}
void HookTests() {
    CHECK(MH_Initialize() == MH_OK); live = Baseline(); Run();
    const sdk::weapons::Binding binding{reinterpret_cast<void*>(&GameCommit), caller, Capture,
        reinterpret_cast<void*>(&GameFrame), FireInfo, Write};
    CHECK(hooks::input::Install(binding, AimTargets, ShotRay)); CHECK(hooks::input::FireAvailable());
    hooks::input::PublishFire({}, true, true, false, 0); Run();
    CHECK(native.buttons.held == 1 && native.buttons.pressed == 1 && ignoredTeam && writes == 1);
    CHECK(native.angle.yaw == live.view.yaw && traced.yaw == live.view.yaw); // Standalone never turns.
    Next(live); Run(); CHECK(native.buttons.held == 1 && !native.buttons.pressed);
    Next(live); hit = false; Run(); CHECK(!native.buttons.held && native.buttons.released == 1);
    Next(live); hit = true; manual = {1, 1, 0}; Run(); CHECK(native.buttons.held == 1);
    hooks::input::PublishFire({}, false, false, false, 0); Next(live); Run();
    CHECK(native.buttons.held == 1 && !native.buttons.released); manual = {};
    hooks::input::PublishFire({}, true, false, true, 0); Next(live); const auto before = writes; Run(); CHECK(writes == before); // Unbound hold.
    hooks::input::PublishFire({}, true, false, false, 0); Next(live); Run(); CHECK(native.buttons.held == 1);
    captureAllowed = false; Next(live); Run(); CHECK(!native.buttons.held && native.buttons.released == 1);
    captureAllowed = true; Next(live); Run(); CHECK(native.buttons.held == 1);
    // A temporarily unavailable/stale sample must not latch an adapter fault.
    // Release the previously committed hold before a fresh press, even after a long gap.
    for (const auto deferred : {AttackResult::NoSample, AttackResult::StaleSample}) {
        writeResult = deferred;
        for (int i = 0; i < 3; ++i) { Next(live); Run(); CHECK(hooks::input::FireAvailable() && !native.buttons.held); }
        writeResult = AttackResult::Applied; Next(live); Run();
        CHECK(!native.buttons.held && native.buttons.released == 1);
        Next(live); Run(); CHECK(native.buttons.held == 1 && native.buttons.pressed == 1);
    }
    // Actual MergeAttack rejects a full native subtick history without changing it.
    hit = false; fullHistory = true; Next(live); Run();
    CHECK(hooks::input::FireAvailable() && !native.buttons.held && !native.buttons.released && native.count == 32);
    hooks::input::PublishFire({}, false, false, false, 0);
    Next(live); Run(); CHECK(hooks::input::FireAvailable() && native.count == 32);
    fullHistory = false; Next(live); Run(); CHECK(native.buttons.released == 1 && !native.buttons.held);
    // With no previous hold, skip a full press sample and generate a new edge after recovery.
    hit = true; hooks::input::PublishFire({}, true, false, false, 0);
    fullHistory = true; Next(live); Run(); CHECK(hooks::input::FireAvailable() && !native.buttons.held && native.count == 32);
    fullHistory = false; Next(live); Run(); CHECK(native.buttons.held == 1 && native.buttons.pressed == 1);
    CHECK(hooks::input::ReadFireDebug().deferred == 9);
    CHECK(!std::strcmp(hooks::input::ReadFireDebug().lastDeferred, "input history invalid or full"));
    // A failed automatic release must never clear a subsequent manual press.
    hit = false; writeResult = AttackResult::NoSample; Next(live); Run();
    manual = {1, 1, 0}; writeResult = AttackResult::Applied; Next(live); Run();
    CHECK(native.buttons.held == 1 && !native.buttons.released);
    manual = {}; hit = true; Next(live); Run(); CHECK(native.buttons.pressed == 1);
    disableDuringFrame = true; Next(live); Run(); CHECK(!native.buttons.held); disableDuringFrame = false;
    hooks::input::PublishFire({}, true, false, false, 0); Next(live); Run(1); CHECK(!native.buttons.held);
    // RCS sees prior synthetic firing plus real shot feedback. The ray receives final corrected input.
    hooks::input::Publish({100, 100, 1, 0}, true); Next(live); Run();
    Next(live); Shot(live); live.punch.pitch = 2; Run(); CHECK(std::abs(traced.pitch + 2) < .001);
    hooks::input::PublishAim({30, 120, 90}, true, false, false, 0);
    Next(live); Run(); Next(live); Run();
    CHECK(traced.yaw > 0 && traced.yaw == native.angle.yaw); // Auto Fire uses this sample's Aim output.
    hooks::input::PublishAim({}, false, false, false, 0);
    throwing = true; Next(live); Run(); throwing = false;
    CHECK(!hooks::input::FireAvailable() && hooks::input::Available() && !native.buttons.held);
    CHECK(!std::strcmp(hooks::input::FireUnavailableReason(), "fire capture exception"));
    CHECK(hooks::input::ReadFireDebug().exceptionCode == 0xE0424242 && hooks::input::ReadFireDebug().exceptionAddress);
    CHECK(hooks::input::Stop(1000)); hooks::input::Finalize();
    CHECK(hooks::input::Install(binding, nullptr, ShotRay)); CHECK(hooks::input::FireAvailable());
    // A genuine native write exception remains latched and is never retried every frame.
    hooks::input::PublishFire({}, true, false, false, 0);
    writeException = true; Next(live); Run(); writeException = false;
    CHECK(!hooks::input::FireAvailable() && hooks::input::Available());
    CHECK(hooks::input::ReadFireDebug().exceptionCode == 0xE0424243);
    CHECK(!std::strcmp(hooks::input::FireUnavailableReason(), "input write exception"));
    const auto faultWrites = writes; Next(live); Run(); CHECK(writes == faultWrites && !native.buttons.held);
    CHECK(hooks::input::Stop(1000)); hooks::input::Finalize();
    CHECK(hooks::input::Install(binding, nullptr, ShotRay)); CHECK(hooks::input::FireAvailable());
    modules::ModuleManager manager; auto& module = manager.Create<modules::AutoFire>(); manager.Seal();
    CHECK(module.category() == "combat" && !module.requestedEnabled() && !module.keybind().hold() && module.settings().size() == 4);
    Properties::NumberProp* delay = nullptr; Properties::BoolProperty* scoped = nullptr;
    for (const auto& row : module.settings()) {
        if (std::string_view(row.getName()) == "Delay") delay = dynamic_cast<Properties::NumberProp*>(row.getProp());
        if (std::string_view(row.getName()) == "Scoped Only") scoped = dynamic_cast<Properties::BoolProperty*>(row.getProp());
    }
    CHECK(delay && scoped && delay->get() == 0 && !scoped->get());
    delay->set(123); scoped->set(true); module.setEnabled(true); manager.Tick();
    auto config = nova::json::Value::Object(); manager.Save(config); delay->set(0); scoped->set(false); manager.Load(config); manager.Tick();
    CHECK(delay->get() == 123 && scoped->get() && module.requestedEnabled());
    manager.Shutdown(); hooks::input::PublishFire({}, true, false, false, 0);
    // Stop must drain the outer input scope, cancel its pending press, and permit reinstall.
    blocking = true; entered = false; released = false; std::thread worker([] { Run(); });
    for (unsigned i = 0; i < 1000 && !entered; ++i) Sleep(1);
    CHECK(entered && !hooks::input::Stop(10)); released = true; worker.join(); blocking = false;
    CHECK(!native.buttons.held && hooks::input::Stop(1000)); hooks::input::Finalize();
    CHECK(hooks::input::Install(binding, nullptr, ShotRay)); Next(live); Run(); CHECK(!native.buttons.held);
    CHECK(hooks::input::Stop(1000)); hooks::input::Finalize();
    // Missing trace binding has a specific startup reason, and repeated enables notify once.
    CHECK(hooks::input::Install(binding)); CHECK(!hooks::input::FireAvailable());
    CHECK(!std::strcmp(hooks::input::FireUnavailableReason(), "player trace binding unavailable"));
    modules::ModuleManager unavailableManager; auto& unavailable = unavailableManager.Create<modules::AutoFire>(); unavailableManager.Seal();
    nova::notifications::Drain();
    for (int i = 0; i < 4; ++i) { unavailable.setEnabled(true); unavailableManager.Tick(); unavailableManager.Tick(); }
    const auto notices = nova::notifications::Drain();
    CHECK(notices.size() == 1 && notices.front().text.find("player trace binding unavailable") != std::string::npos);
    CHECK(!unavailable.requestedEnabled()); unavailableManager.Shutdown();
    CHECK(hooks::input::Stop(1000)); hooks::input::Finalize();
    CHECK(!hooks::input::Install({}));
    CHECK(!std::strcmp(hooks::input::FireUnavailableReason(), "shared input binding unavailable"));
    CHECK(MH_Uninitialize() == MH_OK);
}
}
void TestAutoFire() {
    traceTest::Run(); ControllerTests(); MergeTests(); AdapterTests(); HookTests();
    puts("PASS: Auto Fire delay/replay, auto/semi/burst/R8, scope/cooldowns, native subtick/manual merge, transient recovery/release, final ray/recoil, config/hold, fault diagnostics/isolation/drain/reinstall; no game injection");
}
