#pragma once
#include "cs2/modules/Module.h"
namespace nova::cs2::modules {
class Chams final : public Module {
public:
    Chams();
    std::string suffix() const override;
private:
    void onEnable() override;
    void onDisable() override;
    void onUpdate() override;
    Properties::ModeProperty& material_;
    Properties::BoolProperty& throughWalls_;
    Properties::ColorProperty visible_{0xFF00FF00u, true}, hidden_{0xFF0000FFu, true};
};
}
