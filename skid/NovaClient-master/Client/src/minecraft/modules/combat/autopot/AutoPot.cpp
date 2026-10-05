#include "minecraft/modules/combat/autopot/AutoPotLogic.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_potion.h"
#include "ui/clickgui/ClickGui.h"
#include "utils/Keyboard.h"
#include <format>

namespace nova::module {
namespace {
struct PotionIO {
    bool Selected(int& slot) { return java::SelectedSlot(slot); }
    bool Select(int slot) { return java::SetSelectedSlot(slot); }
    bool Healing(int slot) { return java::HotbarHasHealingSplashPotion(slot); }
    bool Sync() { return java::SyncSelectedItem(); }
    bool Use() { return java::UseSelectedHealingPotion(); }
};

class AutoPotModule final : public Module {
public:
    AutoPotModule() : Module("AutoPot", Category::Combat, "General", "General") {
        maximumHealth_ = &addNumber("Max Health", 1, 20, 1, 12, {},
            [](double value) { return std::format("{:.0f}", value); });
        tick_ = event::EventBus::Get().Subscribe<event::GameTickEvent>([this](auto& e) {
            if (e.type == event::EventType::Pre && isEnabled()) OnTick();
        });
    }
private:
    void onEnable() override { state_ = {}; }
    void onDisable() override { state_ = {}; }
    void OnTick() {
        if(automlg::IsWorking()||autocharge::IsWorking())return;
        auto player = java::PlayerEntity();
        if (!player || !java::LevelEntity()) { state_ = {}; return; }
        if (clickgui::IsOpen() || java::ScreenOpen() || !input::IsProcessForeground() ||
            !player.IsAlive() || player.IsSpectator()) return;
        float health = 0, absorption = 0;
        if (!player.Health(health, absorption)) return;
        PotionIO io;
        state_.Tick(io, true, health, maximumHealth_->get(), GetTickCount64());
    }
    autopot::State state_;
    Properties::NumberProp* maximumHealth_ = nullptr;
    event::Subscription tick_;
};
}
std::unique_ptr<Module> MakeAutoPotModule() { return std::make_unique<AutoPotModule>(); }
}
