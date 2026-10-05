#pragma once
#include "cs2/modules/Module.h"
namespace nova::cs2::modules {
class Time final : public Module {
public:
    Time();
    std::string suffix() const override;
private:
    void onEnable() override;
    void onDisable() override;
    void onUpdate() override;
    Properties::ModeProperty& period_;
    Properties::NumberProp &day_, &night_;
};
}
