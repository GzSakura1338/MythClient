#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>
#include "minecraft/java/api/game_entity.h"
#include "minecraft/utils/rotation/Mth.h"

namespace nova::module::bridgeassist {
using java::Box;
using rotation::Vec3;

inline Box Offset(Box b, double x, double y, double z) {
    b.minX += x; b.maxX += x; b.minY += y; b.maxY += y; b.minZ += z; b.maxZ += z;
    return b;
}
inline Box Sweep(Box b, double x, double y, double z) {
    b.minX += std::min(x, 0.0); b.maxX += std::max(x, 0.0);
    b.minY += std::min(y, 0.0); b.maxY += std::max(y, 0.0);
    b.minZ += std::min(z, 0.0); b.maxZ += std::max(z, 0.0);
    return b;
}
inline double Clip(const Box& obstacle, const Box& moving, double delta, int axis) {
    const double lo[]{obstacle.minX, obstacle.minY, obstacle.minZ};
    const double hi[]{obstacle.maxX, obstacle.maxY, obstacle.maxZ};
    const double a[]{moving.minX, moving.minY, moving.minZ};
    const double b[]{moving.maxX, moving.maxY, moving.maxZ};
    for (int i = 0; i < 3; ++i)
        if (i != axis && (b[i] <= lo[i] || a[i] >= hi[i])) return delta;
    if (delta > 0 && b[axis] <= lo[axis]) return std::min(delta, lo[axis] - b[axis]);
    if (delta < 0 && a[axis] >= hi[axis]) return std::max(delta, hi[axis] - a[axis]);
    return delta;
}
inline double Clip(const std::vector<Box>& boxes, const Box& moving, double delta, int axis) {
    for (const auto& box : boxes) delta = Clip(box, moving, delta, axis);
    return delta;
}
inline bool Finite(const Box& b) {
    return std::isfinite(b.minX) && std::isfinite(b.minY) && std::isfinite(b.minZ) &&
        std::isfinite(b.maxX) && std::isfinite(b.maxY) && std::isfinite(b.maxZ) &&
        b.maxX >= b.minX && b.maxY >= b.minY && b.maxZ >= b.minZ;
}
struct Simulation {
    Box box;
    Vec3 motion;
    double x = 0, y = 0, z = 0;
    float yaw = 0, forward = 0, strafe = 0, speed = .1f, airSpeed = .02f;
    float stepHeight = .6f, width = .6f, height = 1.8f, depthStrider = 0;
    double jumpPower = .42f;
    int jumpCooldown = 0, food = 20;
    bool jump = false, ground = false, sprint = false, horizontalCollision = false;
    bool spectator = false, riding = false, blocked = false, usingItem = false;
    bool web = false, water = false, lava = false, ladder = false;
    bool legacy = true;
};

// Vanilla 1.8 uses a 65536-entry sine table, including for the sprint-jump impulse.
inline float Sin(float radians) { return rotation::mth::Sin(radians); }
inline float Cos(float radians) { return rotation::mth::Cos(radians); }

template<class World> bool Move(Simulation& s, World& world) {
    auto [x, y, z] = s.motion;
    if (s.spectator) { s.box = Offset(s.box, x, y, z); return true; }
    if (s.web) { x *= .25; y *= .05000000074505806; z *= .25; }
    const auto start = s.box;
    std::vector<Box> boxes;
    if (!world.Collisions(Sweep(start, x, y, z), boxes)) return false;
    const double dy = Clip(boxes, start, y, 1);
    auto result = Offset(start, 0, dy, 0);
    const double dx = Clip(boxes, result, x, 0);
    result = Offset(result, dx, 0, 0);
    const double dz = Clip(boxes, result, z, 2);
    result = Offset(result, 0, 0, dz);
    if (s.stepHeight > 0 && (s.ground || (y != dy && y < 0)) && (x != dx || z != dz)) {
        if (!world.Collisions(Sweep(start, x, s.stepHeight, z), boxes)) return false;
        auto candidate = [&](bool expanded) {
            double up = Clip(boxes, expanded ? Sweep(start, x, 0, z) : start, s.stepHeight, 1);
            auto box = Offset(start, 0, up, 0);
            double sx = Clip(boxes, box, x, 0);
            box = Offset(box, sx, 0, 0);
            double sz = Clip(boxes, box, z, 2);
            box = Offset(box, 0, 0, sz);
            return std::pair{Offset(box, 0, Clip(boxes, box, -up, 1), 0), sx*sx + sz*sz};
        };
        const auto a = candidate(true), b = candidate(false);
        const auto& best = a.second > b.second ? a : b;
        if (best.second > dx*dx + dz*dz) result = best.first;
    }
    s.box = result;
    return Finite(result);
}

// Only the first tick's final box is needed. Gravity, friction and block callbacks
// after moveEntity affect the *next* tick, so must not be run against the live player.
template<class World> bool Predict(Simulation s, World& world, Box& result) {
    if (!Finite(s.box) || !std::isfinite(s.motion.x) || !std::isfinite(s.motion.y) ||
        !std::isfinite(s.motion.z)) return false;
    if (s.y < -64 || s.riding) { result = s.box; return true; }
    if (!world.Water(s)) return false;
    if (s.spectator) s.ground = false;
    if (s.legacy && !s.spectator) {
        auto headspace = [&](int x, int y, int z) {
            // Deliberately preserve Raven's isOpenBlockSpace/isNormalCube predicate.
            for (int h = 0; h < static_cast<int>(std::ceil(s.height)); ++h)
                if (!world.NormalCube(x, y+h, z)) return false;
            return true;
        };
        for (auto corner : {std::pair{-1,1}, {-1,-1}, {1,-1}, {1,1}}) {
            const double px = s.x + corner.first * s.width * .35;
            const double pz = s.z + corner.second * s.width * .35;
            int x = static_cast<int>(std::floor(px)), y = static_cast<int>(std::floor(s.box.minY+.5));
            int z = static_cast<int>(std::floor(pz));
            if (headspace(x,y,z)) continue;
            double distance = 9999; int side = -1;
            const double fx = px-x, fz = pz-z;
            if (headspace(x-1,y,z) && fx < distance) { distance=fx; side=0; }
            if (headspace(x+1,y,z) && 1-fx < distance) { distance=1-fx; side=1; }
            if (headspace(x,y,z-1) && fz < distance) { distance=fz; side=4; }
            if (headspace(x,y,z+1) && 1-fz < distance) side=5;
            if (side==0) s.motion.x=-.1f;
            if (side==1) s.motion.x=.1f;
            if (side==4) s.motion.z=-.1f;
            if (side==5) s.motion.z=.1f;
        }
    }
    if (s.usingItem) { s.forward *= .2f; s.strafe *= .2f; }
    if (s.forward <= 0 || s.horizontalCollision || s.food <= 6) s.sprint=false;
    if (std::abs(s.motion.x)<.005) s.motion.x=0;
    if (std::abs(s.motion.y)<.005) s.motion.y=0;
    if (std::abs(s.motion.z)<.005) s.motion.z=0;
    if (s.blocked) { s.forward=s.strafe=0; s.jump=false; }
    if (s.jump) {
        if (s.water || s.lava) s.motion.y += .03999999910593033;
        else if (s.ground && s.jumpCooldown <= 1) {
            s.motion.y=s.jumpPower;
            if (s.sprint) {
                float angle=s.yaw*.017453292f;
                s.motion.x -= Sin(angle)*.2f; s.motion.z += Cos(angle)*.2f;
            }
        }
    }
    s.strafe *= .98f; s.forward *= .98f;
    float acceleration = .02f;
    if (s.water) {
        const float depth=std::min(s.depthStrider,3.0f)*(s.ground?1.0f:.5f);
        if (depth>0) acceleration += (s.speed-acceleration)*depth/3.0f;
    } else if (!s.lava) {
        float friction=.91f;
        if (s.ground) friction=world.Slipperiness(static_cast<int>(std::floor(s.x)),
            static_cast<int>(std::floor(s.box.minY))-1,static_cast<int>(std::floor(s.z)))*.91f;
        acceleration=s.ground ? s.speed*(.16277136f/(friction*friction*friction)) : s.airSpeed;
    }
    float length=s.strafe*s.strafe+s.forward*s.forward;
    if (length>=1.0e-4f) {
        float factor=acceleration/std::max(1.0f,std::sqrt(length));
        float strafe=s.strafe*factor, forward=s.forward*factor;
        float angle=s.yaw*3.1415927f/180.0f;
        s.motion.x += strafe*Cos(angle)-forward*Sin(angle);
        s.motion.z += forward*Cos(angle)+strafe*Sin(angle);
    }
    if (!s.water && !s.lava && s.ladder && !s.spectator) {
        s.motion.x=std::clamp(s.motion.x,-double(.15f),double(.15f));
        s.motion.z=std::clamp(s.motion.z,-double(.15f),double(.15f));
        s.motion.y=std::max(s.motion.y,-.15);
    }
    if (!world.Good() || !Move(s,world)) return false;
    // EntityPlayer's world-coordinate clamp also updates its box.
    double x=(s.box.minX+s.box.maxX)*.5, z=(s.box.minZ+s.box.maxZ)*.5;
    s.box=Offset(s.box,std::clamp(x,-29999999.0,29999999.0)-x,0,std::clamp(z,-29999999.0,29999999.0)-z);
    result=s.box;
    return world.Good();
}
template<class World> bool EdgeOffset(const Box& predicted, World& world, double& result) {
    const Box feet{predicted.minX,predicted.minY-.01,predicted.minZ,predicted.maxX,predicted.minY,predicted.maxZ};
    std::vector<Box> ground;
    if (!world.Collisions(feet,ground)) return false;
    result=std::numeric_limits<double>::quiet_NaN();
    const double x=(predicted.minX+predicted.maxX)*.5, z=(predicted.minZ+predicted.maxZ)*.5;
    for (const auto& b : ground) {
        const double d=std::max(std::abs(x-std::clamp(x,b.minX,b.maxX)),std::abs(z-std::clamp(z,b.minZ,b.maxZ)));
        if (std::isnan(result) || d<result) result=d;
    }
    return true;
}
}
