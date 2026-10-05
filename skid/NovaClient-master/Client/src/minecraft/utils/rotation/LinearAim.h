#pragma once

#include <algorithm>
#include <cmath>
#include "minecraft/utils/rotation/Rotation.h"

namespace nova::rotation {

// A constant-length step in yaw/pitch space, using the shortest yaw arc.
// Used for silent return ticks with a combined yaw/pitch speed budget.
inline Rotation LinearStep(Rotation from, Rotation target, double maxDegrees) {
    if (!std::isfinite(from.yaw) || !std::isfinite(from.pitch) ||
        !std::isfinite(target.yaw) || !std::isfinite(target.pitch) ||
        !std::isfinite(maxDegrees) || maxDegrees <= 0.0) return from;
    from.pitch = std::clamp(from.pitch, -90.0f, 90.0f);
    target.pitch = std::clamp(target.pitch, -90.0f, 90.0f);
    double yaw = std::remainder(static_cast<double>(target.yaw) - from.yaw, 360.0);
    if (yaw == 180.0) yaw = -180.0;
    const double pitch = static_cast<double>(target.pitch) - from.pitch;
    const double distance = std::hypot(yaw, pitch);
    if (distance < 1e-8) return from;
    const double scale = (std::min)(1.0, maxDegrees / distance);
    return {static_cast<float>(from.yaw + yaw * scale),
            static_cast<float>(from.pitch + pitch * scale)};
}

// Camera and silent tracking: each axis has its own angular budget.
inline Rotation AxisLinearStep(Rotation from, Rotation target, double maxYaw, double maxPitch) {
    if (!std::isfinite(from.yaw) || !std::isfinite(from.pitch) ||
        !std::isfinite(target.yaw) || !std::isfinite(target.pitch) ||
        !std::isfinite(maxYaw) || !std::isfinite(maxPitch)) return from;
    double yaw = std::remainder(static_cast<double>(target.yaw) - from.yaw, 360.0);
    if (yaw == 180.0) yaw = -180.0;
    const double pitch = std::clamp(target.pitch, -90.0f, 90.0f) - from.pitch;
    maxYaw = (std::max)(0.0, maxYaw);
    maxPitch = (std::max)(0.0, maxPitch);
    return {static_cast<float>(from.yaw + std::clamp(yaw, -maxYaw, maxYaw)),
            static_cast<float>(from.pitch + std::clamp(pitch, -maxPitch, maxPitch))};
}

// Match vanilla mouse turning: shift both interpolation endpoints by the
// camera delta, so the renderer does not attenuate each frame's movement.
inline Rotation CameraPreviousAfterTurn(Rotation previous, Rotation current, Rotation next, double amount) {
    const double weight = std::isfinite(amount) ? std::clamp(amount / 100.0, 0.0, 1.0) : 0.0;
    return {static_cast<float>(previous.yaw + (next.yaw - current.yaw) * weight),
            static_cast<float>(previous.pitch + (next.pitch - current.pitch) * weight)};
}

// Only representation changes should move the hand's smoothing endpoints.
// Real mouse/aim turns must retain vanilla hand lag.
inline float CameraYawRebase(Rotation current, Rotation next) {
    const float delta = next.yaw - current.yaw;
    if (!std::isfinite(delta) || !std::isfinite(current.pitch) || !std::isfinite(next.pitch) ||
        std::fabs(next.pitch - current.pitch) > .0001f || std::fabs(delta) < 359.99f ||
        std::fabs(std::remainder(delta, 360.f)) > .0001f) return 0;
    return delta;
}

class AimFrameClock {
public:
    void Reset() { last_ = -1.0; }
    double Advance(double now) {
        const double elapsed = now - last_;
        const bool ready = last_ >= 0.0;
        last_ = std::isfinite(now) ? now : -1.0;
        // Do not bank time while inactive or jump after a pause/stall.
        return ready && std::isfinite(elapsed) && elapsed > 0.0 && elapsed <= 0.25
            ? elapsed : 0.0;
    }
private:
    double last_ = -1.0;
};

}
