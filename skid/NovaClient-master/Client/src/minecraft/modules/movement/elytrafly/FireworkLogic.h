#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace nova::module::elytrafly {
struct FireworkPolicy {
    // Keep prompt takeoff. Once a booster is confirmed, use its remaining
    // momentum instead of renewing immediately below 95% of ideal boost speed.
    static constexpr double takeoff = 1.62, cruise = 1.45;
    std::uint64_t nextAttempt = 0, confirmationDeadline = 0;
    std::uint64_t lastBoost = 0;
    bool established = false;
    bool awaitingEntity = false;
    bool valid = false, gliding = false, boosting = false;
    double forward = 0;
    void Observe(std::uint64_t now, bool known, bool flying, bool attached, double projectedSpeed) {
        valid = known && std::isfinite(projectedSpeed);
        gliding = flying; boosting = attached; forward = projectedSpeed;
        if (valid && !gliding) established = false;
        if (valid && gliding && boosting) {
            awaitingEntity = false; established = true; lastBoost = now;
        }
        if (valid && awaitingEntity && now >= confirmationDeadline) awaitingEntity = false;
    }
    bool Ready(std::uint64_t now) const {
        return valid && gliding && !boosting && !awaitingEntity && now >= nextAttempt &&
            (!established || now > lastBoost + 2) && forward < (established ? cruise : takeoff);
    }
    void Submitted(std::uint64_t now) {
        awaitingEntity = true;
        nextAttempt = now + 10;
        // Mio's Fireworks uses a 500 ms guard followed by an attached-entity
        // check. Rocket lifespan is not evidence that the server accepted use.
        // A fresh known observation is required before retrying after the guard.
        confirmationDeadline = nextAttempt;
    }
};
}
