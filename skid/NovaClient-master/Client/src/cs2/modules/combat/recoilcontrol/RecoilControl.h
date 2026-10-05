#pragma once
#include "cs2/modules/Module.h"

namespace nova::cs2::modules {
class RecoilControl final : public Module {
public:
    RecoilControl();
private:
    void onEnable() override;
    void onDisable() override;
    void onUpdate() override;
    Properties::NumberProp &horizontal_, &vertical_, &start_, &smooth_;
};
}
