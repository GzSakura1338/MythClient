#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "ui/clickgui/ClickGui.h"
#include "ui/hud/musichud/MusicHud.h"

namespace nova::module {
namespace {
class MusicHUD final : public Module {
public:
    MusicHUD() : Module("MusicHUD", Category::Render, "General", "Overlay") {
        enableOverlay();
        addSettings(view_.Settings().Rows());
    }
private:
    void onEnable() override { view_.SetEnabled(true); }
    void onDisable() override { view_.SetEnabled(false); }
    bool renderWhenDisabled() const override { return true; }
    void onRender() override { view_.Draw(clickgui::IsOpen()); }
    ui::music::MusicHud view_;
};
}
std::unique_ptr<Module> MakeMusicHudModule() { return std::make_unique<MusicHUD>(); }
}
