#include "minecraft/modules/combat/keepsprint/KeepSprint.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/ModuleManager.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/modules/combat/autototem/AutoTotem.h"
#include "minecraft/modules/player/autoreplenish/AutoReplenish.h"
#include "minecraft/modules/movement/invmove/InventoryMove.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_options.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "ui/clickgui/ClickGui.h"
#include "utils/Keyboard.h"
#include <cstdio>
#include <cmath>

namespace nova::module {
namespace {
std::string Decimal(double value) {
    char text[24]; std::snprintf(text, sizeof(text), "%.2f", value); return text;
}
// NumberProp quantizes with step multiplication (e.g. 12 * .05). Read the
// displayed decimal so Rise's literal .6 and fractional buffer thresholds
// do not acquire an extra binary rounding step from the UI representation.
double SettingDecimal(double value) { return std::round(value * 100.0) / 100.0; }
class KeepSprintModule final : public Module {
public:
    KeepSprintModule() : Module("KeepSprint", Category::Combat, "General", "General") {
        velocity_ = &addNumber("Hit Slow Down During Velocity", 0, 1, 0.05, 0.6, {}, Decimal);
        normal_ = &addNumber("Hit Slow Down Normal", 0, 1, 0.05, 0.6, {}, Decimal);
        decrease_ = &addNumber("Buffer Decrease", 0.1, 10, 0.1, 1, {}, Decimal,
            [this] { return buffer_->get(); });
        maximum_ = &addNumber("Max Buffer", 1, 10, 1, 5, {},
            [](double v) { return std::to_string(static_cast<int>(v)); },
            [this] { return buffer_->get(); });
        velocitySprint_ = &addBool("Velocity Hit Sprint");
        normalSprint_ = &addBool("Normal Hit Sprint");
        buffer_ = &addBool("Buffer Abuse");
        air_ = &addBool("Only In Air");
        rotation_ = &addBool("Rotation");
        hit_ = event::EventBus::Get().Subscribe<event::AttackSlowdownEvent>(
            [this](event::AttackSlowdownEvent& e) {
                if (!isEnabled()) return;
                const keepsprint::Settings settings{SettingDecimal(velocity_->get()), SettingDecimal(normal_->get()), SettingDecimal(decrease_->get()),
                    static_cast<int>(maximum_->get()), velocitySprint_->get(), normalSprint_->get(),
                    buffer_->get(), air_->get()};
                const auto result = state_.Hit(settings, e.onGround, e.hurtTime, {e.slowdown, e.sprint});
                e.slowdown = result.slowdown; e.sprint = result.sprint;
            });
        input_ = event::EventBus::Get().Subscribe<event::MoveInputEvent>(
            [this](event::MoveInputEvent& e) { RotateMovement(e); }, event::Priority::High - 1);
        tick_ = event::EventBus::Get().Subscribe<event::GameTickEvent>([this](auto& e) {
            if (e.type == event::EventType::Pre && !CanRotate()) ReleaseMovement();
        }, event::Priority::Low);
    }
private:
    bool OwnsRotation() const {
        const auto& state = rotation::State();
        return ownsRotation_ && state.active && state.direct && !state.linear &&
            state.priority == rotation::RotationPriority::Medium &&
            state.rotations.yaw == applied_.yaw && state.rotations.pitch == applied_.pitch;
    }
    bool CanRotate() const {
        const auto* speed = ModuleManager::Get().Find("Speed");
        if (speed && speed->isEnabled()) return false;
        return isEnabled() && rotation_->get() && !java::ScreenOpen() && !clickgui::IsOpen() &&
            input::IsProcessForeground() && !java::FreeCamActive() && !scaffold::IsEnabled() &&
            !invmove::SuppressSprint() && !autototem::SuppressMovement() && !autoreplenish::SuppressMovement();
    }
    void ReleaseMovement() {
        if (OwnsRotation()) rotation::ReleaseDirectRotations();
        ownsRotation_ = false;
        if (sprintKey_) {
            const auto* sprint = ModuleManager::Get().Find("Sprint");
            if (invmove::SuppressSprint() || autototem::SuppressMovement() || autoreplenish::SuppressMovement())
                java::SetSprintKeyDown(false);
            else
                java::RestoreSprintKey(sprint && sprint->isEnabled());
            sprintKey_ = false;
        }
    }
    void onDisable() override { ReleaseMovement(); }
    void RotateMovement(event::MoveInputEvent& e) {
        if (!CanRotate() || e.sneak || e.suppressSprint || e.skipRotationFix ||
            e.hasFinalForward || e.hasFinalStrafe || (e.forward == 0 && e.strafe == 0) ||
            !std::isfinite(e.forward) || !std::isfinite(e.strafe) ||
            rotation::AutomatedItemUseSubmitted()) {
            ReleaseMovement(); return;
        }
        auto player = java::PlayerEntity();
        if (!player || !player.IsAlive() || !java::LevelEntity()) { ReleaseMovement(); return; }
        // Movement is a fallback request: aiming/placement keeps its rotation.
        if (rotation::IsActive() && !OwnsRotation()) { ReleaseMovement(); return; }
        float yaw = 0, pitch = 0;
        if (!java::PlayerRotation(yaw, pitch) || !std::isfinite(yaw) || !std::isfinite(pitch)) {
            ReleaseMovement(); return;
        }
        const rotation::Rotation wanted{rotation::Direction(e.forward, e.strafe, yaw), pitch};
        if (!rotation::SetDirectRotations(wanted)) { ReleaseMovement(); return; }
        applied_ = rotation::State().rotations;
        ownsRotation_ = true;
        // RotationBridge fixes the untouched input immediately after this handler.
        // Vanilla still decides whether food, item use or collisions allow sprint.
        sprintKey_ = java::SetSprintKeyDown(true) || sprintKey_;
        e.requestSprint = true;
    }
    keepsprint::State state_;
    Properties::NumberProp *velocity_, *normal_, *decrease_, *maximum_;
    Properties::BoolProperty *velocitySprint_, *normalSprint_, *buffer_, *air_, *rotation_;
    rotation::Rotation applied_;
    bool ownsRotation_ = false, sprintKey_ = false;
    event::Subscription hit_, input_, tick_;
};
}
std::unique_ptr<Module> MakeKeepSprintModule() { return std::make_unique<KeepSprintModule>(); }
}
