#pragma once

namespace nova::module::timer {
// Rise counts eligible C03 packets, not elapsed time or saved milliseconds.
struct Balance {
    double value = 0;
    bool compensating = false;
    void Reset() { value = 0; compensating = false; }
    bool Press() {
        if (compensating) return true;
        if (value <= 0) return false;
        compensating = true;
        return true;
    }
    // True means compensation finished and the module must disable.
    bool Movement(bool lowTimer, double multiplier) {
        if (compensating) {
            if (value > 0 && --value <= 0) {
                compensating = false;
                return true;
            }
        } else if (lowTimer) value += multiplier;
        return false;
    }
};
}
