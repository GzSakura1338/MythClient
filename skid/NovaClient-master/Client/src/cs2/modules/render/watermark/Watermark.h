#pragma once
#include "cs2/modules/Module.h"
#include "cs2/sdk/hud/HudSnapshot.h"
#include "ui/hud/watermark/WatermarkHud.h"

namespace nova::cs2::modules {
class Watermark final : public Module {
public:
    Watermark();
private:
    void onEnable() override;
    void onUpdate() override;
    void onRender() override;
    bool renderWhenDisabled() const override { return true; }
    ui::watermark::WatermarkHud hud_;
    ui::watermark::Snapshot data_;
    sdk::hud::TickRate ticks_;
    double nextSample_ = 0;
};
}
