#include "minecraft/modules/Module.h"
#include "minecraft/modules/ModuleManager.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/modules/combat/autototem/AutoTotem.h"
#include "minecraft/modules/combat/killaura/KillAura.h"
#include "minecraft/modules/player/autoreplenish/AutoReplenish.h"
#include "minecraft/modules/movement/invmove/InventoryMove.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_options.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_speed.h"
#include "minecraft/java/api/game_player.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "ui/clickgui/ClickGui.h"
#include "utils/Keyboard.h"
#include <cmath>

namespace nova::module {
namespace {
class SpeedModule final : public Module {
public:
    SpeedModule() : Module("Speed", Category::Movement, "General", "General") {
        input_ = event::EventBus::Get().Subscribe<event::MoveInputEvent>(
            [this](auto& e) { Move(e); }, event::Priority::High - 1);
        tick_ = event::EventBus::Get().Subscribe<event::GameTickEvent>([this](auto& e) {
            if (e.type == event::EventType::Pre && !CanRun()) Release();
        }, event::Priority::Low);
    }
private:
    bool CanRun() const {
        return isEnabled() && !java::ScreenOpen() && !clickgui::IsOpen() && input::IsProcessForeground() &&
            !java::FreeCamActive() && !scaffold::IsEnabled() && !invmove::SuppressSprint() &&
            !autototem::SuppressMovement() && !autoreplenish::SuppressMovement();
    }
    bool OwnsRotation() const {
        const auto& state = rotation::State();
        return ownsRotation_ && state.active && state.direct && !state.linear &&
            state.priority == rotation::RotationPriority::Medium &&
            state.rotations.yaw == applied_.yaw && state.rotations.pitch == applied_.pitch;
    }
    void Release() {
        if (OwnsRotation()) rotation::ReleaseDirectRotations();
        ownsRotation_ = false;
        if (sprintKey_) {
            if (invmove::SuppressSprint() || autototem::SuppressMovement() || autoreplenish::SuppressMovement())
                java::SetSprintKeyDown(false);
            else {
                const auto* sprint = ModuleManager::Get().Find("Sprint");
                java::RestoreSprintKey(sprint && sprint->isEnabled());
            }
            sprintKey_ = false;
        }
    }
    void onDisable() override { Release(); }
    void Hop(event::MoveInputEvent& e, bool ground) {
        if (!ground) return;
        java::ResetPlayerJumpDelay();
        e.jump = true;
    }
    void Move(event::MoveInputEvent& e) {
        if (!CanRun() || e.sneak || e.suppressSprint || e.skipRotationFix ||
            e.hasFinalForward || e.hasFinalStrafe || (e.forward == 0 && e.strafe == 0) ||
            !std::isfinite(e.forward) || !std::isfinite(e.strafe) ||
            rotation::AutomatedItemUseSubmitted()) {
            Release(); return;
        }
        java::SpeedEnvironment environment;
        float yaw = 0, pitch = 0;
        if (!java::ReadSpeedEnvironment(environment) || environment.blocked ||
            !java::PlayerRotation(yaw, pitch) || !std::isfinite(yaw) || !std::isfinite(pitch)) {
            Release(); return;
        }
        if (rotation::IsActive() && !OwnsRotation()) {
            const auto& state = rotation::State();
            // Aura owns aiming; hopping does not require acquiring its rotation.
            // Exclusive placement/MLG and other rotation modes still take priority.
            const bool aura = killaura::HasTargets() && state.active && !state.direct && !state.linear &&
                state.priority == rotation::RotationPriority::Medium && !rotation::BlocksManualItemUse();
            Release();
            if (aura) Hop(e, environment.ground);
            return;
        }
        const float direction = rotation::Direction(e.forward, e.strafe, yaw);
        // Vanilla's sprint jump impulse follows yaw. Keep takeoff straight,
        // then use diagonal input during flight for the legacy 0.98 normalization gain.
        const rotation::Rotation wanted{direction + (environment.ground ? 0.f : 45.f), pitch};
        if (!rotation::SetDirectRotations(wanted)) { Release(); return; }
        applied_ = rotation::State().rotations;
        ownsRotation_ = true;
        // RotationBridge applies movement fix next; no raw speed/velocity writes.
        Hop(e, environment.ground);
        e.requestSprint = true;
        sprintKey_ = java::SetSprintKeyDown(true) || sprintKey_;
    }
    rotation::Rotation applied_;
    bool ownsRotation_ = false, sprintKey_ = false;
    event::Subscription input_, tick_;
};
}
std::unique_ptr<Module> MakeSpeedModule() { return std::make_unique<SpeedModule>(); }
}
