#pragma once
#include "cs2/sdk/weapons/Weapons.h"
#include <array>

namespace nova::cs2::autofire {
struct Settings {
    unsigned delayMs = 0;
    bool scopedOnly = false;
    bool operator==(const Settings&) const = default;
};
Settings Normalize(Settings);
class Controller {
public:
    void Configure(Settings);
    void Reset();
    bool Process(const sdk::weapons::Sample&, const sdk::weapons::FireState&, std::uint32_t target);
private:
    using Sample = sdk::weapons::Sample;
    struct State {
        Sample previous;
        std::uint32_t target = 0;
        double acquired = 0, released = -1;
        bool initialized = false, held = false;
    } state_;
    struct Checkpoint { Sample sample; State before; bool valid = false; };
    bool Step(const Sample&, const sdk::weapons::FireState&, std::uint32_t);
    Settings settings_;
    std::array<Checkpoint, 64> history_{};
    unsigned next_ = 0;
};
}
