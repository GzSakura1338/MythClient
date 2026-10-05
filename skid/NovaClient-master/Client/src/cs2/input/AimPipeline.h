#pragma once
#include "cs2/modules/combat/aimbot/AimController.h"
#include "cs2/modules/combat/recoilcontrol/RecoilController.h"

namespace nova::cs2::input {
class AimPipeline {
public:
    void Configure(recoil::Settings, bool recoilEnabled, aim::Settings, bool aimEnabled);
    void Reset();
    void ResetAim();
    std::uint32_t Target() const { return aim_.Target(); }
    sdk::weapons::Angle VisualOffset() const { return visualOffset_; }
    sdk::weapons::Angle Process(sdk::weapons::Sample, const sdk::aim::Snapshot&);
    using CaptureTargets = bool (*)(void*, const sdk::weapons::Sample&, sdk::weapons::Angle reference, sdk::aim::Snapshot&);
    sdk::weapons::Angle Process(sdk::weapons::Sample, void*, CaptureTargets);
private:
    recoil::Controller recoil_;
    aim::Controller aim_;
    recoil::Settings recoilSettings_;
    aim::Settings aimSettings_;
    bool recoilEnabled_ = false, aimEnabled_ = false;
    sdk::weapons::Angle visualOffset_;
    struct Result { sdk::weapons::Sample source; sdk::weapons::Angle output; bool valid = false; };
    std::array<Result, 64> results_{};
    unsigned next_ = 0;
};
}
