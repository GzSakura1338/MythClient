#pragma once
#include "cs2/sdk/weapons/Weapons.h"
#include <array>

namespace nova::cs2::recoil {
struct Settings {
    unsigned horizontal = 100, vertical = 100, startBullet = 2, smoothMs = 0;
    bool operator==(const Settings&) const = default;
};
Settings Normalize(Settings);
// Pure, allocation-free input controller. Incoming angles contain the previous
// sample's actual correction; output is absolute, not another mouse delta.
class Controller {
public:
    using Sample = sdk::weapons::Sample;
    using Angle = sdk::weapons::Angle;
    void Configure(Settings);
    void Reset();
    Angle Process(const Sample&);
    // Accumulated compensation relative to the configured burst baseline.
    Angle Contribution() const { return state_.applied; }
private:
    struct State {
        Sample previous;
        Angle baseline, filtered, applied;
        bool initialized = false, engaged = false;
    } state_;
    struct Checkpoint {
        Sample sample;
        State before;
        Angle output;
        bool valid = false;
    };
    Angle Step(const Sample&);
    Settings settings_;
    std::array<Checkpoint, 64> history_{};
    unsigned next_ = 0;
};
}
