#include "cs2/modules/ModuleManager.h"
#include "cs2/modules/render/esp2d/Esp2D.h"
#include "cs2/modules/misc/team/Team.h"
#include "cs2/modules/render/radaresp/RadarESP.h"
#include "cs2/modules/combat/recoilcontrol/RecoilControl.h"
#include "cs2/modules/combat/aimbot/Aimbot.h"
#include "cs2/modules/combat/autofire/AutoFire.h"
#include "cs2/modules/render/fovcontrol/FovControl.h"
#include "cs2/modules/render/time/Time.h"
#include "cs2/modules/render/chams/Chams.h"
#include "cs2/modules/render/watermark/Watermark.h"
#include <windows.h>
#include "ui/clickgui/ClickGui.h"
#include "ui/hud/musichud/MusicHud.h"
#include "ui/notifications/NotificationHud.h"

namespace nova::cs2::modules {
namespace {
class ArrayList final : public Module {
public:
    ArrayList() : Module("ArrayList", "render", "HUD", true, true) { addSettings(hud_.Options().Rows()); }
private:
    bool renderWhenDisabled() const override { return true; }
    void onRender() override { hud_.Draw(isEnabled(), clickgui::IsOpen(), ModuleManager::Get().Entries()); }
    ui::arraylist::ArrayListHud hud_;
};
class Music final : public Module {
public:
    explicit Music(bool preview) : Module("MusicHUD", "render", "HUD", true), preview_(preview) { addSettings(hud_.Settings().Rows()); }
private:
    void onEnable() override { if (!preview_) hud_.SetEnabled(true); }
    void onDisable() override { if (!preview_) hud_.SetEnabled(false); }
    bool renderWhenDisabled() const override { return true; }
    void onRender() override {
        if (preview_) { if (isEnabled() || clickgui::IsOpen()) hud_.DrawPreview(); }
        else hud_.Draw(clickgui::IsOpen());
    }
    bool preview_;
    ui::music::MusicHud hud_;
};
class Notifications final : public Module {
public:
    Notifications() : Module("Notifications", "render", "HUD", true, true) { addSettings(hud_.Settings().Rows()); }
private:
    bool renderWhenDisabled() const override { return true; }
    void onRender() override { hud_.Draw(isEnabled()); }
    ui::NotificationHud hud_;
};
class GuiMode final : public Mode {
public:
    GuiMode(const char* name, clickgui::Mode value) : Mode(name), value_(value) {}
    void onEnable() override { clickgui::SetMode(value_); }
private:
    clickgui::Mode value_;
};
class ClickGui final : public Module {
public:
    ClickGui() : Module("ClickGUI", "render", "General") {
        keepBindWhileMenuOpen(); keybind().setKeybind(VK_INSERT);
        hiddenInArrayListProperty().set(true);
        std::vector<std::unique_ptr<Mode>> modes;
        modes.push_back(std::make_unique<GuiMode>("Panel", clickgui::Mode::Panel));
        modes.push_back(std::make_unique<GuiMode>("Classic", clickgui::Mode::Classic));
        setModes(std::move(modes));
    }
    bool persistEnabled() const override { return false; }
    bool renderWhenDisabled() const override { return true; } // Floating windows outlive the menu toggle.
private:
    void onEnable() override { clickgui::SetOpen(true); }
    void onDisable() override { clickgui::SetOpen(false); }
    void onRender() override { clickgui::Render(); }
};
}
void RegisterBuiltinModules(bool preview) {
    auto& manager = ModuleManager::Get();
    manager.Create<Team>();
    manager.Create<RecoilControl>();
    manager.Create<Aimbot>();
    manager.Create<AutoFire>();
    manager.Create<ArrayList>(); manager.Create<Watermark>();
    manager.Create<Music>(preview); manager.Create<Notifications>();
    manager.Add(MakeEsp2D()); manager.Create<RadarESP>(); manager.Create<FovControl>(); manager.Create<Time>(); manager.Create<Chams>(); manager.Create<ClickGui>();
    manager.Seal();
}
}
