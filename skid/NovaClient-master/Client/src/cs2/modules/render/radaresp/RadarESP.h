#pragma once
#include "cs2/modules/Module.h"

namespace nova::cs2::modules {
class RadarESP final : public Module {
public:
    RadarESP();
private:
    void onEnable() override;
    void onDisable() override;
};
}
