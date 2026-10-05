#pragma once
#include <cmath>
#include <initializer_list>
#include "minecraft/utils/rotation/Vec3.h"

namespace nova::module::ghosthand {
// Raven BlockUtils.traverseBlocksAlongRay: visit full ray, including blocks
// behind the first hit, so a preferred bed can replace that first hit.
template<class Visit>
void Traverse(rotation::Vec3 start, rotation::Vec3 end, Visit visit) {
    for (double value : {start.x, start.y, start.z, end.x, end.y, end.z})
        if (!std::isfinite(value)) return;
    const int dx = static_cast<int>(std::floor(end.x));
    const int dy = static_cast<int>(std::floor(end.y));
    const int dz = static_cast<int>(std::floor(end.z));
    int x = static_cast<int>(std::floor(start.x));
    int y = static_cast<int>(std::floor(start.y));
    int z = static_cast<int>(std::floor(start.z));
    if (visit(x, y, z)) return;
    auto p = start;
    for (int remaining = 200; remaining >= 0; --remaining) {
        if (x == dx && y == dy && z == dz) return;
        const double bx = dx > x ? x + 1.0 : x;
        const double by = dy > y ? y + 1.0 : y;
        const double bz = dz > z ? z + 1.0 : z;
        const auto delta = end.Subtract(p);
        double tx = dx == x ? 999.0 : (bx - p.x) / delta.x;
        double ty = dy == y ? 999.0 : (by - p.y) / delta.y;
        double tz = dz == z ? 999.0 : (bz - p.z) / delta.z;
        if (tx == 0.0) tx = -1.e-4;
        if (ty == 0.0) ty = -1.e-4;
        if (tz == 0.0) tz = -1.e-4;
        int face;
        if (tx < ty && tx < tz) {
            face = dx > x ? 4 : 5;
            p = {bx, p.y + delta.y * tx, p.z + delta.z * tx};
        } else if (ty < tz) {
            face = dy > y ? 0 : 1;
            p = {p.x + delta.x * ty, by, p.z + delta.z * ty};
        } else {
            face = dz > z ? 2 : 3;
            p = {p.x + delta.x * tz, p.y + delta.y * tz, bz};
        }
        x = static_cast<int>(std::floor(p.x)) - (face == 5);
        y = static_cast<int>(std::floor(p.y)) - (face == 1);
        z = static_cast<int>(std::floor(p.z)) - (face == 3);
        if (visit(x, y, z)) return;
    }
}
}
