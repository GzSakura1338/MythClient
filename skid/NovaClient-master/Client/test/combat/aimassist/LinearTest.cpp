#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <utility>
#include <Windows.h>
#include "minecraft/utils/rotation/LinearAim.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/RotationUtils.h"
#include "minecraft/utils/rotation/Mth.h"
#include "minecraft/modules/combat/aimassist/AimTargetSelection.h"
#include "minecraft/modules/combat/aimassist/AimPoint.h"

namespace mock { int randomCalls = 0, nonlinearCalls = 0, picks = 0, cameraCorrections = 0; float yaw = 0, pitch = 0; }
namespace nova::java {
bool FreeCamActive() { return false; }
bool SetPlayerRotation(float yaw, float pitch) { mock::yaw = yaw; mock::pitch = pitch; return true; }
bool SetPlayerCameraRotation(float yaw, float pitch, double) {
    ++mock::cameraCorrections; return SetPlayerRotation(yaw, pitch);
}
bool PlayerPreviousRotation(float& yaw, float& pitch) { yaw = pitch = 0; return true; }
bool PlayerSensitivity(double& value) { value = .5; return true; }
int ClientFps() { return 60; }
int PlayerTickCount() { return 10; }
void PickCursor(float) { ++mock::picks; }
}
namespace nova::rotation {
RotationManagerState g_state;
const void* exclusiveOwner=nullptr;
bool PlayerRotation(float& yaw, float& pitch) { yaw = mock::yaw; pitch = mock::pitch; return true; }
float ClampPitch(float value) { return std::clamp(value, -90.0f, 90.0f); }
void ClearRaycast() { g_state.raycast = nullptr; g_state.hasRaycast = false; }
double Random() { ++mock::randomCalls; return .5; }
double ToRadians(double value) { return value * .017453292519943295; }
double ToDegrees(double value) { return value * 57.29577951308232; }
Rotation Smooth(const Rotation&, const Rotation& target, double, const Rotation&, double, int, bool) {
    ++mock::nonlinearCalls; return target;
}
// @PRODUCTION_ROTATION_METHODS@
}

void Near(double a, double b, double tolerance = .001) { assert(std::abs(a - b) < tolerance); }
int main() {
    using namespace nova::rotation;
    using nova::module::aimassist::PointInFov;
    const Vec3 origin{};
    assert(PointInFov({0, 0}, origin, {0, 0, 3}, 15));
    assert(PointInFov({0, 0}, origin, {1, 0, 3}, 90));
    assert(!PointInFov({0, 0}, origin, {1, 0, 3}, 15)); // Live FOV shrink drops a target.
    assert(!PointInFov({0, 0}, origin, {0, 3, 3}, 30)); // Same yaw, outside vertical FOV.
    assert(PointInFov({0, -45}, origin, {0, 3, 3}, 15));
    assert(!PointInFov({0, 0}, origin, {1, 1, std::sqrt(2.0)}, 80)); // Diagonal cone edge.
    assert(PointInFov({0, 0}, origin, {1, 1, std::sqrt(2.0)}, 90));
    assert(PointInFov({179, 0}, origin, {.01, 0, -3}, 15));
    assert(PointInFov({-541, 0}, origin, {.01, 0, -3}, 15)); // Unbounded player yaw.
    assert(!PointInFov({0, 0}, origin, {0, 0, -3}, 359));
    assert(PointInFov({0, 0}, origin, {0, 0, -3}, 360));
    assert(!PointInFov({0, 0}, origin, {NAN, 0, 3}, 360));
    assert(!PointInFov({0, 0}, origin, origin, 90));
    // Body preference and tracking both use the exact point, rather than feet.
    assert(!PointInFov({0, 0}, {0, 1.62, 0}, {0, 0, 3}, 15));
    assert(PointInFov({0, 0}, {0, 1.62, 0}, {0, 1.62, 3}, 15));
    assert(!PointInFov({30, 0}, {0, 1.62, 0}, {0, 1.62, 3}, 15)); // Camera moved within the same tick.
    std::puts("AimAssist FOV: live size/camera changes, vertical/diagonal bounds, body points and yaw wrap passed");
    auto axis = AxisLinearStep({0, 0}, {90, 60}, 3, 1);
    Near(axis.yaw, 3); Near(axis.pitch, 1);
    axis = AxisLinearStep({0, 0}, {90, 60}, 0, 1);
    Near(axis.yaw, 0); Near(axis.pitch, 1);
    axis = AxisLinearStep({0, 0}, {90, 60}, 3, 0);
    Near(axis.yaw, 3); Near(axis.pitch, 0);
    axis = AxisLinearStep({179, 89}, {-179, 120}, 10, 10);
    Near(axis.yaw, 181); Near(axis.pitch, 90);
    axis = AxisLinearStep({0, 0}, {-90, -60}, 3, 1);
    Near(axis.yaw, -3); Near(axis.pitch, -1);
    for (int fps : {30, 60, 144, 240}) {
        AimFrameClock clock;
        clock.Advance(0);
        Rotation camera{};
        for (int i = 1; i <= fps / 2; ++i) {
            const auto dt = clock.Advance(double(i) / fps);
            camera = AxisLinearStep(camera, {170, 80}, 180 * dt, 60 * dt);
        }
        Near(camera.yaw, 90); Near(camera.pitch, 30);
        const auto dt = clock.Advance(10);
        camera = AxisLinearStep(camera, {170, 80}, 180 * dt, 60 * dt);
        Near(camera.yaw, 90); Near(camera.pitch, 30);
    }
    std::puts("Normal aim: independent axis speeds, zero speeds, yaw wrap, frame rates and stalls passed");
    using nova::module::aimassist::SmoothRenderOffset;
    auto offset = SmoothRenderOffset({1, 0, 0}, {0, 0, 0}, .5, 100);
    Near(offset.x, -.5);
    Near(SmoothRenderOffset({1, 0, 0}, {0, 0, 0}, .5, 50).x, -.25);
    Near(SmoothRenderOffset({1, 0, 0}, {0, 0, 0}, .5, 0).x, 0);
    Near(SmoothRenderOffset({10, 0, 0}, {0, 0, 0}, .5, 100).x, 0); // Teleport: no trail.
    Near(SmoothRenderOffset({1, 0, 0}, {0, 0, 0}, NAN, 100).x, 0);
    // Full smoothing removes the position discontinuity at a game tick boundary.
    const double end = 1 + SmoothRenderOffset({1, 0, 0}, {0, 0, 0}, 1, 100).x;
    const double start = 2 + SmoothRenderOffset({2, 0, 0}, {1, 0, 0}, 0, 100).x;
    Near(start, end);
    for (double partial : {0.0, .1, .5, .9, 1.0}) {
        const Rotation previous{10, 5}, current{12, 6}, next{15, 7};
        const auto shifted = CameraPreviousAfterTurn(previous, current, next, 100);
        const double beforeYaw = previous.yaw + (current.yaw - previous.yaw) * partial;
        const double afterYaw = shifted.yaw + (next.yaw - shifted.yaw) * partial;
        Near(afterYaw - beforeYaw, 3); // Full camera delta at every render fraction.
        const double beforePitch = previous.pitch + (current.pitch - previous.pitch) * partial;
        const double afterPitch = shifted.pitch + (next.pitch - shifted.pitch) * partial;
        Near(afterPitch - beforePitch, 1);
    }
    Near(CameraPreviousAfterTurn({10, 5}, {12, 6}, {15, 7}, 0).yaw, 10);
    Near(CameraPreviousAfterTurn({10, 5}, {12, 6}, {15, 7}, 50).yaw, 11.5);
    for (int fps : {30, 60, 144, 240}) {
        for (double amount : {0.0, 50.0, 100.0}) {
            Rotation camera{}, previous{};
            for (int i = 0; i < fps / 2; ++i) {
                const auto next = AxisLinearStep(camera, {170, 80}, 180.0 / fps, 60.0 / fps);
                previous = CameraPreviousAfterTurn(previous, camera, next, amount);
                camera = next;
            }
            Near(camera.yaw, 90); Near(camera.pitch, 30); // Amount never reduces turn speed.
        }
    }
    std::puts("Smooth: render continuity, adjustable interpolation, teleports and unchanged axis speeds passed");
    auto r = LinearStep({0, 0}, {30, 40}, 10);
    Near(r.yaw, 6); Near(r.pitch, 8); // Combined angular speed, not one speed per axis.
    r = LinearStep({179, 0}, {-179, 0}, 1);
    Near(r.yaw, 180);
    r = LinearStep(r, {-179, 0}, 10);
    Near(r.yaw, 181); // Shortest path and no overshoot at the wrap boundary.
    r = LinearStep({0, 89}, {0, 120}, 50); Near(r.pitch, 90);
    r = LinearStep({12, 3}, {150, 30}, 0); Near(r.yaw, 12); Near(r.pitch, 3);
    r = LinearStep({12, 3}, {std::numeric_limits<float>::quiet_NaN(), 30}, 20);
    Near(r.yaw, 12);
    // Same real time produces the same camera angle at different frame rates.
    for (const int fps : {30, 60, 144, 240}) {
        AimFrameClock clock;
        Near(clock.Advance(0), 0);
        Rotation current{};
        for (int frame = 1; frame <= fps / 2; ++frame)
            current = LinearStep(current, {170, 0}, 180 * clock.Advance(double(frame) / fps));
        Near(current.yaw, 90);
        Near(clock.Advance(10), 0); // Stall cannot bank nine seconds of rotation.
        clock.Reset(); Near(clock.Advance(20), 0);
        Near(clock.Advance(19), 0); // Clock rewind cannot reverse the aim.
    }
    // Every step keeps the same length until only the final shorter step remains.
    r = {};
    for (int i = 0; i < 9; ++i) {
        const auto next = LinearStep(r, {95, 0}, 10);
        Near(next.yaw - r.yaw, 10); r = next;
    }
    r = LinearStep(r, {95, 0}, 10); Near(r.yaw, 95);
    r = LinearStep(r, {95, 0}, 10); Near(r.yaw, 95);

    // Silent uses the same independent budgets as Normal, integrated at 20 TPS.
    g_state = {};
    assert(SetLinearRotations({90, 60}, 180, 60));
    Near(g_state.rotations.yaw, 9); Near(g_state.rotations.pitch, 3);
    assert(SetLinearRotations({90, 60}, 720, 720));
    Near(g_state.rotations.yaw, 9); Near(g_state.rotations.pitch, 3); // No second budget in one tick.
    float axisWireYaw=0, axisWirePitch=0;
    OnMotionPre(axisWireYaw, axisWirePitch);
    assert(SetLinearRotations({90, 60}, 0, 60));
    Near(g_state.rotations.yaw, 9); Near(g_state.rotations.pitch, 6);
    OnMotionPre(axisWireYaw, axisWirePitch);
    assert(SetLinearRotations({90, 60}, 180, 0));
    Near(g_state.rotations.yaw, 18); Near(g_state.rotations.pitch, 6);
    OnMotionPre(axisWireYaw, axisWirePitch);
    assert(SetLinearRotations({90, 60}, 0, 0));
    Near(g_state.rotations.yaw, 18); Near(g_state.rotations.pitch, 6);
    assert(!SetLinearRotations({90, 60}, -1, 60));
    assert(!SetLinearRotations({90, 60}, 60, NAN));
    OnMotionPre(axisWireYaw, axisWirePitch);
    ReturnLinearRotations(60);
    const auto beforeReturn=g_state.rotations;
    UpdateGlobalYaw(); OnMotionPre(axisWireYaw, axisWirePitch);
    Near(std::hypot(beforeReturn.yaw-axisWireYaw, beforeReturn.pitch-axisWirePitch), 3);
    // Return still moves both axes even when their tracking speeds were zero.
    for(int i=0; i<10 && g_state.active; ++i) {
        UpdateGlobalYaw(); OnMotionPre(axisWireYaw, axisWirePitch);
    }
    assert(!g_state.active); Near(axisWireYaw, 0); Near(axisWirePitch, 0);
    g_state = {};
    assert(SetLinearRotations({170, 0}, 180, 180));
    Near(g_state.rotations.yaw, 9);
    assert(SetLinearRotations({170, 0}, 180, 180));
    Near(g_state.rotations.yaw, 9); // Extra render callbacks cannot spend a tick twice.
    for (int i = 0; i < 9; ++i) {
        g_state.lastRotations = g_state.rotations;
        g_state.smoothed = false;
        assert(SetLinearRotations({170, 0}, 180, 180));
    }
    Near(g_state.rotations.yaw, 90); // Silent: same half-second as camera integration.
    assert(mock::randomCalls == 0 && mock::nonlinearCalls == 0);
    ReleaseLinearRotations();
    assert(!g_state.active && !g_state.linear && !g_state.hasRotations && g_state.moveInputFix);

    g_state = {};
    g_state.active = true; g_state.priority = RotationPriority::Highest;
    g_state.rotations = {42, 0};
    assert(!SetLinearRotations({80, 0}, 180, 180));
    ReleaseLinearRotations();
    assert(g_state.active); Near(g_state.rotations.yaw, 42);

    g_state = {};
    assert(SetLinearRotations({80, 0}, 180, 180));
    g_state.lastRotations = g_state.rotations; g_state.smoothed = false;
    SetRotations({100, 0}, 10, nullptr, RotationPriority::Medium);
    assert(!g_state.linear && mock::nonlinearCalls == 1);
    ReleaseLinearRotations();
    assert(g_state.active); // Releasing AimAssist must not cancel another module's request.
    g_state = {};
    const int callsBeforeDirect = mock::nonlinearCalls;
    assert(SetDirectRotations({20, 70}));
    assert(g_state.hasLastRotations && g_state.direct && !g_state.linear);
    g_state.smoothed = false; Smooth();
    Near(g_state.rotations.yaw, 20); Near(g_state.rotations.pitch, 70);
    assert(mock::nonlinearCalls == callsBeforeDirect);
    g_state.priority = RotationPriority::Highest;
    assert(!SetDirectRotations({40, 75}));
    g_state.priority = RotationPriority::Medium;
    SetRotations({100, 0}, 10, nullptr, RotationPriority::Medium);
    ReleaseDirectRotations(); assert(g_state.active && !g_state.direct);
    assert(SetDirectRotations({30, 60})); ReleaseDirectRotations();
    assert(!g_state.active && !g_state.direct && !g_state.hasRotations);
    // Drive the actual player-tick/motion sequence, including detached returns
    // after the Aim module is disabled and no longer submits frame updates.
    g_state = {};
    assert(SetLinearRotations({90, 0}, 180, 180));
    float wireYaw = 0, wirePitch = 0;
    OnMotionPre(wireYaw, wirePitch); Near(wireYaw, 9);
    Near(g_state.targetRotations.yaw, 90); // Tracking never borrows the return target.
    ReturnLinearRotations(60);
    assert(g_state.active && g_state.linearReturning);
    for (float expected : {6.f, 3.f, 0.f}) {
        UpdateGlobalYaw(); OnMotionPre(wireYaw, wirePitch); Near(wireYaw, expected);
    }
    assert(!g_state.active && !g_state.linearReturning && !g_state.hasRotations);
    assert(mock::cameraCorrections == 0); // Returning never turns the real camera.

    g_state = {}; assert(SetLinearRotations({90, 0}, 180, 180)); OnMotionPre(wireYaw, wirePitch);
    ReturnLinearRotations(60); mock::yaw = 30;
    UpdateGlobalYaw(); OnMotionPre(wireYaw, wirePitch); Near(wireYaw, 12);
    assert(SetLinearRotations({90, 0}, 360, 360));
    assert(!g_state.linearReturning); OnMotionPre(wireYaw, wirePitch); Near(wireYaw, 30);
    ReturnLinearRotations(10);
    mock::yaw = 29.25f;
    UpdateGlobalYaw(); OnMotionPre(wireYaw, wirePitch); Near(wireYaw, 29.5);
    assert(g_state.active); // No old one-degree snap at a slow return speed.
    UpdateGlobalYaw(); OnMotionPre(wireYaw, wirePitch); Near(wireYaw, 29.25);
    assert(!g_state.active);

    g_state = {}; mock::yaw = 179;
    assert(SetLinearRotations({-179, 0}, 720, 720)); OnMotionPre(wireYaw, wirePitch);
    ReturnLinearRotations(20);
    UpdateGlobalYaw(); OnMotionPre(wireYaw, wirePitch); Near(wireYaw, 180);
    UpdateGlobalYaw(); OnMotionPre(wireYaw, wirePitch); Near(wireYaw, 179);
    assert(!g_state.active);
    mock::yaw = 0;
    assert(SetLinearRotations({90, 0}, 180, 180)); ReturnLinearRotations(60);
    SetRotations({50, 0}, 20, nullptr, RotationPriority::Highest);
    assert(!g_state.linear && !g_state.linearReturning);
    const auto other = g_state.rotations;
    ReturnLinearRotations(60); assert(g_state.active); Near(g_state.rotations.yaw, other.yaw);
    g_state = {}; assert(SetLinearRotations({90, 0}, 180, 180)); ReturnLinearRotations(60); OnRespawn();
    assert(!g_state.active && !g_state.linearReturning);
    std::puts("Silent: independent aim/return speeds, moving camera, reacquire, slow completion, yaw wrap and ownership passed");

    // Regression: a disabled Silent Aim may still be returning when Scaffold
    // takes over. Exercise the actual input and movement-yaw handlers together.
    auto forwardMotion = [] {
        MoveInput input{1, 0};
        OnMoveInput(input);
        float yaw = mock::yaw;
        OnStrafe(yaw);
        return input.forward * std::cos(ToRadians(yaw)) +
               input.strafe * std::sin(ToRadians(yaw));
    };
    auto scaffoldTick = [&] {
        SetRotations({180, 80}, 180, nullptr);
        UpdateGlobalYaw();
        assert(g_state.active && g_state.moveInputFix);
        Near(forwardMotion(), 1);
        OnMotionPre(wireYaw, wirePitch);
    };
    for (bool returnBeforeTakeover : {false, true}) {
        g_state = {}; mock::yaw = mock::pitch = 0;
        assert(SetLinearRotations({90, 0}, 670, 100));
        SetMoveInputFix(false); // Aim: Keep move direction OFF.
        OnMotionPre(wireYaw, wirePitch);
        if (returnBeforeTakeover) ReturnLinearRotations(260);
        scaffoldTick();
        ReturnLinearRotations(260);
        ReleaseLinearRotations(); // The old owner cannot release Scaffold.
        assert(g_state.active && !g_state.linear && g_state.moveInputFix);
        for (int tick = 0; tick < 200; ++tick) scaffoldTick();
        SetActive(false); UpdateGlobalYaw();
        scaffoldTick(); // Disabling/re-enabling no longer inherits the leak.
    }
    g_state = {};
    assert(SetDirectRotations({180, 80}));
    SetMoveInputFix(false); // AntiKB Prediction deliberately uses raw keys.
    Near(forwardMotion(), -1);
    OnMotionPre(wireYaw, wirePitch);
    scaffoldTick();
    ReleaseDirectRotations();
    assert(g_state.active && g_state.moveInputFix);

    // Direct callers such as BridgeAssist also get their own default policy,
    // including a handoff from a different direct caller.
    for (bool previousDirect : {false, true}) {
        g_state = {};
        if (previousDirect) assert(SetDirectRotations({90, 0}));
        else assert(SetLinearRotations({90, 0}, 670, 100));
        SetMoveInputFix(false);
        assert(SetDirectRotations({180, 80}));
        assert(g_state.moveInputFix); Near(forwardMotion(), 1);
    }

    // Breaker opts out after its accepted request; rejected/obsolete owners
    // must not erase that choice or cancel the current rotation.
    g_state = {}; UpdateGlobalYaw();
    SetRotations({180, 80}, 360, nullptr, RotationPriority::Highest);
    SetMoveInputFix(false);
    Near(forwardMotion(), -1);
    SetRotations({0, 0}, 180, nullptr, RotationPriority::Medium);
    assert(!SetLinearRotations({0, 0}, 670, 100));
    assert(!SetDirectRotations({0, 0}));
    ReleaseLinearRotations(); ReleaseDirectRotations();
    assert(g_state.active && !g_state.moveInputFix);
    Near(forwardMotion(), -1);
    OnMotionPre(wireYaw, wirePitch);
    scaffoldTick();

    // Silent Aim can still choose either policy on each accepted update.
    g_state = {};
    assert(SetLinearRotations({180, 0}, 3600, 100));
    SetMoveInputFix(false); Near(forwardMotion(), -1);
    assert(!SetLinearRotations({NAN, 0}, 670, 100));
    assert(!SetDirectRotations({NAN, 0}));
    assert(!g_state.moveInputFix);
    assert(SetLinearRotations({180, 0}, 3600, 100));
    assert(g_state.moveInputFix); Near(forwardMotion(), 1);
    SetMoveInputFix(false);
    ReturnLinearRotations(260);
    assert(!g_state.moveInputFix); // Return retains Aim's selected policy.
    for (int tick = 0; tick < 100 && g_state.linear; ++tick) {
        UpdateGlobalYaw(); OnMotionPre(wireYaw, wirePitch);
    }
    assert(!g_state.active && g_state.moveInputFix);

    for (bool direct : {false, true}) {
        g_state = {}; UpdateGlobalYaw();
        if (direct) assert(SetDirectRotations({180, 80}));
        else SetRotations({180, 80}, 180, nullptr);
        SetMoveInputFix(false); OnRespawn();
        assert(g_state.moveInputFix);
    }
    std::puts("Movement ownership: Aim/direct -> Scaffold, 200 ticks, re-enable, respawn, rejected requests and raw-key modes passed");
    int rescue=0,otherOwner=0;
    g_state={};mock::yaw=0;mock::pitch=0;
    assert(SetExclusiveRotations(&rescue,{90,90}));
    assert(!SetExclusiveRotations(&otherOwner,{0,0}));
    assert(!SetLinearRotations({0,0},180,180));
    assert(!SetDirectRotations({0,0}));
    SetRotations({0,0},180,nullptr,RotationPriority::Highest);
    SetMoveInputFix(false);ReleaseLinearRotations();ReleaseDirectRotations();
    assert(g_state.moveInputFix&&g_state.active);
    Near(g_state.rotations.yaw,90);Near(g_state.rotations.pitch,90);
    OnMotionPre(wireYaw,wirePitch);Near(wireYaw,90);Near(wirePitch,90);
    ReleaseExclusiveRotations(&otherOwner);assert(OwnsExclusiveRotations(&rescue));
    ReleaseExclusiveRotations(&rescue);assert(!OwnsExclusiveRotations(&rescue)&&g_state.linearReturning);
    for(int i=0;i<20&&g_state.active;++i){UpdateGlobalYaw();OnMotionPre(wireYaw,wirePitch);}
    assert(!g_state.active);
    assert(SetExclusiveRotations(&rescue,{0,0}));OnMotionPre(wireYaw,wirePitch);
    assert(g_state.active); // Matching the camera must not drop the rescue lease.
    OnRespawn();assert(!OwnsExclusiveRotations(&rescue)&&!g_state.active);
    std::puts("AutoMLG rotation lease: competition, movement fix, published angle, return and respawn passed");

    nova::module::aimassist::TargetSelection targets;
    assert(targets.Choose(3, 3, false, 1000, 250) == 0);
    assert(targets.Choose(3, 2, false, 5000, 250) == 2); // Single follows identity after re-sort.
    assert(targets.Choose(3, 3, false, 5010, 250) == 0); // Lost target selects best eligible.
    assert(targets.Choose(3, 0, true, 5020, 250) == 0); // Mode change starts a fresh interval.
    assert(targets.Choose(3, 0, true, 5269, 250) == 0);
    assert(targets.Choose(3, 0, true, 5270, 250) == 1);
    assert(targets.Choose(3, 1, true, 5520, 250) == 2);
    assert(targets.Choose(3, 2, true, 5770, 250) == 0);
    assert(targets.Choose(1, 0, true, 9000, 0) == 0); // One target never indexes past the end.
    assert(targets.Choose(0, 0, true, 9100, 0) == 0); // Empty world resets timer.
    assert(targets.Choose(2, 2, true, 9200, 0) == 0);
    assert(targets.Choose(2, 0, true, 9201, 0) == 1); // Zero delay: once per selection tick.
    assert(targets.Choose(2, 1, true, 100, 250) == 1); // Time rollback does not spuriously switch.
    targets.Reset();
    assert(targets.Choose(2, 2, true, 200, 250) == 0);
    nova::java::Box box{0, 10, 0, .6, 11.8, .6};
    nova::rotation::Vec3 point;
    const nova::rotation::Vec3 eye{4, 15, -4};
    using nova::module::aimassist::PreferredPoint;
    assert(PreferredPoint(box, 0, eye, 0, 0, point)); Near(point.y, 11.62);
    assert(PreferredPoint(box, 1, eye, 0, 0, point)); Near(point.y, 11.17);
    assert(PreferredPoint(box, 2, eye, 0, 0, point)); Near(point.y, 10.45);
    assert(PreferredPoint(box, 2, eye, 100, 100, point));
    assert(point.y <= 10 + 1.8 * .42 + .0001 && point.x < box.maxX && point.z > box.minZ);
    box.maxY = 10.6; // Pose changes scale all body points into the actual box.
    assert(PreferredPoint(box, 0, eye, 0, 0, point)); Near(point.y, 10.54);
    box.maxY = box.minY;
    assert(!PreferredPoint(box, 0, eye, 0, 0, point));
    std::puts("AimAssist: linear rotation, Single/Switch and preferred body part checks passed");
}
