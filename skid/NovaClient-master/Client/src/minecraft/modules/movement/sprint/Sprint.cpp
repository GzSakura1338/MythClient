#include <memory>

#include "minecraft/modules/Module.h"
#include "minecraft/modules/combat/autototem/AutoTotem.h"
#include "minecraft/modules/player/autoreplenish/AutoReplenish.h"
#include "minecraft/modules/movement/invmove/InventoryMove.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/movement/sprint/MovementResume.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_options.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_click.h"
#include "ui/clickgui/ClickGui.h"
#include "utils/Keyboard.h"

namespace nova::module {
namespace {

class SprintModule final : public Module {
public:
    SprintModule() : Module("Sprint", Category::Movement, "General", "General") {
        tick_ = event::EventBus::Get().Subscribe<event::GameTickEvent>([this](auto& e) {
            if (e.type == event::EventType::Pre) ResumeMovement();
        }, event::Priority::Highest);
    }

private:
    void onUpdate() override {
        nova::java::SetSprintKeyDown(!invmove::SuppressSprint()&&!autototem::SuppressMovement()&&!autoreplenish::SuppressMovement());
    }

    void onDisable() override {
        nova::java::SetSprintKeyDown(false);
        resume_ = {};
        player_.Reset(); world_.Reset();
    }

    void ResumeMovement() {
        if (!isEnabled() || clickgui::IsOpen() || !input::IsProcessForeground()) {
            resume_ = {}; player_.Reset(); world_.Reset(); return;
        }
        auto player = java::PlayerEntity(), world = java::LevelEntity();
        if (!player || !world) { resume_ = {}; player_.Reset(); world_.Reset(); return; }
        if (!player_ || !world_ || !player.IsSame(player_) || !world.IsSame(world_)) {
            resume_.WorldChanged(); player_ = std::move(player); world_ = std::move(world);
        }
        if (!player_.IsAlive()) return;
        JniAttach attach;
        if (!attach) return;
        auto screen = java::CurrentScreen();
        sprint::Screen kind = sprint::Screen::None;
        if (screen) {
            kind = java::ScreenIsContainerScreen(screen) ? sprint::Screen::Container : sprint::Screen::Other;
            attach.env->DeleteLocalRef(screen);
        } else if (java::ScreenOpen()) return;
        resume_.Tick(true, kind, [] { return java::RestoreMovementKeys(); });
    }
    sprint::MovementResume resume_;
    java::EntityHandle player_, world_;
    event::Subscription tick_;
};

}

std::unique_ptr<Module> MakeSprintModule() {
    return std::make_unique<SprintModule>();
}

}
