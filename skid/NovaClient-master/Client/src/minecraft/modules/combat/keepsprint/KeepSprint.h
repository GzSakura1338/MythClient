#pragma once
#include <algorithm>

namespace nova::module::keepsprint {
struct Settings {
    double velocity = 0.6, normal = 0.6;
    double decrease = 1;
    int maximum = 5;
    bool velocitySprint = false, normalSprint = false;
    bool buffer = false, onlyAir = false;
};
struct Result { double slowdown = 0.6; bool sprint = false; };

// Rise KeepSprint.onHitSlowDown, including the zero-buffer transition and
// persistence across toggles. Called once at the actual hit-slowdown site.
struct State {
    bool resetting = false;
    double combo = 0;
    Result Hit(const Settings& s, bool ground, int hurt, Result original = {}) {
        if (ground && s.onlyAir) return original;
        if (s.buffer) {
            if (combo < s.maximum && !resetting) ++combo;
            else {
                if (combo > 0) {
                    combo = std::max(0.0, combo - s.decrease);
                    resetting = true;
                    return original;
                }
                resetting = false;
            }
        } else combo = 0;
        return hurt > 0 ? Result{s.velocity, s.velocitySprint} : Result{s.normal, s.normalSprint};
    }
};
}
