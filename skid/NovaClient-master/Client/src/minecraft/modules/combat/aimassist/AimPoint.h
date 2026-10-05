#pragma once

#include <algorithm>
#include <cmath>
#include "minecraft/java/api/game_entity.h"
#include "minecraft/utils/rotation/Rotation.h"

namespace nova::module::aimassist {

// Align tick positions with the rendered entity without filtering angular
// velocity. Skip teleport-sized discontinuities rather than trailing them.
inline rotation::Vec3 SmoothRenderOffset(const rotation::Vec3& now, const rotation::Vec3& before,
                                         double partial, double amount) {
    const auto delta = before.Subtract(now);
    const double distance = delta.DistanceToSqr({});
    if (!std::isfinite(distance) || distance > 16 || !std::isfinite(partial) ||
        !std::isfinite(amount)) return {};
    return delta.Scale((1.0 - std::clamp(partial, 0.0, 1.0)) * std::clamp(amount / 100.0, 0.0, 1.0));
}

// Full cone angle around the real camera. Use the actual selected body point,
// not entity feet or silent/server yaw, for both acquisition and tracking.
inline bool PointInFov(rotation::Rotation view, const rotation::Vec3& eye,
                       const rotation::Vec3& point, double fov) {
    if (!std::isfinite(view.yaw) || !std::isfinite(view.pitch) ||
        !std::isfinite(fov) || fov <= 0 ||
        !std::isfinite(eye.x) || !std::isfinite(eye.y) || !std::isfinite(eye.z) ||
        !std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z)) return false;
    const double x = point.x - eye.x, y = point.y - eye.y, z = point.z - eye.z;
    const double length = std::hypot(x, y, z);
    if (!std::isfinite(length) || length < 1e-8) return false;
    if (fov >= 360) return true;
    constexpr double radians = 0.017453292519943295;
    const double yaw = std::remainder(static_cast<double>(view.yaw), 360.0) * radians;
    const double pitch = std::clamp(view.pitch, -90.0f, 90.0f) * radians;
    const double dot = (-std::sin(yaw) * std::cos(pitch) * x - std::sin(pitch) * y +
                        std::cos(yaw) * std::cos(pitch) * z) / length;
    return dot >= std::cos(fov * .5 * radians) - 1e-12;
}

// Relative to the current bounding box so crouching/swimming poses scale with
// the entity. Multipoint stays inside the selected height band.
inline bool PreferredPoint(const java::Box& box, int part, const rotation::Vec3& eye,
                           double horizontal, double vertical, rotation::Vec3& out) {
    if (!std::isfinite(box.minX) || !std::isfinite(box.maxX) ||
        !std::isfinite(box.minY) || !std::isfinite(box.maxY) ||
        !std::isfinite(box.minZ) || !std::isfinite(box.maxZ) ||
        !std::isfinite(eye.x) || !std::isfinite(eye.y) || !std::isfinite(eye.z) ||
        !std::isfinite(horizontal) || !std::isfinite(vertical) ||
        box.maxX <= box.minX || box.maxY <= box.minY || box.maxZ <= box.minZ) return false;
    struct Band { double low, center, high; };
    constexpr Band bands[] = {{0.80, 0.90, 0.98}, {0.45, 0.65, 0.78}, {0.12, 0.25, 0.42}};
    const auto band = bands[std::clamp(part, 0, 2)];
    const double height = box.maxY - box.minY;
    const double x = (box.minX + box.maxX) * 0.5;
    const double y = box.minY + height * band.center;
    const double z = (box.minZ + box.maxZ) * 0.5;
    const double th = std::clamp(horizontal / 100.0, 0.0, 1.0);
    const double tv = std::clamp(vertical / 100.0, 0.0, 1.0);
    const double insetX = (box.maxX - box.minX) * 0.05;
    const double insetZ = (box.maxZ - box.minZ) * 0.05;
    out = {x + (std::clamp(eye.x, box.minX + insetX, box.maxX - insetX) - x) * th,
           y + (std::clamp(eye.y, box.minY + height * band.low, box.minY + height * band.high) - y) * tv,
           z + (std::clamp(eye.z, box.minZ + insetZ, box.maxZ - insetZ) - z) * th};
    return true;
}

}
