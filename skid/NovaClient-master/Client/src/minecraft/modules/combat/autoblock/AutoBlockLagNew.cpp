#include "minecraft/modules/combat/autoblock/AutoBlock.h"
#include "minecraft/modules/combat/autoblock/AutoBlockPacketSync.h"
#include "minecraft/modules/combat/autoblock/AutoBlockStats.h"
#include "minecraft/modules/combat/killaura/KillAura.h"
#include "minecraft/modules/combat/antikb/AntiKB.h"
#include "minecraft/modules/combat/velocity/Velocity.h"
#include "minecraft/modules/misc/teams/Teams.h"
#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/RayCastUtil.h"
#include <windows.h>

namespace nova::module {
namespace {
namespace lunar = compat::lunar;
struct LagNewIO {
    java::EntityHandle& target;
    bool& queue;
    bool& clicked;
    bool eligible = false, manual = false;
    bool Eligible() const {
        auto self = java::PlayerEntity();
        return eligible && self && !lunar::EntityDead(self.raw()) && !java::ScreenOpen() &&
            lunar::HoldingSword() &&
            (!lunar::BlockLagging() || queue) &&
            antikb::AttackQueue() == 0 && velocity::AttackQueue() == 0;
    }
    std::uint64_t Now() const { return GetTickCount64(); }
    std::uint64_t Movement() const { return lunar::BlockInputMovementSequence(); }
    bool Blocking() const { return lunar::HypixelPlayerBlocking(); }
    bool Begin() {
        if (lunar::BlockLagging() && !queue) return false;
        lunar::BeginHypixelBlockLag();
        queue = lunar::HypixelBlockLagging();
        autoblock::SynchronizePacketDelays();
        queue = queue && lunar::HypixelBlockLagging();
        // Watchdog can deny blink while waiting for an attack to release S12.
        // KillAura has delegated its attacks to this cycle, so denying the
        // cycle too would deadlock both modules. Continue the same release /
        // attack / block stages without owning a blink queue. The native
        // Watchdog adapter still orders actions and acknowledgements at C03.
        // Never adopt or flush another mode's queue to make this fallback work.
        return queue || (!lunar::BlockLagging() && Eligible());
    }
    void Flush() {
        if (queue) lunar::EndHypixelBlockLag();
        queue = false;
        autoblock::SynchronizePacketDelays();
    }
    bool Release() { return lunar::StopSwordBlockDirect(); }
    bool Attack(bool& hit) {
        hit = false;
        if ((manual && !clicked) || lunar::HypixelPlayerBusy()) return false;
        const auto ray = rotation::LeaderRayCast(rotation::GetRotation(), 3.0);
        if (ray.type == rotation::HitResultType::Entity && ray.entity &&
            target && target.IsSame(ray.entity) && !teams::IsTeammate(target) && !antibot::IsBot(target)) {
            lunar::SwingHand();
            hit = lunar::AttackEntity(target);
        } else {
            // Reference T() still swings when the block-range target isn't under the ray.
            lunar::SwingHand();
        }
        clicked = false;
        return true;
    }
    bool Block(bool interact) {
        // Another module (or an attack whose JNI call failed after sending C02)
        // can also own the attack in this movement interval. Never fall back to
        // bare C08 after any accepted attack when the late ray no longer hits.
        if (interact || lunar::BlockInputAttacked()) {
            if (!target || lunar::EntityDead(target.raw())) return false;
            const auto ray = rotation::LeaderRayCast(rotation::GetRotation(), 8.0);
            if (ray.type == rotation::HitResultType::Entity && ray.entity && target.IsSame(ray.entity))
                return lunar::InteractSwordBlock(target, ray.location);
            return false;
        }
        return lunar::StartSwordBlockDirect();
    }
};
}

void AutoBlock::TickLagNew(bool auraEnabled, bool attackRequested) {
    if (!enabled_ || settings_.mode != AutoBlockMode::LagNew) return;
    const auto sequence = lunar::GrimResetSequence();
    if (lagNewResetSequence_ != sequence) {
        ResetLagNew(); Render(); return;
    }
    // AntiKB may flush our output between callbacks. Keep the input phase and
    // its movement boundary; restarting here could release in a use interval.
    if (lagNewQueue_ && !lunar::HypixelBlockLagging()) lagNewQueue_ = false;
    lagNewClick_ |= attackRequested || lunar::PendingAttackClick();
    bool nearby = false;
    if (auraEnabled) {
        int delay = 0;
        if (!killaura::ReadHypixelTarget(lagNewTarget_, settings_.range, nearby, delay)) nearby = false;
    } else {
        lagNewTarget_ = FindTarget();
        nearby = lagNewTarget_.Valid();
    }
    LagNewIO io{lagNewTarget_, lagNewQueue_, lagNewClick_,
        nearby && (!settings_.requireRight || lunar::PhysicalUseDown()), settings_.lagNewManualClick};
    lagNew_.Pre(io, settings_.lagNewApsMode);
    if (lagNew_.active) lunar::ClearCombatClicks();
    else { lagNewClick_ = false; lagNewTarget_.Reset(); }
    blocking_ = lagNew_.blocking;
    lunar::SetHypixelBlockManaged(lagNew_.active);
    autoblock::SetCoverageActive(lagNew_.active);
    Render();
}

void AutoBlock::PostLagNew() {
    if (!enabled_ || settings_.mode != AutoBlockMode::LagNew || !lagNew_.active) return;
    if (lagNewResetSequence_ != lunar::GrimResetSequence()) { ResetLagNew(); Render(); return; }
    LagNewIO io{lagNewTarget_, lagNewQueue_, lagNewClick_,
        lagNewTarget_.Valid() && !lunar::EntityDead(lagNewTarget_.raw()) &&
        (!settings_.requireRight || lunar::PhysicalUseDown()), settings_.lagNewManualClick};
    lagNew_.Post(io);
    blocking_ = lagNew_.blocking;
    lunar::SetHypixelBlockManaged(lagNew_.active);
    autoblock::SetCoverageActive(lagNew_.active);
    Render();
}

void AutoBlock::ResetLagNew() {
    LagNewIO io{lagNewTarget_, lagNewQueue_, lagNewClick_};
    lagNew_.Reset(io);
    lagNewTarget_.Reset(); lagNewClick_ = false; blocking_ = false;
    lunar::SetHypixelBlockManaged(false);
    lunar::ResetHypixelPacketState();
    lagNewResetSequence_ = lunar::GrimResetSequence();
    autoblock::SetCoverageActive(false);
}
}
