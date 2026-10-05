#pragma once
#include "cs2/sdk/aim/AimSnapshot.h"

namespace nova::cs2::aim {
using Angle = sdk::weapons::Angle;
using Sample = sdk::weapons::Sample;
struct Settings {
    float fov = 5;
    unsigned horizontal = 120, vertical = 90;
    bool switching = false;
    sdk::aim::Point point = sdk::aim::Point::Chest;
    bool compensateRecoil = true;
    unsigned smoothMs = 0;
    bool operator==(const Settings&) const = default;
};
Settings Normalize(Settings);
bool Direction(sdk::players::Vec3 eye, sdk::players::Vec3 point, Angle&);
float Distance(Angle, Angle);
class Controller {
public:
    void Reset();
    std::uint32_t Target() const { return state_.target; }
    bool Acting() const { return acting_; }
    // Read the same pre-sample/replay state used by Process, so hitbox FOV
    // filtering and final target selection use identical compensated angles.
    Angle Reference(const Sample&, Settings, Angle current, Angle compensation) const;
    Angle Process(const Sample&, const sdk::aim::Snapshot&, Settings, Angle current, Angle compensation);
private:
    struct State {
        sdk::weapons::Identity identity;
        double time = 0;
        std::uint32_t target = 0;
        bool ready = false;
        Angle compensation;
        bool compensated = false;
    } state_;
    struct Checkpoint { std::uint32_t frame = 0, ordinal = 0; double time = 0; sdk::weapons::Identity identity; State before; bool valid = false; };
    std::array<Checkpoint, 64> history_{};
    unsigned next_ = 0;
    bool acting_ = false;
};
}
