#include "cs2/hooks/InputHook.h"
#include "cs2/input/AimPipeline.h"
#include "cs2/modules/combat/aimbot/Aimbot.h"
#include "cs2/modules/ModuleManager.h"
#include "cs2/sdk/aim/Offsets.h"
#include "cs2/sdk/players/Offsets.h"
#include "ui/hud/aim/FovCircle.h"
#include "MinHook.h"
#include <windows.h>
#include <intrin.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <numbers>
#include <unordered_map>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "Aim test failed at %d: %s\n", __LINE__, #x); std::exit(1); } } while (false)
namespace {
using namespace nova::cs2;
using sdk::weapons::Sample;
using sdk::weapons::Angle;
void Near(float a, float b, float tolerance = .0003f) { CHECK(std::abs(a - b) < tolerance); }
Sample Baseline() {
    Sample s; s.frame = 1; s.ordinal = 1; s.time = 10;
    s.weapon.identity = {1, 2, 3, 4, 5, 6, 7, 0, false};
    s.weapon.clip = 30; s.weapon.lastShot = 9; s.weapon.accuracyValid = true; s.view.roll = 1;
    return s;
}
void Advance(Sample& s, double dt = .01) { ++s.frame; s.time += dt; }
sdk::aim::Target Target(std::uint32_t handle, float yaw, float pitch = 0) {
    const double r = std::numbers::pi / 180.;
    return {handle, {static_cast<float>(100 * std::cos(pitch * r) * std::cos(yaw * r)),
        static_cast<float>(100 * std::cos(pitch * r) * std::sin(yaw * r)), static_cast<float>(-100 * std::sin(pitch * r))}};
}
sdk::aim::Snapshot Snapshot(float yaw, float pitch = 0) {
    sdk::aim::Snapshot s; s.targets[0] = Target(0x8002, yaw, pitch); s.count = 1; return s;
}
void MathTests() {
    Angle out;
    CHECK(aim::Direction({}, {1, 1, 0}, out)); Near(out.yaw, 45); Near(out.pitch, 0);
    CHECK(aim::Direction({}, {0, 0, 1}, out)); Near(out.pitch, -90);
    CHECK(!aim::Direction({}, {}, out));
    CHECK(!aim::Direction({}, {std::numeric_limits<float>::infinity(), 0, 0}, out));
    Near(aim::Distance({0, 179, 0}, {0, -179, 0}), 2);
    Near(aim::Distance({90, 0, 0}, {90, 120, 0}), 0);
    aim::Controller c; aim::Settings config{30, 120, 90};
    auto s = Baseline(); auto targets = Snapshot(20, 10);
    s.view = c.Process(s, targets, config, s.view, {}); Near(s.view.yaw, 0);
    Advance(s); s.view = c.Process(s, targets, config, s.view, {});
    Near(s.view.yaw, 1.2f); Near(s.view.pitch, .9f); Near(s.view.roll, 1);
    config.horizontal = 0; config.vertical = 0;
    Advance(s); out = c.Process(s, targets, config, s.view, {}); Near(out.yaw, 1.2f); Near(out.pitch, .9f);
    c.Reset(); s = Baseline(); s.view.yaw = 179; config = {30, 720, 720}; targets = Snapshot(-179);
    c.Process(s, targets, config, s.view, {}); Advance(s); s.view = c.Process(s, targets, config, s.view, {}); Near(s.view.yaw, -179);
    c.Reset(); s = Baseline(); s.view.pitch = -88; targets = Snapshot(0, -90);
    c.Process(s, targets, config, s.view, {}); Advance(s); Near(c.Process(s, targets, config, s.view, {}).pitch, -89);
    auto timed = [](int fps) {
        aim::Controller controller; auto sample = Baseline(); auto snapshot = Snapshot(25);
        const aim::Settings config{30, 12, 0}; controller.Process(sample, snapshot, config, sample.view, {});
        for (int i = 0; i < fps; ++i) {
            Advance(sample, 1. / fps); sample.view = controller.Process(sample, snapshot, config, sample.view, {});
        }
        return sample.view.yaw;
    };
    for (int fps : {30, 60, 144, 240}) Near(timed(fps), 12);
    // Pauses, identity changes and reverse time establish a fresh baseline.
    c.Reset(); s = Baseline(); targets = Snapshot(20); c.Process(s, targets, config, s.view, {});
    Advance(s, 1); Near(c.Process(s, targets, config, s.view, {}).yaw, 0);
    ++s.weapon.identity.pawnHandle; Advance(s); Near(c.Process(s, targets, config, s.view, {}).yaw, 0);
    Advance(s, -.02); Near(c.Process(s, targets, config, s.view, {}).yaw, 0);
    s.time = std::numeric_limits<double>::quiet_NaN(); c.Process(s, targets, config, s.view, {}); CHECK(c.Target() == 0);
}
void SelectionTests() {
    aim::Controller c; auto s = Baseline(); aim::Settings config{30, 0, 0}; auto targets = Snapshot(10);
    c.Process(s, targets, config, s.view, {}); CHECK(c.Target() == 0x8002);
    targets.count = 2; targets.targets[1] = Target(0x8003, 1);
    Advance(s); c.Process(s, targets, config, s.view, {}); CHECK(c.Target() == 0x8002); // Single retains.
    config.switching = true; Advance(s); c.Process(s, targets, config, s.view, {}); CHECK(c.Target() == 0x8003);
    targets.targets[0] = Target(0x8002, .8f);
    Advance(s); c.Process(s, targets, config, s.view, {}); CHECK(c.Target() == 0x8003); // Hysteresis.
    targets.targets[0] = Target(0x8002, .5f);
    Advance(s); c.Process(s, targets, config, s.view, {}); CHECK(c.Target() == 0x8002);
    targets.count = 0; Advance(s); c.Process(s, targets, config, s.view, {}); CHECK(c.Target() == 0);
    targets = Snapshot(31); Advance(s); c.Process(s, targets, config, s.view, {}); CHECK(c.Target() == 0);
    targets = Snapshot(1); Advance(s); c.Process(s, targets, config, s.view, {}); CHECK(c.Target() == 0x8002);
    targets.count = 2; targets.targets[1] = Target(0x8001, 1); c.Reset();
    c.Process(s, targets, config, s.view, {}); CHECK(c.Target() == 0x8001); // Stable tie.
    targets.count = sdk::aim::maxTargets + 1; c.Process(s, targets, config, s.view, {}); CHECK(c.Target() == 0);
}
void BodyPriorityTests() {
    using H = sdk::aim::HitboxPoint;
    using V = sdk::aim::Visibility;
    sdk::aim::Query query; query.fov = 5;
    struct Trace {
        sdk::aim::Vec3 blocked{}, unknown{};
        unsigned calls = 0;
    } trace;
    const auto visible = [](void* context, sdk::aim::Vec3 p) {
        auto& t = *static_cast<Trace*>(context); ++t.calls;
        if (p.x == t.blocked.x && p.y == t.blocked.y && p.z == t.blocked.z) return V::Blocked;
        if (p.x == t.unknown.x && p.y == t.unknown.y && p.z == t.unknown.z) return V::Unknown;
        return V::Visible;
    };
    std::array<H, 4> points{{{Target(1, 4).point, true}, {Target(1, 1).point, false},
        {Target(1, 3).point, false}, {Target(1, 7).point, true}}};
    sdk::aim::Vec3 selected, acquisition;
    const auto choose = [&] { return sdk::aim::SelectPoint({}, points, query, &trace, visible, selected, &acquisition); };
    const auto yaw = [&] { Angle a; CHECK(aim::Direction({}, selected, a)); return a.yaw; };
    CHECK(choose()); Near(yaw(), 4); CHECK(trace.calls == 2); // Visible body admits the preferred region.
    trace.blocked = points[0].position; CHECK(choose()); Near(yaw(), 7); // Other preferred point can be outside FOV.
    trace.unknown = points[3].position; CHECK(choose()); Near(yaw(), 1); // Both preferred points occluded/unknown.
    trace.unknown = points[1].position; points[3].preferred = false;
    CHECK(choose()); Near(yaw(), 3);
    points[0].position = Target(1, 8).point; trace = {};
    query.preferred = 1; CHECK(choose()); Near(yaw(), 8); // FOV admits the player, not only the final region.
    Angle admitted; CHECK(aim::Direction({}, acquisition, admitted)); Near(admitted.yaw, 1);
    points[1].position = Target(1, 6).point; CHECK(choose()); Near(yaw(), 8);
    points[2].position = Target(1, 9).point; trace.calls = 0;
    CHECK(!choose() && trace.calls == 0); // No traces outside FOV.
    points[2].position = Target(1, 2).point; trace.blocked = points[2].position;
    CHECK(!choose()); // Occlusion never counts as a fallback.
    trace = {}; points[2].position.x = std::numeric_limits<float>::quiet_NaN(); CHECK(!choose());
    points[2].position = {}; CHECK(!choose());
    points[2].position = Target(1, 5).point; CHECK(choose()); Near(yaw(), 8);
    query.fov = std::numeric_limits<float>::quiet_NaN(); CHECK(!choose());

    // Exercise capture and controller together, including a recoil kick larger
    // than the FOV. Both stages must see the same pre-sample aim reference.
    struct Capture {
        std::array<H, 2> points{{{Target(1, 12).point, true}, {Target(1, 2).point, false}}};
        float fov = 5;
        Angle reference;
    } capture;
    const auto collect = [](void* data, const Sample&, Angle reference, sdk::aim::Snapshot& out) {
        auto& c = *static_cast<Capture*>(data); c.reference = reference;
        sdk::aim::Query q; q.reference = reference; q.fov = c.fov;
        sdk::aim::Vec3 point, acquisition;
        if (sdk::aim::SelectPoint({}, c.points, q, nullptr, [](void*, sdk::aim::Vec3) { return V::Visible; }, point, &acquisition)) {
            out.targets[0] = {0x8002, point, acquisition}; out.count = 1;
        }
        return true;
    };
    input::AimPipeline pipeline; aim::Settings settings{5, 720, 720};
    pipeline.Configure({}, false, settings, true);
    auto s = Baseline(); s.view = pipeline.Process(s, &capture, collect);
    Advance(s); s.view = pipeline.Process(s, &capture, collect); Near(s.view.yaw, 7.2f);
    CHECK(pipeline.Target() == 0x8002); // Controller must not reject the selected head outside the acquisition FOV.
    capture.points[0].position = Target(1, 4).point;
    Advance(s); s.view = pipeline.Process(s, &capture, collect); Near(s.view.yaw, 4); // Priority returns.
    capture.fov = settings.fov = .2f; settings.smoothMs = 100;
    pipeline.Configure({}, false, settings, true);
    Advance(s); s.view = pipeline.Process(s, &capture, collect);
    Advance(s); s.punch = {6, -7, 0}; s.view = pipeline.Process(s, &capture, collect);
    CHECK(pipeline.Target() == 0x8002); Near(capture.reference.pitch, 0); Near(capture.reference.yaw, 4);
    Near(s.view.pitch + s.punch.pitch, 0); Near(s.view.yaw + s.punch.yaw, 4);
    const auto result = s.view;
    s.view = pipeline.Process(s, &capture, collect); Near(s.view.pitch, result.pitch); Near(s.view.yaw, result.yaw);
    Near(capture.reference.pitch, 0); Near(capture.reference.yaw, 4); // Replay reference is stable, too.
    Advance(s); s.view.yaw += 1; pipeline.Process(s, &capture, collect); CHECK(!pipeline.Target());
}
void SmoothingTests() {
    const auto run = [](int fps, unsigned smooth) {
        aim::Settings settings{30, 12, 6}; settings.smoothMs = smooth;
        aim::Controller c; auto s = Baseline(); const auto targets = Snapshot(8, 4);
        c.Process(s, targets, settings, s.view, {});
        for (int i = 0; i < fps; ++i) {
            const auto previous = s.view;
            Advance(s, 1. / fps); s.view = c.Process(s, targets, settings, s.view, {});
            CHECK(s.view.yaw >= previous.yaw && s.view.yaw <= 8);
            CHECK(s.view.pitch >= previous.pitch && s.view.pitch <= 4);
            CHECK(s.view.yaw - previous.yaw <= 12.f / fps + .0001f);
            CHECK(s.view.pitch - previous.pitch <= 6.f / fps + .0001f);
        }
        return s.view;
    };
    const auto reference = run(60, 100);
    for (int fps : {30, 144, 240}) {
        const auto result = run(fps, 100); Near(result.yaw, reference.yaw); Near(result.pitch, reference.pitch);
    }
    Near(run(60, 0).yaw, 8); CHECK(run(60, 500).yaw < reference.yaw && reference.yaw < 8);
    aim::Settings settings{30, 120, 0}; settings.smoothMs = 100;
    input::AimPipeline pipeline; pipeline.Configure({}, false, settings, true);
    auto s = Baseline(); auto targets = Snapshot(2, 4);
    s.view = pipeline.Process(s, targets); Advance(s); s.view = pipeline.Process(s, targets);
    CHECK(s.view.yaw > 0 && s.view.yaw < 1.2f); Near(s.view.pitch, 0);
    const auto before = s; const auto first = s.view;
    Near(pipeline.Process(s, targets).yaw, first.yaw);
    Advance(s); s.view = pipeline.Process(s, targets);
    auto replay = before; replay.view = {}; Near(pipeline.Process(replay, targets).yaw, first.yaw);
    // Reversal must immediately move toward the new target without stale debt.
    Advance(replay); replay.view = first; const auto reversed = pipeline.Process(replay, Snapshot(-2));
    CHECK(reversed.yaw < first.yaw && reversed.yaw > -2);
    pipeline.Reset(); s = Baseline(); s.view.yaw = 179; targets = Snapshot(-179);
    s.view = pipeline.Process(s, targets); Advance(s); s.view = pipeline.Process(s, targets);
    CHECK(s.view.yaw > 179 && s.view.yaw < 180);
    pipeline.Configure({}, false, settings, false); Advance(s);
    Near(pipeline.Process(s, targets).yaw, s.view.yaw);
    settings.smoothMs = 900; CHECK(aim::Normalize(settings).smoothMs == 500);
}
void GeometryTests() {
    // 90-degree vertical view, then zoom and viewport changes.
    Near(nova::ui::aim::Radius(5, 1, 1080), 47.24388f);
    Near(nova::ui::aim::Radius(5, 2, 1080), 94.48776f);
    Near(nova::ui::aim::Radius(5, 1, 2160), 94.48776f);
    Near(nova::ui::aim::Radius(5, 0, 1080), 0);
    Near(nova::ui::aim::Radius(5, 1, 0), 0);
    sdk::aim::Transform t; sdk::aim::Vec3 out;
    t.position = {10, 20, 30}; t.scale = 2;
    CHECK(sdk::aim::TransformPoint({1, 2, 3}, t, out)); Near(out.x, 12); Near(out.y, 24); Near(out.z, 36);
    t.qz = t.qw = std::sqrt(.5f);
    CHECK(sdk::aim::TransformPoint({1, 0, 0}, t, out)); Near(out.x, 10); Near(out.y, 22);
    t.scale = 0; CHECK(!sdk::aim::TransformPoint({}, t, out)); t.scale = 1;
    t.qw = 10; CHECK(!sdk::aim::TransformPoint({}, t, out));
    using V = sdk::aim::Visibility;
    CHECK(sdk::aim::ClassifyTrace(1, 1, .1f, false) == V::Visible);
    CHECK(sdk::aim::ClassifyTrace(1, 0, 1, false) == V::Visible);
    CHECK(sdk::aim::ClassifyTrace(1, 2, .999f, false) == V::Blocked);
    CHECK(sdk::aim::ClassifyTrace(1, 0, .999f, false) == V::Blocked);
    CHECK(sdk::aim::ClassifyTrace(1, 1, 1, true) == V::Blocked);
    CHECK(sdk::aim::ClassifyTrace(1, 0, std::numeric_limits<float>::quiet_NaN(), false) == V::Unknown);
    CHECK(sdk::aim::ClassifyTrace(0, 0, 1, false) == V::Unknown);
    // Actual point-ray output: +0xBC is set on ordinary hits, not allSolid.
    std::array<unsigned char, 192> bytes{};
    const std::uintptr_t hit = 0x12345000; const float fraction = .25f;
    std::memcpy(bytes.data() + 8, &hit, sizeof(hit));
    std::memcpy(bytes.data() + 0xAC, &fraction, sizeof(fraction));
    bytes[0xBC] = 1;
    const auto decoded = sdk::aim::DecodeTrace(bytes);
    CHECK(decoded.hit == hit && decoded.fraction == fraction && !decoded.startSolid);
    CHECK(sdk::aim::ClassifyTrace(hit, decoded.hit, decoded.fraction, decoded.startSolid) == V::Visible);
    CHECK(sdk::aim::ClassifyTrace(hit + 1, decoded.hit, decoded.fraction, decoded.startSolid) == V::Blocked);
    bytes[0xBB] = 1;
    CHECK(sdk::aim::DecodeTrace(bytes).startSolid); // Still reject starting inside solid geometry.
}
void PrecisionTests() {
    input::AimPipeline pipeline;
    aim::Settings config{10, 720, 720};
    auto sample = Baseline();
    sdk::aim::Snapshot distant; distant.count = 1; distant.targets[0] = {0x8002, {16000, 20, 5}};
    Angle desired; CHECK(aim::Direction(distant.eye, distant.targets[0].point, desired));
    // Normal mode now corrects the first sniper shot without requiring RCS.
    pipeline.Reset(); pipeline.Configure({}, false, config, true);
    sample = Baseline(); sample.weapon.identity.item = 40; sample.punch = {2, 1, 0};
    sample.view = pipeline.Process(sample, distant); Advance(sample);
    sample.view = pipeline.Process(sample, distant);
    Near(sample.view.pitch + sample.punch.pitch, desired.pitch);
    Near(sample.view.yaw + sample.punch.yaw, desired.yaw);
    // A new recoil kick exceeds the small FOV but is not player aim motion.
    config.fov = .2f; config.horizontal = config.vertical = 30;
    pipeline.Configure({}, false, config, true);
    Advance(sample); sample.view = pipeline.Process(sample, distant);
    Advance(sample); sample.punch = {-4, 3, 0}; sample.view = pipeline.Process(sample, distant);
    CHECK(pipeline.Target() == 0x8002);
    Near(sample.view.pitch + pipeline.VisualOffset().pitch, desired.pitch);
    Near(sample.view.yaw + pipeline.VisualOffset().yaw, desired.yaw);
    const auto stable = sample.view;
    Near(pipeline.Process(sample, distant).pitch, stable.pitch);
    Advance(sample); sample.view.yaw += 1; pipeline.Process(sample, distant); CHECK(!pipeline.Target());

    aim::Controller c; sample = Baseline(); config = {}; config.fov = .15f;
    c.Process(sample, Snapshot(.2f), config, sample.view, {}); CHECK(!c.Target());
    Advance(sample); c.Process(sample, Snapshot(.1f), config, sample.view, {}); CHECK(c.Target());
    config.fov = 180; Advance(sample); c.Process(sample, Snapshot(170), config, sample.view, {}); CHECK(c.Target());
    config.fov = std::numeric_limits<float>::quiet_NaN(); CHECK(aim::Normalize(config).fov == 5);
    Near(nova::ui::aim::Radius(180, 1, 1080), 0); // A full sphere has no finite on-screen ring.

    std::array<sdk::aim::Candidate, 128> candidates{};
    for (unsigned i = 0; i < candidates.size(); ++i) candidates[i].handle = i + 1;
    unsigned visited = 0; sdk::aim::Snapshot collected;
    sdk::aim::CollectTargets(candidates, &visited, [](void* context, const sdk::aim::Candidate& c, sdk::aim::Target& target) {
        ++*static_cast<unsigned*>(context); target.point = {16000, 1, 0};
        return c.handle >= 10; // First nine occluded candidates cannot starve the rest.
    }, collected);
    CHECK(visited == 128 && collected.count == 119 && collected.targets[0].handle == 10);
}
struct Memory {
    std::unordered_map<std::uintptr_t, unsigned char> bytes;
    template<class T> void Put(std::uintptr_t address, T value) {
        const auto* data = reinterpret_cast<const unsigned char*>(&value);
        for (unsigned i = 0; i < sizeof(T); ++i) bytes[address + i] = data[i];
    }
    sdk::players::Memory Reader() {
        return {this, [](void* context, std::uintptr_t address, void* out, std::size_t size) {
            const auto& bytes = static_cast<Memory*>(context)->bytes;
            for (std::size_t i = 0; i < size; ++i) {
                auto found = bytes.find(address + i); if (found == bytes.end()) return false;
                static_cast<unsigned char*>(out)[i] = found->second;
            }
            return true;
        }};
    }
};
void CandidateTests() {
    namespace p = sdk::players::offsets;
    constexpr std::uintptr_t world = 0x10000000, chunk = 0x20000000, pawn = 0x30000000, scene = 0x40000000;
    constexpr std::uint32_t handle = 0x8002; const auto slot = chunk + 2 * p::stride;
    Memory m; m.Put(world + 0x10, chunk); m.Put(slot, pawn); m.Put(slot + 0x10, handle); m.Put(slot + 0x30, 0u);
    m.Put(pawn + p::identity, slot); m.Put(pawn + p::health, 100); m.Put(pawn + p::lifeState, std::uint8_t{0});
    m.Put(pawn + p::team, std::uint8_t{2}); m.Put(pawn + sdk::aim::offsets::immunity, std::uint8_t{0});
    m.Put(pawn + p::sceneNode, scene); m.Put(scene + p::dormant, std::uint8_t{0}); m.Put(scene + p::origin, sdk::aim::Vec3{1, 2, 3});
    sdk::aim::Candidate out; auto valid = [&] { return sdk::aim::ReadCandidate(m.Reader(), world, handle, out); };
    CHECK(valid() && out.pawn == pawn && out.team == 2 && out.handle == handle);
    CHECK(!sdk::aim::ReadCandidate(m.Reader(), world, handle + 0x8000, out));
    m.Put(slot + 0x10, handle + 0x8000); m.Put(slot + 0x30, 1u); CHECK(valid());
    m.Put(pawn + p::identity, slot + 8); CHECK(!valid()); m.Put(pawn + p::identity, slot);
    m.Put(pawn + p::health, 0); CHECK(!valid()); m.Put(pawn + p::health, 100);
    for (auto address : {pawn + p::lifeState, scene + p::dormant, pawn + sdk::aim::offsets::immunity}) {
        m.Put(address, std::uint8_t{1}); CHECK(!valid()); m.Put(address, std::uint8_t{0});
    }
    m.Put(pawn + p::team, std::uint8_t{1}); CHECK(!valid()); m.Put(pawn + p::team, std::uint8_t{3}); CHECK(valid());
    m.Put(scene + p::origin, sdk::aim::Vec3{std::numeric_limits<float>::quiet_NaN(), 0, 0}); CHECK(!valid());
    CHECK(!sdk::aim::ResolveCapture()); // No game loaded in test process.
}
void PipelineTests() {
    input::AimPipeline pipeline; aim::Settings aimConfig{30, 120, 90}; aimConfig.compensateRecoil = false;
    pipeline.Configure({}, false, aimConfig, true); auto s = Baseline(); const auto targets = Snapshot(20);
    s.view = pipeline.Process(s, targets); Advance(s); s.view = pipeline.Process(s, targets); Near(s.view.yaw, 1.2f);
    const auto first = s; Near(pipeline.Process(s, targets).yaw, 1.2f); // repeated output
    Advance(s); s.view = pipeline.Process(s, targets); Near(s.view.yaw, 2.4f);
    auto replay = first; replay.view = {}; Near(pipeline.Process(replay, targets).yaw, 1.2f); // raw replay
    // A producer may reuse frame/ordinal with a new timestamp; it isn't a replay.
    replay.view = pipeline.Process(replay, targets); replay.time += .01;
    replay.view = pipeline.Process(replay, targets); Near(replay.view.yaw, 2.4f);
    pipeline.Configure({}, false, {30, 0, 0}, true);
    Near(pipeline.Process(replay, targets).yaw, 1.2f); // Re-evaluate that raw sample with changed settings.
    Advance(replay); replay.view.yaw = 7;
    Near(pipeline.Process(replay, targets).yaw, 7); // No stale turn after settings change.
    pipeline.Reset(); pipeline.Configure({100, 100, 2, 0}, true, aimConfig, true);
    s = Baseline(); s.view = pipeline.Process(s, Snapshot(0)); s.attack = true;
    auto shot = [&](float punch) {
        Advance(s); ++s.weapon.shots; --s.weapon.clip; s.weapon.lastShot = static_cast<float>(s.time); s.punch.pitch = punch;
    };
    shot(1); s.view = pipeline.Process(s, Snapshot(0)); Near(s.view.pitch, 0); // Start Bullet=2
    shot(3); s.view = pipeline.Process(s, Snapshot(0)); Near(s.view.pitch, -2); // Aim must not cancel RCS.
    Near(pipeline.Process(s, Snapshot(0)).pitch, -2); // Nor apply either twice.
    shot(4); s.view = pipeline.Process(s, {}); Near(s.view.pitch, -3); // Lost target leaves RCS alone.
    pipeline.Configure({100, 100, 2, 0}, true, aimConfig, false);
    shot(5); s.view = pipeline.Process(s, {}); Near(s.view.pitch, -4);
    pipeline.Configure({100, 100, 2, 0}, true, aimConfig, true);
    shot(6); s.view = pipeline.Process(s, Snapshot(0)); Near(s.view.pitch, -5);
    shot(7); s.view = pipeline.Process(s, Snapshot(0)); Near(s.view.pitch, -6);
    // Match standalone RCS smoothing while a target remains at the baseline angle.
    pipeline.Reset(); pipeline.Configure({100, 100, 1, 50}, true, aimConfig, true);
    recoil::Controller reference; reference.Configure({100, 100, 1, 50});
    s = Baseline(); auto standalone = s; pipeline.Process(s, Snapshot(0)); reference.Process(standalone); s.attack = true;
    for (int i = 0; i < 20; ++i) {
        shot(10); standalone = s;
        s.view = pipeline.Process(s, Snapshot(0)); standalone.view = reference.Process(standalone);
        Near(s.view.pitch, standalone.view.pitch);
    }
}
Sample live;
sdk::aim::Query lastQuery;
void* caller = nullptr;
unsigned captures = 0;
bool throwAim = false, allowSample = true, allowTargets = true;
#pragma optimize("", off)
__declspec(noinline) std::uintptr_t __fastcall GameCommit(void* context, unsigned slot, Angle* angle) {
    caller = _ReturnAddress();
    return reinterpret_cast<std::uintptr_t>(context) + slot + (angle ? 123 : 0);
}
using Commit = std::uintptr_t (__fastcall*)(void*, unsigned, Angle*);
Commit volatile invoke = GameCommit;
__declspec(noinline) void FinalInput(Angle* angle) { invoke(nullptr, 0, angle); }
__declspec(noinline) void OtherInput(Angle* angle) { invoke(reinterpret_cast<void*>(100), 0, angle); }
#pragma optimize("", on)
bool CaptureSample(Angle* angle, Sample& sample) { sample = live; sample.view = *angle; return allowSample; }
bool CaptureAim(const Sample&, sdk::aim::Query query, sdk::aim::Snapshot& snapshot) {
    ++captures; lastQuery = query;
    if (throwAim) RaiseException(0xE0424242, 0, 0, nullptr);
    snapshot = allowTargets ? Snapshot(20) : sdk::aim::Snapshot{}; return true;
}
void HookTests() {
    CHECK(MH_Initialize() == MH_OK);
    live = Baseline(); auto angle = live.view; FinalInput(&angle);
    const sdk::weapons::Binding binding{reinterpret_cast<void*>(&GameCommit), caller, CaptureSample};
    CHECK(hooks::input::Install(binding, CaptureAim)); CHECK(hooks::input::AimAvailable());
    hooks::input::PublishAim({30, 120, 90}, true, true, true, 0);
    FinalInput(&angle); Advance(live); FinalInput(&angle); CHECK(captures == 0); Near(angle.yaw, 0); // Unbound Hold is idle.
    hooks::input::PublishAim({30, 120, 90}, true, true, false, 0);
    FinalInput(&angle); Advance(live); FinalInput(&angle); Near(angle.yaw, 1.2f); CHECK(lastQuery.ignoreTeam);
    CHECK(lastQuery.preferred == 0x8002); FinalInput(&angle); Near(angle.yaw, 1.2f);
    Advance(live); OtherInput(&angle); Near(angle.yaw, 1.2f); FinalInput(&angle); Near(angle.yaw, 2.4f);
    allowSample = false; Advance(live); FinalInput(&angle); Near(angle.yaw, 2.4f);
    allowSample = true; Advance(live); FinalInput(&angle); Near(angle.yaw, 2.4f); // Capture invalidation resets baseline.
    modules::ModuleManager manager; auto& module = manager.Create<modules::Aimbot>(); manager.Seal();
    CHECK(module.category() == "combat" && !module.requestedEnabled() && module.keybind().hold());
    CHECK(module.keybind().keybind() == 0 && module.settings().size() == 11);
    CHECK(module.suffix().empty());
    Properties::BoolProperty* compensation = nullptr;
    Properties::ModeProperty* priority = nullptr;
    Properties::NumberProp* smooth = nullptr;
    for (const auto& row : module.settings()) {
        CHECK(std::string_view(row.getName()) != "Mode" && std::string_view(row.getName()) != "Auto Fire");
        if (std::string_view(row.getName()) == "Recoil Compensation")
            compensation = dynamic_cast<Properties::BoolProperty*>(row.getProp());
        if (std::string_view(row.getName()) == "Aim Priority") priority = dynamic_cast<Properties::ModeProperty*>(row.getProp());
        if (std::string_view(row.getName()) == "Smooth") smooth = dynamic_cast<Properties::NumberProp*>(row.getProp());
    }
    CHECK(compensation && compensation->get());
    CHECK(priority && priority->get() == 1 && smooth && smooth->get() == 0);
    auto oldConfig = nova::json::Value::Object();
    auto& oldRows = oldConfig["modules"]["Aimbot"]["settings"];
    oldRows["Mode"]["value"] = "Rage";
    oldRows["Mode"]["sub"]["Auto Fire"] = true;
    oldRows["Mode"]["sub"]["Recoil Compensation"] = false;
    oldRows["Aim Point"] = "Head";
    manager.Load(oldConfig); CHECK(!compensation->get());
    CHECK(priority->get() == 0);
    oldRows["Recoil Compensation"] = true;
    oldRows["Aim Priority"] = "Pelvis";
    manager.Load(oldConfig); CHECK(compensation->get());
    CHECK(priority->get() == 2);
    smooth->set(125);
    auto migrated = nova::json::Value::Object(); manager.Save(migrated);
    CHECK(!migrated["modules"]["Aimbot"]["settings"].Find("Mode"));
    smooth->set(0); priority->set(0); manager.Load(migrated);
    CHECK(smooth->get() == 125 && priority->get() == 2);
    Properties::NumberProp* fov = nullptr;
    Properties::BoolProperty *circle = nullptr, *onlyClick = nullptr;
    for (const auto& row : module.settings()) {
        if (std::string_view(row.getName()) == "Aim FOV") fov = dynamic_cast<Properties::NumberProp*>(row.getProp());
        if (std::string_view(row.getName()) == "Show FOV") circle = dynamic_cast<Properties::BoolProperty*>(row.getProp());
        if (std::string_view(row.getName()) == "Only While Clicking") onlyClick = dynamic_cast<Properties::BoolProperty*>(row.getProp());
    }
    CHECK(circle && circle->get() && onlyClick && onlyClick->get());
    circle->set(false); onlyClick->set(false);
    CHECK(fov && fov->get() == 5); fov->set(12); module.setEnabled(true);
    auto saved = nova::json::Value::Object(); manager.Save(saved); fov->set(4); circle->set(true); onlyClick->set(true); manager.Load(saved);
    CHECK(fov->get() == 12 && !module.requestedEnabled()); // Hold activity is never persisted.
    CHECK(!circle->get() && !onlyClick->get());
    module.keybind().setHold(false); module.setEnabled(true); manager.Tick(); manager.Save(saved);
    module.setEnabled(false); manager.Tick(); manager.Load(saved); manager.Tick(); CHECK(module.requestedEnabled());
    hooks::input::Publish({100, 100, 1, 0}, true); FinalInput(&angle);
    throwAim = true; Advance(live); FinalInput(&angle); throwAim = false;
    CHECK(!hooks::input::AimAvailable() && hooks::input::Available()); // Aim fault doesn't stop RCS.
    const float before = angle.pitch;
    live.attack = true; ++live.weapon.shots; --live.weapon.clip; live.punch.pitch = 2;
    Advance(live); live.weapon.lastShot = static_cast<float>(live.time); FinalInput(&angle); Near(angle.pitch, before - 2);
    manager.Tick(); CHECK(!module.requestedEnabled());
    CHECK(hooks::input::Stop(1000)); hooks::input::Finalize(); manager.Shutdown();
    CHECK(hooks::input::Install(binding, CaptureAim)); CHECK(hooks::input::AimAvailable());
    const auto previous = angle; Advance(live); FinalInput(&angle); Near(angle.yaw, previous.yaw);
    CHECK(hooks::input::Stop(1000)); hooks::input::Finalize(); CHECK(MH_Uninitialize() == MH_OK);
}
}
void TestAim() {
    MathTests(); SelectionTests(); BodyPriorityTests(); SmoothingTests(); GeometryTests(); PrecisionTests(); CandidateTests(); PipelineTests(); HookTests();
    puts("PASS: body priority/fallback and visibility, bounded smoothing across FPS, targeting, recoil/FOV consistency, config migration, input gating, replay, fault isolation and reinstall; no game injection");
}

