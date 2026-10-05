#pragma once
#include "minecraft/utils/rotation/Rotation.h"
#include "minecraft/utils/rotation/Vec3.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace nova::module::pearlphase {
// Mio Phase.getFloatArray1759 / getDoubleArray1760. Keep Java Math.round
// semantics (floor(v + .5)), notably for negative half-degree headings.
inline rotation::Rotation Aim(const rotation::Vec3& position, float yaw,
                              float forward, float strafe, bool ladder) {
    if (forward == 0 && strafe == 0) forward = 1;
    if (forward != 0) {
        if (strafe > 0) yaw += forward > 0 ? -45.f : 45.f;
        else if (strafe < 0) yaw += forward > 0 ? 45.f : -45.f;
        strafe = 0;
        forward = forward > 0 ? 1.f : -1.f;
    }
    const double radians = yaw * std::numbers::pi / 180.0;
    const double dx = forward * 2.0 * -std::sin(radians) + strafe * 2.0 * std::cos(radians);
    const double dz = forward * 2.0 * std::cos(radians) + strafe * 2.0 * std::sin(radians);
    const double bx = std::floor(position.x), bz = std::floor(position.z);
    const double x = std::clamp(position.x + dx, bx, bx + 1);
    const double z = std::clamp(position.z + dz, bz, bz + 1);
    // Point back at the cell center, then reverse it by 180 degrees.
    const double raw = std::atan2(bz + .5 - z, bx + .5 - x) * 180.0 / std::numbers::pi - 90.0;
    const float wrapped = static_cast<float>(raw - std::floor((raw + 180.0) / 360.0) * 360.0);
    return {static_cast<float>(std::floor(wrapped + .5f) + 180.f),
            ladder ? 80.f : position.y - std::floor(position.y) > .1 ? 81.f : 85.f};
}
}
