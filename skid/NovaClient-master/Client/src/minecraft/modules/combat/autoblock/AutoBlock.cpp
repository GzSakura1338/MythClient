#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/modules/combat/autoblock/AutoBlock.h"
#include "minecraft/modules/combat/autoblock/AutoBlockStats.h"
#include "minecraft/modules/misc/teams/Teams.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/utils/rotation/Random.h"
#include "minecraft/utils/rotation/RayCastUtil.h"

namespace nova::module {
namespace {
namespace lunar = compat::lunar;
int Ticks(int ms) { return ms <= 0 ? 0 : (ms + 49) / 50; }
bool RightDown() { return lunar::PhysicalMouseDown(1); }
double TargetDistanceSq(const rotation::Vec3& eye, const java::EntityHandle& player) {
    java::Box box;
    double border = 0;
    if (!player.BoundingBox(box) || !player.PickRadius(border)) return std::numeric_limits<double>::max();
    const double dx = eye.x - std::clamp(eye.x, box.minX-border, box.maxX+border);
    const double dy = eye.y - std::clamp(eye.y, box.minY-border, box.maxY+border);
    const double dz = eye.z - std::clamp(eye.z, box.minZ-border, box.maxZ+border);
    return dx*dx + dy*dy + dz*dz;
}
}

void AutoBlock::Configure(const AutoBlockSettings& settings) {
    if (settings_.mode != settings.mode ||
        (settings.mode == AutoBlockMode::Hypixel &&
         (settings_.hypixel.mode != settings.hypixel.mode || settings_.hypixel.lagMode != settings.hypixel.lagMode))) {
        Reset(true);
        lunar::EndBlockLag();
        lunar::SetAutoBlockState(false, false);
        tick_ = 0;
    }
    settings_ = settings;
}

void AutoBlock::SetEnabled(bool enabled) {
    enabled = enabled && compat::IsLegacyLunar();
    if (enabled_ == enabled) return;
    enabled_ = enabled;
    lunar::SetAutoBlockEnabled(enabled);
    if (enabled) { tick_ = 0; Reset(false); }
    else Reset(true);
}

void AutoBlock::Start() {
    if (!lunar::HoldingSword()) return;
    lunar::StartSwordBlock();
    blocking_ = true;
    startTick_ = tick_;
}

void AutoBlock::Stop(bool forceRelease) {
    if (!blocking_ && !forceRelease) return;
    lunar::StopSwordBlock();
    blocking_ = false;
    startTick_ = -1;
}

void AutoBlock::ReleaseLag() {
    if (!lagWindow_) return;
    lunar::ReleaseBlockLag();
    lagWindow_ = false;
    lagStartTick_ = -1;
}

void AutoBlock::Render() {
    bool animation = false;
    bool managed = false;
    // Use the whole block cycle, not just the local item-use state: attacking
    // and releasing must not interrupt the visual pose. Publish input ownership
    // separately; visual state never starts or stops a block packet cycle.
    switch (settings_.mode) {
    case AutoBlockMode::Lag:
        animation = blocking_ || lagWindow_;
        managed = (blocking_ && !manual_) || lagWindow_;
        break;
    case AutoBlockMode::Vanilla: managed = animation = vanillaManaged_; break;
    case AutoBlockMode::Grim: managed = animation = grimPhase_ != GrimPhase::Idle; break;
    case AutoBlockMode::Hypixel:
        animation = hypixel_.fakeBlockState;
        managed = hypixel_.isBlocking;
        break;
    case AutoBlockMode::Slinky:
        lunar::DrainSlinkyBlockLag();
        managed = animation = blocking_ || lunar::SlinkyBlockLagging();
        break;
    case AutoBlockMode::LagNew: managed = animation = lagNew_.active; break;
    }
    const bool ready = enabled_ &&
        java::PlayerEntity() && !java::ScreenOpen() && lunar::HoldingSword();
    // Input ownership is the live automatic cycle, independent of the visual pose.
    lunar::SetAutoBlockState(ready && managed, ready && settings_.animation && animation);
    const bool suppressManual = ready && settings_.disableManualBlock && !managed;
    lunar::SetAutoBlockManualSuppressed(suppressManual);
    // Keep vanilla interaction input. The native item-use fallback suppresses
    // sword blocking after chests/doors/entities have handled the right click.
    if (suppressManual) {
        // Slinky clears blocking_ before vanilla consumes the released key.
        // Restoring a held RMB here cancels that release and also consumes the
        // vanilla right-click cooldown between automatic holds. Slinky's idle
        // input stays released; real press-count clicks still open containers
        // through BlockVanillaUseHook without starting a manual sword block.
        lunar::SetUseKeyDown(settings_.mode != AutoBlockMode::Slinky && lunar::PhysicalUseDown());
    }
}

void AutoBlock::Reset(bool releaseUseKey) {
    if (!compat::IsLegacyLunar()) return;
    lunar::SetAutoBlockManualSuppressed(false);
    if (settings_.mode == AutoBlockMode::Slinky) {
        ResetSlinky();
        autoblock::SetCoverageActive(false);
        lunar::SetAutoBlockState(false, false);
        return;
    }
    if (settings_.mode == AutoBlockMode::Hypixel) ResetHypixel();
    if (settings_.mode == AutoBlockMode::LagNew) {
        ResetLagNew();
        lunar::SetAutoBlockState(false, false);
        if (releaseUseKey) lunar::SetUseKeyDown(RightDown() && !java::ScreenOpen());
        return;
    }
    autoblock::SetCoverageActive(false);
    if (grimPhase_ != GrimPhase::Idle) {
        lunar::ClearCombatClicks();
        if (grimPhase_ == GrimPhase::Blinking || grimPhase_ == GrimPhase::Reblocking ||
            grimPhase_ == GrimPhase::Draining)
            lunar::EndBlockLag();
    }
    grimPhase_ = GrimPhase::Idle;
    grimPendingAttack_ = false;
    grimTicks_ = 0;
    grimSlot_ = -1;
    grimCooldown_ = 0;
    grimMovement_ = 0;
    grimResetSequence_ = lunar::GrimResetSequence();
    if (vanillaManaged_) lunar::ClearCombatClicks();
    vanillaManaged_ = vanillaPendingAttack_ = vanillaReleasing_ = false;
    vanillaHeldTicks_ = 0;
    vanillaReleaseMovement_ = 0;
    ReleaseLag();
    Stop(releaseUseKey);
    manual_ = false;
    lunar::SetAutoBlockState(false, false);
    if (RightDown() && !java::ScreenOpen()) lunar::SetUseKeyDown(true);
    lastHurt_ = 0;
}

java::EntityHandle AutoBlock::FindTarget() {
    auto self = java::PlayerEntity();
    rotation::Vec3 eye, position;
    if (!self || !self.EyePosition(1, eye) || !self.Position(position)) return {};
    const double range = settings_.mode == AutoBlockMode::Hypixel ? settings_.hypixel.range : settings_.range;
    auto valid = [&](const java::EntityHandle& player) {
        if (!player || !player.IsPlayer() || player.IsSame(self) || lunar::EntityDead(player.raw())) return false;
        int death = 0;
        if (!player.DeathTime(death) || death != 0) return false;
        if (teams::IsTeammate(self, player) || antibot::IsBot(player)) return false;
        return TargetDistanceSq(eye, player) <= range * range;
    };
    java::EntityHandle hovered;
    if (java::AttackTargetEntity(hovered) && valid(hovered)) return hovered;
    std::vector<java::EntityHandle> players;
    if (!java::WorldPlayers(players)) return {};
    java::EntityHandle closest;
    double best = std::numeric_limits<double>::max();
    for (auto& player : players) {
        if (!valid(player)) continue;
        const double distance = TargetDistanceSq(eye, player);
        if (distance < best) { best = distance; closest = std::move(player); }
    }
    return closest;
}

bool AutoBlock::OwnsItemUse() {
    if (blocking_ || lagWindow_ || vanillaManaged_ || vanillaPendingAttack_ || vanillaReleasing_ ||
        grimPhase_ != GrimPhase::Idle || hypixel_.isBlocking || hypixel_.swapped || hypixel_.postBlock ||
        lagNew_.active || lagNewQueue_ || slinky_.awaitingUse) return true;
    if (!enabled_ || !lunar::HoldingSword()) return false;
    // Hypixel and Slinky may start after the projectile listener this tick.
    // Reserve their eligible nearby target before either writes packets/keys.
    if (settings_.mode != AutoBlockMode::Slinky) return static_cast<bool>(FindTarget());
    const auto self = java::PlayerEntity();
    return self && SlinkyEnemyNearby(self);
}

void AutoBlock::Tick(const AutoBlockSettings& settings, bool attackRequested, bool captureClicks) {
    Configure(settings);
    if (!enabled_) return;
    switch (settings_.mode) {
    case AutoBlockMode::Lag: TickLag(); break;
    case AutoBlockMode::Vanilla: TickVanilla(attackRequested, captureClicks); break;
    case AutoBlockMode::Grim: TickGrim(attackRequested, captureClicks); break;
    case AutoBlockMode::Hypixel: break; // Runs at EntityPlayerSP.onUpdate HEAD/RETURN.
    case AutoBlockMode::LagNew: break; // PrePlayerInteract / PostPlayerTick own this cycle.
    case AutoBlockMode::Slinky: TickSlinky(); break;
    }
    autoblock::SetCoverageActive(blocking_ || lagWindow_ || vanillaManaged_ ||
        grimPhase_ != GrimPhase::Idle ||
        (settings_.mode == AutoBlockMode::Slinky && lunar::SlinkyBlockLagging()));
    Render();
}

void AutoBlock::TickGrim(bool attackRequested, bool captureClicks) {
    const auto resetSequence = lunar::GrimResetSequence();
    if (grimResetSequence_ != resetSequence) {
        Reset(true);
        grimCooldown_ = 2;
        return;
    }
    auto self = java::PlayerEntity();
    if (!self || lunar::EntityDead(self.raw()) || java::ScreenOpen()) { Reset(true); return; }
    if (grimCooldown_ > 0) { --grimCooldown_; return; }

    int slot = -1;
    if (!lunar::SelectedSlot(slot)) { Reset(true); return; }
    if (grimPhase_ != GrimPhase::Idle && slot != grimSlot_) {
        Reset(true);
        return;
    }
    auto target = FindTarget();
    if (!lunar::HoldingSword() || !target || (settings_.requireRight && !RightDown())) {
        if (grimPhase_ != GrimPhase::Idle) { Reset(true); return; }
        if (lunar::HoldingSword()) lunar::SetUseKeyDown(RightDown());
        java::EntityHandle hovered;
        if (attackRequested && !lunar::PlayerUsingItem() && java::AttackTargetEntity(hovered) &&
            !(teams::IsTeammate(self, hovered) || antibot::IsBot(hovered)))
            lunar::ClickAttack();
        return;
    }
    if (captureClicks) attackRequested |= lunar::PendingAttackClick();
    if (!lunar::ClearCombatClicks()) { Reset(true); return; }
    grimPendingAttack_ |= attackRequested;
    const bool usingItem = lunar::PlayerUsingItem();
    blocking_ = usingItem;

    if (grimPhase_ == GrimPhase::Idle) {
        if (java::AttackTargetIsBlock()) { lunar::SetUseKeyDown(false); return; }
        grimSlot_ = slot;
        grimTicks_ = 0;
        grimMovement_ = lunar::MovementPacketSequence();
        if (usingItem || lunar::StartSwordBlock()) grimPhase_ = GrimPhase::Blocking;
        return;
    }

    if (grimPhase_ == GrimPhase::Reblocking) {
        if (lunar::BlockLagging()) lunar::EndBlockLag();
        grimPhase_ = GrimPhase::Blocking;
    }
    if (grimPhase_ == GrimPhase::Blocking) {
        if (!usingItem) {
            grimTicks_ = 0;
            grimMovement_ = lunar::MovementPacketSequence();
            if (!java::AttackTargetIsBlock()) lunar::StartSwordBlock();
            return;
        }
        ++grimTicks_;
        const bool earlyAttack = settings_.releaseOnAttack && grimPendingAttack_;
        if ((!earlyAttack && grimTicks_ < settings_.holdTicks) ||
            lunar::MovementPacketSequence() == grimMovement_) {
            lunar::SetUseKeyDown(true);
            return;
        }
        if (!lunar::BeginGrimBlockLag(settings_.blinkTicks * 50)) return;
        grimMovement_ = lunar::MovementPacketSequence();
        grimTicks_ = 0;
        grimPhase_ = GrimPhase::Blinking;
        lunar::SetUseKeyDown(false);
        return;
    }

    lunar::SetUseKeyDown(false);
    if (grimPhase_ == GrimPhase::Blinking) {
        ++grimTicks_;
        const bool expired = grimTicks_ >= settings_.blinkTicks || lunar::GrimBlockLagExpired();
        const bool attack = settings_.releaseOnAttack && grimPendingAttack_ && !usingItem;
        if (!expired && !attack) return;
        if (settings_.moveTicks == 0 && !usingItem && !java::AttackTargetIsBlock() &&
            lunar::PrepareGrimBlockLagRelease(grimMovement_)) {
            if (lunar::StartSwordBlock()) {
                if (grimPendingAttack_) {
                    java::EntityHandle hovered;
                    if (java::AttackTargetEntity(hovered) && hovered.IsSame(target)) lunar::ClickAttack();
                    grimPendingAttack_ = false;
                }
                grimTicks_ = 0;
                grimPhase_ = GrimPhase::Reblocking;
                return;
            }
        }
        lunar::EndBlockLag();
        grimPhase_ = GrimPhase::Draining;
        grimTicks_ = 0;
    }
    if (usingItem || lunar::MovementPacketSequence() == grimMovement_) return;

    if (grimPendingAttack_) {
        java::EntityHandle hovered;
        if (java::AttackTargetEntity(hovered) && hovered.IsSame(target)) lunar::ClickAttack();
        grimPendingAttack_ = false;
    }
    // Leave real, unbuffered movement ticks between block cycles. Vanilla can
    // accelerate and resume sprinting here; don't extend Blink or alter velocity.
    // Require a fresh movement boundary for each tick, including before reblock.
    if (grimTicks_ < settings_.moveTicks) {
        ++grimTicks_;
        grimMovement_ = lunar::MovementPacketSequence();
        return;
    }
    grimPhase_ = GrimPhase::Idle;
    grimTicks_ = 0;
    if (!java::AttackTargetIsBlock()) {
        grimMovement_ = lunar::MovementPacketSequence();
        if (lunar::StartSwordBlock()) grimPhase_ = GrimPhase::Blocking;
    }
}

void AutoBlock::TickVanilla(bool attackRequested, bool captureClicks) {
    auto self = java::PlayerEntity();
    if (!self || lunar::EntityDead(self.raw()) || java::ScreenOpen()) {
        Reset(true);
        return;
    }

    const bool usingItem = lunar::PlayerUsingItem();
    if (vanillaReleasing_ && !usingItem && lunar::MovementPacketSequence() == vanillaReleaseMovement_) {
        if (captureClicks) attackRequested |= lunar::PendingAttackClick();
        lunar::ClearCombatClicks();
        lunar::SetUseKeyDown(false);
        vanillaPendingAttack_ |= attackRequested;
        return;
    }

    auto target = FindTarget();
    if (!lunar::HoldingSword() || !target || (settings_.requireRight && !RightDown())) {
        if (vanillaManaged_) Reset(true);
        if (lunar::HoldingSword()) lunar::SetUseKeyDown(RightDown());
        java::EntityHandle hovered;
        if (attackRequested && !lunar::PlayerUsingItem() && java::AttackTargetEntity(hovered) &&
            !(teams::IsTeammate(self, hovered) || antibot::IsBot(hovered)))
            lunar::ClickAttack();
        return;
    }

    if (captureClicks) attackRequested |= lunar::PendingAttackClick();
    if (!lunar::ClearCombatClicks()) { Reset(true); return; }
    vanillaManaged_ = true;
    vanillaPendingAttack_ |= attackRequested;
    blocking_ = usingItem;

    if (usingItem) {
        ++vanillaHeldTicks_;
        if (vanillaReleasing_ || (vanillaPendingAttack_ && vanillaHeldTicks_ >= settings_.holdTicks)) {
            vanillaReleasing_ = true;
            vanillaReleaseMovement_ = lunar::MovementPacketSequence();
            lunar::SetUseKeyDown(false);
        } else {
            lunar::SetUseKeyDown(true);
        }
        return;
    }

    vanillaReleasing_ = false;
    vanillaHeldTicks_ = 0;

    if (vanillaPendingAttack_) {
        java::EntityHandle hovered;
        if (java::AttackTargetEntity(hovered) && hovered.IsSame(target)) lunar::ClickAttack();
        vanillaPendingAttack_ = false;
    }
    if (!java::AttackTargetIsBlock()) lunar::StartSwordBlock();
    else lunar::SetUseKeyDown(false);
}

void AutoBlock::TickLag() {
    const auto& settings = settings_;
    auto self = java::PlayerEntity();
    if (!self || lunar::EntityDead(self.raw()) || java::ScreenOpen()) { Reset(true); return; }
    const int hurt = java::PlayerHurtTime();
    const bool hurtAgain = hurt > lastHurt_;
    lastHurt_ = hurt;
    if (!lunar::HoldingSword()) { Reset(false); return; }
    ++tick_;
    auto target = FindTarget();
    const bool right = RightDown();
    const bool left = lunar::PhysicalMouseDown(0) || target.Valid();
    if (!right) { Reset(true); return; }
    if (!target && settings_.disableManualBlock) {
        ReleaseLag(); Stop(true); manual_ = false;
        return;
    }
    if (!left) {
        if (lagWindow_) ReleaseLag();
        if (!blocking_) { Start(); manual_ = true; }
        return;
    }
    if (manual_) { Stop(true); manual_ = false; }
    const bool conditions = target.Valid() && (!settings.requireLeft || left) &&
                            (!settings.requireRight || right);
    if (lagWindow_) {
        const int maxTicks = Ticks(settings.lagMs);
        const bool expired = maxTicks > 0 && lagStartTick_ >= 0 && tick_ - lagStartTick_ >= maxTicks;
        if (expired || !conditions) {
            ReleaseLag();
            if (expired && settings.blockAgain && conditions) Start();
        }
    }
    if (!conditions) {
        Stop(true);
        lunar::SetUseKeyDown(lunar::PhysicalUseDown());
        return;
    }
    if (!blocking_ && !lagWindow_) {
        const int trigger = std::clamp(static_cast<int>(std::lround(settings.hurtMs / 50.0)), 1, 10);
        if (!settings.damaged || hurt == trigger) Start();
    }
    if (blocking_) {
        const int hold = Ticks(settings.holdMs);
        const bool expired = hold > 0 && startTick_ >= 0 && tick_ - startTick_ >= hold;
        if (expired || (settings.damaged && hurtAgain)) {
            const bool chance = settings.lagChance >= 100 ||
                (settings.lagChance > 0 && rotation::Random() * 100 < settings.lagChance);
            if (chance && !lagWindow_) {
                const int reference = startTick_ >= 0 ? startTick_ : tick_;
                const int maxTicks = Ticks(settings.lagMs);
                if (maxTicks <= 0 || tick_ - reference < maxTicks) {
                    lunar::BeginBlockLag();
                    lagWindow_ = true;
                    lagStartTick_ = reference;
                }
            }
            Stop(true);
        }
    }
}

void AutoBlock::OnAttackPacket() {
    switch (settings_.mode) {
    case AutoBlockMode::Lag: OnLagAttackPacket(); break;
    case AutoBlockMode::Vanilla: break;
    case AutoBlockMode::Grim: break;
    case AutoBlockMode::Hypixel: break;
    case AutoBlockMode::LagNew: break;
    case AutoBlockMode::Slinky: break; // Queue reacts without touching gameplay state off-thread.
    }
}

void AutoBlock::OnLagAttackPacket() {
    if (!enabled_ || !lagWindow_ || !settings_.preventDelayAttacks) return;
    ReleaseLag();
    if (settings_.blockAgain && lunar::HoldingSword()) Start();
}

}
