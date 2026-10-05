#pragma once
#include "cs2/sdk/players/Players.h"
#include <cmath>

namespace nova::cs2::sdk::players {
struct alignas(16) Transform { Vec3 position; float scale = 1; float qx = 0, qy = 0, qz = 0, qw = 1; };
static_assert(sizeof(Transform) == 32);
inline bool TransformPoint(Vec3 v, const Transform& t, Vec3& out) {
    const auto finite = [](Vec3 a) { return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z) &&
        std::abs(a.x) < 1000000 && std::abs(a.y) < 1000000 && std::abs(a.z) < 1000000; };
    const double norm = t.qx * t.qx + t.qy * t.qy + t.qz * t.qz + t.qw * t.qw;
    if (!finite(v) || !finite(t.position) || !std::isfinite(norm) || std::abs(norm - 1) > .02 ||
        !std::isfinite(t.scale) || t.scale <= 0 || t.scale > 10) return false;
    v = {v.x * t.scale, v.y * t.scale, v.z * t.scale};
    const Vec3 cross{2 * (t.qy * v.z - t.qz * v.y), 2 * (t.qz * v.x - t.qx * v.z), 2 * (t.qx * v.y - t.qy * v.x)};
    out = {t.position.x + v.x + t.qw * cross.x + t.qy * cross.z - t.qz * cross.y,
        t.position.y + v.y + t.qw * cross.y + t.qz * cross.x - t.qx * cross.z,
        t.position.z + v.z + t.qw * cross.z + t.qx * cross.y - t.qy * cross.x};
    return finite(out);
}
}
