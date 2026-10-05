#include "cs2/modules/combat/aimbot/AimController.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace nova::cs2::aim {
namespace {
constexpr double radians = std::numbers::pi / 180.;
bool Finite(Angle a) { return std::isfinite(a.pitch) && std::isfinite(a.yaw) && std::isfinite(a.roll); }
float Turn(float error, unsigned speed, double dt, unsigned smoothMs) {
    const double distance = std::abs(static_cast<double>(error));
    if (!speed || distance == 0) return 0;
    if (!smoothMs) return static_cast<float>(std::copysign(std::min(distance, speed * dt), error));
    // Integrate a speed-limited exponential approach exactly: constant speed
    // far away, ease-out near the point. Independent of input sample frequency.
    const double tau = smoothMs / 1000., threshold = speed * tau;
    const double linearTime = std::max(0., (distance - threshold) / speed);
    const double remaining = dt <= linearTime ? distance - speed * dt :
        std::min(distance, threshold) * std::exp(-(dt - linearTime) / tau);
    return static_cast<float>(std::copysign(std::clamp(distance - remaining, 0., distance), error));
}
}
Settings Normalize(Settings s) {
    s.fov = std::isfinite(s.fov) ? std::clamp(s.fov, .1f, 180.f) : 5.f;
    s.horizontal = std::min(s.horizontal, 720u); s.vertical = std::min(s.vertical, 720u);
    s.smoothMs = std::min(s.smoothMs, 500u);
    if (s.point > sdk::aim::Point::Pelvis) s.point = sdk::aim::Point::Chest;
    return s;
}
bool Direction(sdk::players::Vec3 eye, sdk::players::Vec3 point, Angle& out) {
    const double x = static_cast<double>(point.x) - eye.x, y = static_cast<double>(point.y) - eye.y, z = static_cast<double>(point.z) - eye.z;
    const double horizontal = std::hypot(x, y);
    if (!std::isfinite(horizontal) || !std::isfinite(z) || std::hypot(horizontal, z) < .001) return false;
    out = {static_cast<float>(-std::atan2(z, horizontal) / radians), static_cast<float>(std::atan2(y, x) / radians), 0};
    return true;
}
float Distance(Angle a, Angle b) {
    if (!Finite(a) || !Finite(b)) return 180;
    const double dot = std::sin(a.pitch * radians) * std::sin(b.pitch * radians) +
        std::cos(a.pitch * radians) * std::cos(b.pitch * radians) * std::cos((a.yaw - b.yaw) * radians);
    return static_cast<float>(std::acos(std::clamp(dot, -1., 1.)) / radians);
}
void Controller::Reset() { state_ = {}; history_ = {}; next_ = 0; acting_ = false; }
Angle Controller::Reference(const Sample& s, Settings settings, Angle current, Angle compensation) const {
    auto before = state_;
    for (const auto& saved : history_) {
        if (saved.valid && saved.identity == s.weapon.identity && saved.frame == s.frame &&
            saved.ordinal == s.ordinal && saved.time == s.time) { before = saved.before; break; }
    }
    const double dt = s.time - before.time;
    if (!before.ready || before.identity != s.weapon.identity || dt < 0 || dt > .25) before = {};
    const auto prior = settings.compensateRecoil && before.target && before.compensated ? before.compensation : compensation;
    return {current.pitch - prior.pitch, current.yaw - prior.yaw, current.roll};
}
Angle Controller::Process(const Sample& s, const sdk::aim::Snapshot& snapshot, Settings settings, Angle current, Angle compensation) {
    settings = Normalize(settings);
    acting_ = false;
    if (!Finite(current) || !Finite(compensation) || !std::isfinite(s.time) || s.time < 0 || snapshot.count > snapshot.targets.size()) {
        Reset(); return current;
    }
    const auto reference = Reference(s, settings, current, compensation);
    for (const auto& saved : history_) {
        if (!saved.valid || saved.identity != s.weapon.identity || saved.frame != s.frame || saved.ordinal != s.ordinal || saved.time != s.time) continue;
        state_ = saved.before;
        for (auto& future : history_)
            if (future.valid && (future.frame > s.frame || (future.frame == s.frame && future.ordinal >= s.ordinal))) future.valid = false;
        break;
    }
    history_[next_++ % history_.size()] = {s.frame, s.ordinal, s.time, s.weapon.identity, state_, true};
    const double dt = s.time - state_.time;
    const bool boundary = !state_.ready || state_.identity != s.weapon.identity || dt < 0 || dt > .25;
    if (boundary) state_ = {};
    state_.identity = s.weapon.identity; state_.time = s.time; state_.ready = true;
    // Compare in the uncompensated space. The RCS offset is applied exactly once
    // to the desired input angle below, and never accumulated as aim motion.
    const bool continuing = settings.compensateRecoil && state_.target && state_.compensated;
    const auto priorCompensation = continuing ? state_.compensation : compensation;
    int best = -1, locked = -1; float bestDistance = 181, lockedDistance = 181;
    std::array<Angle, sdk::aim::maxTargets> directions{};
    for (unsigned i = 0; i < snapshot.count; ++i) {
        if (!snapshot.targets[i].handle || !Direction(snapshot.eye, snapshot.targets[i].point, directions[i])) continue;
        Angle acquisition = directions[i];
        if (snapshot.targets[i].fovPoint && !Direction(snapshot.eye, *snapshot.targets[i].fovPoint, acquisition)) continue;
        const float distance = Distance(reference, acquisition);
        if (distance > settings.fov) continue;
        if (snapshot.targets[i].handle == state_.target) { locked = static_cast<int>(i); lockedDistance = distance; }
        if (distance < bestDistance || (distance == bestDistance && best >= 0 && snapshot.targets[i].handle < snapshot.targets[best].handle)) {
            best = static_cast<int>(i); bestDistance = distance;
        }
    }
    if (locked >= 0 && (!settings.switching || bestDistance + .35f >= lockedDistance)) best = locked;
    if (best < 0) { state_.target = 0; state_.compensated = false; return current; }
    state_.target = snapshot.targets[best].handle;
    const auto desired = directions[best];
    if (continuing) {
        // A new recoil impulse must neither consume the turn-speed budget nor
        // kick the locked target out of a small sniper FOV.
        if (settings.horizontal) current.yaw = std::remainder(current.yaw + compensation.yaw - priorCompensation.yaw, 360.f);
        if (settings.vertical) current.pitch = std::clamp(current.pitch + compensation.pitch - priorCompensation.pitch, -89.f, 89.f);
    }
    state_.compensation = compensation; state_.compensated = true;
    if (boundary || dt == 0) return current;
    acting_ = true;
    if (settings.horizontal) {
        const float error = std::remainder(desired.yaw + compensation.yaw - current.yaw, 360.f);
        current.yaw = std::remainder(current.yaw + Turn(error, settings.horizontal, dt, settings.smoothMs), 360.f);
    }
    if (settings.vertical) {
        const float error = desired.pitch + compensation.pitch - current.pitch;
        current.pitch = std::clamp(current.pitch + Turn(error, settings.vertical, dt, settings.smoothMs), -89.f, 89.f);
    }
    return current;
}
}
