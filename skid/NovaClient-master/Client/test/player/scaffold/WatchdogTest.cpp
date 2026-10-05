#include <cassert>
#include <iostream>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include "minecraft/utils/rotation/Rotation.h"
// @HYPIXEL_HELPERS@
#include "minecraft/utils/rotation/RotationUtils.h"

namespace nova::rotation { double Random() { return 0.5; } }

using namespace nova::module::scaffold;
using nova::rotation::Rotation;

int main() {
    assert(SnapTellyYaw(-22.5f) == 0);
    assert(SnapTellyYaw(22.5f) == 45);

    Rotation picked{};
    // Score all valid candidates instead of selecting the first (45 degrees).
    assert(FindWatchdogPlacement(0, 90, {90, 82}, false,
                                 [](Rotation) { return true; }, picked));
    assert(picked.yaw == 90 && picked.pitch == 82);
    assert(FindWatchdogPlacement(0, 90, {90, 82}, false,
        [](Rotation r) { return r.pitch == 83.5f; }, picked));
    assert(picked.yaw == 90 && picked.pitch == 83.5f);

    int rays = 0;
    assert(!FindWatchdogPlacement(0, 90, {90, 82}, false,
        [&](Rotation) { ++rays; return false; }, picked));
    assert(rays == 28);
    rays = 0;
    assert(!FindWatchdogPlacement(0, 0, {}, false,
        [&](Rotation) { ++rays; return false; }, picked));
    assert(rays == 21); // The movement-derived yaw duplicates the first candidate.

    assert(FindWatchdogPlacement(0, 90, {90, 82}, true,
                                 [](Rotation) { return true; }, picked));
    assert(std::fabs(std::remainder(picked.yaw, 90.0f)) == 45);

    const auto wrap = StepTellyRotation({179, 85}, {-179, 85}, 35);
    assert(wrap.yaw == 181 && wrap.pitch == 85);
    const auto axes = StepTellyRotation({0, 0}, {90, 82}, 36);
    assert(axes.yaw == 36 && axes.pitch == 36);
    const auto fast = StepTellyRotation({0, 0}, {90, 82}, 125);
    assert(fast.yaw == 90 && fast.pitch == 82);
    // The shared manager must not shrink the already limited axes a second time.
    const auto effective = nova::rotation::Smooth({0, 0}, axes, 360, {0, 0}, 0.5, 120, true);
    assert(std::fabs(effective.yaw - 36) < 0.2f && std::fabs(effective.pitch - 36) < 0.2f);
    const Rotation still = nova::rotation::Move({180, 82}, {-180, 82}, 35);
    assert(still.yaw == 0 && still.pitch == 0);
    const Rotation smooth = nova::rotation::Smooth({-135, 82}, {-135, 82}, 35,
                                                   {-135, 82}, 0.5, 120, true);
    assert(std::isfinite(smooth.yaw) && std::isfinite(smooth.pitch));
    const Rotation step = nova::rotation::Move({0, 0}, {90, 60}, 35);
    assert(std::fabs(std::hypot(step.yaw, step.pitch) - 35) < 0.001);
    std::cout << "PASS: Watchdog candidate scoring, pitch search, angle wrapping, independent axis steps\n";
}
