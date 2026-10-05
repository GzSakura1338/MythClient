#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include "minecraft/utils/rotation/Vec3.h"
#include "minecraft/utils/rotation/MathConst.h"

namespace nova::module::throwableaura {
using rotation::Vec3;
struct Bounds { Vec3 min, max; };
struct Path { std::vector<Vec3> points; bool impact = false; };
inline bool Finite(Vec3 p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }

// Segment parameter of the first intersection, including a start inside the box.
inline bool Intersect(const Bounds& box, Vec3 from, Vec3 to, double& fraction) {
    if (!Finite(from) || !Finite(to) || !Finite(box.min) || !Finite(box.max)) return false;
    double entry = 0, exit = 1;
    const double start[]{from.x,from.y,from.z}, end[]{to.x,to.y,to.z};
    const double lo[]{box.min.x,box.min.y,box.min.z}, hi[]{box.max.x,box.max.y,box.max.z};
    for (int axis = 0; axis < 3; ++axis) {
        if (lo[axis] > hi[axis]) return false;
        const double delta = end[axis] - start[axis];
        if (std::abs(delta) < 1e-12) { if (start[axis] < lo[axis] || start[axis] > hi[axis]) return false; continue; }
        double a = (lo[axis] - start[axis]) / delta, b = (hi[axis] - start[axis]) / delta;
        if (a > b) std::swap(a,b);
        entry = std::max(entry,a); exit = std::min(exit,b);
        if (entry > exit) return false;
    }
    fraction = entry; return true;
}

// Snowballs/eggs share 1.8 EntityThrowable launch physics; random spread is unknown.
template<class Trace, class Water>
Path Predict(Vec3 eye, float yaw, float pitch, const std::vector<Bounds>& entities, Trace&& trace, Water&& water) {
    Path result;
    if (!Finite(eye) || !std::isfinite(yaw) || !std::isfinite(pitch)) return result;
    const double y = yaw * rotation::kToRadians, p = pitch * rotation::kToRadians;
    Vec3 position = eye.Add(-std::cos(y)*.16,-.1,-std::sin(y)*.16);
    Vec3 motion{-std::sin(y)*std::cos(p)*1.5,-std::sin(p)*1.5,std::cos(y)*std::cos(p)*1.5};
    result.points.reserve(81); result.points.push_back(position);
    double traveled = 0;
    for (int tick = 0; tick < 80 && traveled < 64; ++tick) {
        const auto end = position.Add(motion.x,motion.y,motion.z);
        Vec3 blockHit;
        bool block = false;
        if (!trace(position,end,block,blockHit)) break;
        double nearest = 1;
        bool hit = block;
        const double length = position.DistanceTo(end);
        if (block) {
            if (!Finite(blockHit)) break;
            nearest = length > 1e-12 ? std::clamp(position.DistanceTo(blockHit)/length,0.,1.) : 0;
        }
        for (const auto& box : entities) {
            double t = 0;
            if (Intersect(box,position,end,t) && (!hit || t < nearest)) { nearest = t; hit = true; }
        }
        position = position.Add(motion.x*nearest,motion.y*nearest,motion.z*nearest);
        result.points.push_back(position);
        if (hit) { result.impact = true; break; }
        traveled += length;
        motion = motion.Scale(water(position) ? .8 : .99);
        motion.y -= .03;
    }
    return result;
}
}
