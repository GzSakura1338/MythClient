#include "cs2/modules/combat/recoilcontrol/RecoilController.h"
#include <algorithm>
#include <cmath>

namespace nova::cs2::recoil {
namespace {
using Angle = sdk::weapons::Angle;
bool Finite(Angle a) {
    return std::isfinite(a.pitch) && std::isfinite(a.yaw) && std::isfinite(a.roll) &&
        std::abs(a.pitch) <= 180 && std::abs(a.yaw) <= 360 && std::abs(a.roll) <= 180;
}
float Wrap(float value) { return std::remainder(value, 360.f); }
bool Same(Angle a, Angle b) { return a.pitch == b.pitch && a.yaw == b.yaw && a.roll == b.roll; }
}
Settings Normalize(Settings s) {
    s.horizontal = std::min(s.horizontal, 100u); s.vertical = std::min(s.vertical, 100u);
    s.startBullet = std::clamp(s.startBullet, 1u, 10u); s.smoothMs = std::min(s.smoothMs, 100u);
    return s;
}
void Controller::Configure(Settings s) {
    s = Normalize(s);
    if (settings_ != s) { settings_ = s; Reset(); }
}
void Controller::Reset() { state_ = {}; history_ = {}; next_ = 0; }
Controller::Angle Controller::Process(const Sample& source) {
    if (!Finite(source.view) || !Finite(source.punch) || !std::isfinite(source.time) || source.time < 0 ||
        !std::isfinite(source.weapon.lastShot) || !std::isfinite(source.weapon.recoilIndex) ||
        !sdk::weapons::IsFirearm(source.weapon.identity.item)) { Reset(); return source.view; }
    Sample sample = source;
    // Restore the state immediately before this sample if an input producer
    // replays it. Command re-prediction normally reuses the game's saved samples
    // without entering this hook at all. Never advance the filter twice.
    for (auto& checkpoint : history_) {
        if (!checkpoint.valid || checkpoint.sample.frame != sample.frame || checkpoint.sample.ordinal != sample.ordinal ||
            checkpoint.sample.weapon.identity != sample.weapon.identity || checkpoint.sample.time != sample.time) continue;
        if (Same(sample.view, checkpoint.output)) sample.view = checkpoint.sample.view;
        state_ = checkpoint.before;
        const auto frame = sample.frame, ordinal = sample.ordinal;
        for (auto& future : history_)
            if (future.valid && (future.sample.frame > frame ||
                (future.sample.frame == frame && future.sample.ordinal >= ordinal))) future.valid = false;
        break;
    }
    auto& checkpoint = history_[next_++ % history_.size()];
    checkpoint = {sample, state_, {}, true};
    checkpoint.output = Step(sample);
    return checkpoint.output;
}
Controller::Angle Controller::Step(const Sample& sample) {
    auto& s = state_;
    const auto& w = sample.weapon;
    const double dt = sample.time - s.previous.time;
    const bool boundary = !s.initialized || w.identity != s.previous.weapon.identity || dt < 0 || dt > .25 ||
        w.shots < s.previous.weapon.shots || w.lastShot < s.previous.weapon.lastShot;
    if (boundary) {
        s = {}; s.initialized = true; s.baseline = sample.punch; s.previous = sample;
        return sample.view;
    }
    const bool shot = w.lastShot > s.previous.weapon.lastShot &&
        (w.shots > s.previous.weapon.shots || w.clip < s.previous.weapon.clip || w.recoilIndex > s.previous.weapon.recoilIndex);
    s.engaged |= shot;
    // Actual new shots win over empty/reload state so the last round isn't
    // skipped. Burst leftovers continue after trigger release. Cocking the R8
    // alone never starts compensation because no actual shot was observed.
    const bool active = shot || (s.engaged && w.shots > 0 && !w.reload && !w.switchingSilencer &&
        (w.burst > 0 || (sample.attack && w.clip > 0)));
    if (!active || w.shots < static_cast<int>(settings_.startBullet)) {
        s.baseline = sample.punch; s.filtered = {}; s.applied = {};
        if (!active) s.engaged = false;
        s.previous = sample;
        return sample.view;
    }
    Angle target{-(sample.punch.pitch - s.baseline.pitch) * settings_.vertical / 100.f,
        -Wrap(sample.punch.yaw - s.baseline.yaw) * settings_.horizontal / 100.f, 0};
    const float alpha = settings_.smoothMs == 0 ? 1.f : static_cast<float>(-std::expm1(-dt * 1000 / settings_.smoothMs));
    s.filtered.pitch += alpha * (target.pitch - s.filtered.pitch);
    s.filtered.yaw += alpha * (target.yaw - s.filtered.yaw);
    Angle out = sample.view;
    if (settings_.vertical) {
        const float requested = out.pitch + s.filtered.pitch - s.applied.pitch;
        out.pitch = std::clamp(requested, -89.f, 89.f);
        s.baseline.pitch += (out.pitch - requested) * 100.f / settings_.vertical;
    }
    if (settings_.horizontal) out.yaw = Wrap(out.yaw + s.filtered.yaw - s.applied.yaw);
    // Feed back the amount actually applied, including pitch clamping. Discard
    // clipped filter debt so releasing a limit cannot produce a delayed snap.
    s.applied.pitch += out.pitch - sample.view.pitch;
    s.applied.yaw += Wrap(out.yaw - sample.view.yaw);
    s.filtered = s.applied;
    s.previous = sample;
    return out;
}
}
