#pragma once

namespace nova::module::killaura {
// Starain 1.20.1 Criticals: arm on PRE tick in the first block of falling,
// then consume once by clearing forward input. Vanilla owns sprint packets.
struct CriticalSprint {
    bool pending = false;
    void Reset() { pending = false; }
    void Tick(double fallDistance, bool hasTargets, int attackQueue) {
        pending = fallDistance > 0 && fallDistance < 1 && hasTargets && attackQueue == 0;
    }
    void Input(float& forward) {
        if (!pending) return;
        pending = false;
        forward = 0;
    }
};
}
