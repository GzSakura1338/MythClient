#pragma once
#include "cs2/modules/Module.h"

namespace nova::cs2::modules {
class FovControl final : public Module {
public:
    FovControl();
private:
    void onEnable() override;
    void onDisable() override;
    void onUpdate() override;
    Properties::NumberProp& degrees_;
    Properties::BoolProperty &preserveZoom_, &removeShake_;
};
}
