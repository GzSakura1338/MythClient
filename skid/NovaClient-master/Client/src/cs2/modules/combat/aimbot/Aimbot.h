#pragma once
#include "cs2/modules/Module.h"

namespace nova::cs2::modules {
class Aimbot final : public Module {
public:
    Aimbot();
    bool persistEnabled() const override { return !keybind().hold(); }
private:
    void onEnable() override;
    void onDisable() override;
    void onUpdate() override;
    void onRender() override;
    Properties::NumberProp &fov_, &horizontal_, &vertical_, &smooth_;
    Properties::ModeProperty &target_, &point_;
    Properties::BoolProperty &showFov_, &onlyClick_, &compensate_;
};
}
