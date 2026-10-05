#pragma once
#include "cs2/modules/Module.h"

namespace nova::cs2::modules {
class AutoFire final : public Module {
public:
    AutoFire();
    bool persistEnabled() const override { return !keybind().hold(); }
private:
    void onEnable() override;
    void onDisable() override;
    void onUpdate() override;
    void reportUnavailable();
    Properties::NumberProp& delay_;
    Properties::BoolProperty& scoped_;
    std::uint64_t nextLog_ = 0;
    bool unavailableReported_ = false;
};
}
