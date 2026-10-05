#include "cs2/modules/combat/autofire/AutoFireController.h"
#include "cs2/sdk/weapons/Offsets.h"
#include <algorithm>
#include <cmath>

namespace nova::cs2::autofire {
Settings Normalize(Settings s) { s.delayMs = std::min(s.delayMs, 500u); return s; }
void Controller::Configure(Settings s) { s = Normalize(s); if (s != settings_) { Reset(); settings_ = s; } }
void Controller::Reset() { state_ = {}; history_ = {}; next_ = 0; }
bool Controller::Process(const Sample& s, const sdk::weapons::FireState& fire, std::uint32_t target) {
    for (const auto& h : history_) {
        if (!h.valid || h.sample.weapon.identity != s.weapon.identity || h.sample.frame != s.frame ||
            h.sample.ordinal != s.ordinal || h.sample.time != s.time) continue;
        state_ = h.before;
        for (auto& future : history_) if (future.valid && future.sample.time >= s.time) future.valid = false;
        break;
    }
    const auto before = state_;
    const bool down = Step(s, fire, target);
    history_[next_++ % history_.size()] = {s, before, true};
    return down;
}
bool Controller::Step(const Sample& s, const sdk::weapons::FireState& fire, std::uint32_t target) {
    if (!std::isfinite(s.time) || s.time < 0 || !std::isfinite(s.weapon.nextAttack) ||
        !std::isfinite(s.weapon.lastShot)) { state_ = {}; return false; }
    if (!state_.initialized || state_.previous.weapon.identity != s.weapon.identity ||
        s.time < state_.previous.time || s.time - state_.previous.time > .25) state_ = {};
    const auto previous = state_.previous;
    const bool shot = state_.initialized && (s.weapon.lastShot > previous.weapon.lastShot ||
        s.weapon.shots > previous.weapon.shots || s.weapon.clip < previous.weapon.clip);
    state_.previous = s; state_.initialized = true;
    if (!target || !fire.valid || !fire.allowed || !sdk::weapons::IsFirearm(s.weapon.identity.item) ||
        s.weapon.clip <= 0 || s.weapon.reload || s.weapon.switchingSilencer ||
        (settings_.scopedOnly && sdk::weapons::IsSniper(s.weapon.identity.item) && !s.weapon.scoped)) {
        state_.target = 0;
        if (state_.held) state_.released = s.time;
        state_.held = false; return false;
    }
    if (state_.target != target) {
        state_.target = target; state_.acquired = s.time;
        if (state_.held) state_.released = s.time;
        state_.held = false;
    }
    // R8 stays pressed throughout native cocking; its actual shot feedback,
    // not its postpone-fire timer, ends the press. Burst follow-ups stay native.
    const bool burstMode = (s.weapon.identity.item == 4 || s.weapon.identity.item == 10) && s.weapon.identity.mode == 1;
    if ((!fire.fullAuto || fire.revolver || burstMode) && state_.held && shot) {
        state_.held = false; state_.released = s.time; return false;
    }
    if (s.time - state_.acquired + 1e-8 < settings_.delayMs / 1000.) return false;
    if (state_.held) return true; // Don't abort native cocking during its cooldown.
    if (s.weapon.burst || s.time + 1e-8 < s.weapon.nextAttack ||
        (state_.released >= 0 && s.time - state_.released + 1e-8 < sdk::weapons::offsets::interval)) return false;
    state_.held = true;
    return true;
}
}
