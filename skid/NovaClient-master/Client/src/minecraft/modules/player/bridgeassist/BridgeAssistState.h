#pragma once

#include <algorithm>
#include <cmath>

namespace nova::module::bridgeassist {

// Raven BridgeAssist's tick timers and key override state. Random is [0,1).
class SneakState {
public:
    bool sneaking = false, placed = false, forceRelease = false;
    int jumpTicks = -1, jumpStart = -1, releaseTicks = -1, releaseStart = -1;
    void ResetTimers() { jumpTicks = jumpStart = releaseTicks = releaseStart = -1; }
    void Reset() { *this = {}; }
    static int DelayTicks(double milliseconds, double random) {
        const double raw = milliseconds / 50.0;
        const int base = static_cast<int>(raw);
        return base + (random < raw - base ? 1 : 0);
    }
    template<class Input, class SetKey>
    void Repress(Input& e, bool manual, SetKey setKey) {
        if (forceRelease && manual) { setKey(true); e.sneak = true; }
        forceRelease = false;
    }
    template<class Input, class SetKey>
    void Press(Input& e, bool manual, SetKey setKey, bool resetDelay = true) {
        e.sneak = true;
        sneaking = true;
        if (resetDelay) releaseStart = -1;
        Repress(e, manual, setKey);
    }
    template<class Input, class SetKey>
    void Clear(Input& e, bool require, bool manual, SetKey setKey) {
        sneaking = false;
        ResetTimers();
        if (require) Repress(e, manual, setKey);
    }
    template<class Input, class SetKey>
    void TryRelease(Input& e, int tick, double delay, double random,
                    bool require, bool manual, bool onGround, SetKey setKey) {
        if (releaseStart == -1 && jumpStart == -1) {
            releaseStart = tick;
            releaseTicks = DelayTicks(delay - 50.0, random);
        }
        if ((jumpStart != -1 && tick - jumpStart < jumpTicks) ||
            (releaseStart != -1 && tick - releaseStart < releaseTicks)) {
            Press(e, manual, setKey, false);
            return;
        }
        if (!require) e.sneak = false;
        else if (sneaking && manual && (placed || !onGround)) {
            setKey(false); e.sneak = false; forceRelease = true;
        } else if (forceRelease) e.sneak = false;
        sneaking = placed = false;
        ResetTimers();
    }
};

}
