#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/modules/player/breaker/Breaker.h"
#include "minecraft/modules/combat/autoblock/AutoBlock.h"
#include "minecraft/modules/combat/autoblock/AutoBlockPacketSync.h"
#include "minecraft/modules/combat/autoblock/AutoBlockStats.h"
#include "minecraft/modules/combat/killaura/KillAura.h"
#include "minecraft/modules/combat/antikb/AntiKB.h"
#include "minecraft/modules/combat/antikb/prediction/PredictionRuntime.h"
#include "minecraft/modules/combat/velocity/Velocity.h"
#include "minecraft/modules/misc/teams/Teams.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_player.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/Random.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace nova::module {
namespace {
namespace lunar = compat::lunar;

struct HypixelIO {
    java::EntityHandle target;
    bool attackAllowed = false, blockTarget = false, requireRight = false;
    bool automaticAttack = false, clicked = false;
    bool consumed = false;
    java::Box targetBox;
    rotation::Vec3 targetPosition;
    bool geometryValid = false;

    void CaptureGeometry() {
        if (automaticAttack) {
            geometryValid = killaura::ReadHypixelGeometry(targetBox, targetPosition);
        } else {
            double border = 0;
            geometryValid = target && target.BoundingBox(targetBox) && target.PickRadius(border) &&
                target.Position(targetPosition);
            if (geometryValid) targetBox = java::BoxInflate(targetBox, border);
        }
    }
    int normalDelay = 71; // Leader default unblocked 14 APS.

    bool CanAttack() const {
        return attackAllowed && target && !java::ScreenOpen() &&
            (!lunar::GameModeDestroying() || (breaker::OwnsMining() && !breaker::BlocksAura())) &&
            (!lunar::PlayerUsingItem() || lunar::HoldingSword());
    }
    bool CanBlock() const { return !prediction::StoppedBlock() && lunar::HoldingSword() && (!requireRight || lunar::PhysicalUseDown()); }
    bool HasBlockTarget() const { return blockTarget; }
    bool Busy() const { return lunar::HypixelPlayerBusy(); }
    bool IsPlayerBlocking() const { return lunar::HypixelPlayerBlocking(); }
    // NoSlow and Disabler are separate Leader modules, disabled by default.
    // Nova has neither: keep their inactive code path rather than silently
    // enabling movement modification or an inventory swap module.
    bool NoSlowEnabled() const { return false; }
    int CurrentSlot() const { int slot = 0; lunar::SelectedSlot(slot); return slot; }
    int AltSlot(int handle) const { return handle % 8 + 1; }
    int SwapSlot() const {
        const int current = CurrentSlot();
        const int pick = std::clamp(static_cast<int>(rotation::Random() * 8), 0, 7);
        return pick >= current ? pick + 1 : pick;
    }
    void Slot(int slot) { lunar::SendHeldSlot(slot); }
    void Start() { lunar::StartSwordBlockDirect(); }
    void Stop() { lunar::StopSwordBlockDirect(); }
    void Blink(bool enabled) {
        if (enabled) lunar::BeginHypixelBlockLag();
        else lunar::EndHypixelBlockLag();
        autoblock::SynchronizePacketDelays();
    }
    bool AttackReady() const {
        return !prediction::StoppedBlock() && !prediction::CancelsAura() &&
            (automaticAttack || clicked) && lunar::TimerAtLeastNormal() &&
            antikb::AttackQueue() == 0 && velocity::AttackQueue() == 0;
    }
    void ClearExtraAttack() { prediction::ClearExtraAttack(); }
    bool PredictionAttack(const HypixelBlockSettings& s, int& tick) {
        int mode = 0;
        if (!prediction::ConsumeExtraAttack(mode)) return true;
        if (mode == 2) tick = s.mode == 2 ? s.attackTick : 0;
        if (mode == 1) tick = s.mode == 2 ? s.attackTick : s.mode != 3 ? 2 :
            s.lagMode == 0 ? 1 : s.lagMode == 1 ? 2 : s.lagMode == 2 ? 3 : 4;
        return false;
    }
    int NormalAttackDelay() const { return normalDelay; }
    void Swing() { consumed = true; lunar::SwingHand(); }
    double DistanceSq() const {
        auto self = java::PlayerEntity();
        rotation::Vec3 eye;
        if (!self || !geometryValid || !self.EyePosition(1, eye))
            return std::numeric_limits<double>::max();
        const auto& box = targetBox;
        const double dx = eye.x - std::clamp(eye.x, box.minX, box.maxX);
        const double dy = eye.y - std::clamp(eye.y, box.minY, box.maxY);
        const double dz = eye.z - std::clamp(eye.z, box.minZ, box.maxZ);
        return dx*dx + dy*dy + dz*dz;
    }
    bool InSwingRange() const { return DistanceSq() <= 3.5 * 3.5; }
    bool Ray(double range, rotation::Vec3& hit) const {
        auto self = java::PlayerEntity();
        rotation::Vec3 eye, direction;
        if (!self || !geometryValid || !self.EyePosition(1, eye)) return false;
        const auto& box = targetBox;
        const auto rot = rotation::GetRotation();
        if (!lunar::DirectionFromRotation(rot.pitch, rot.yaw, direction)) return false;
        // calculateIntercept returns the exit face when starting inside.
        // Do not replace it with the eye: INTERACT_AT needs that exact point.
        const rotation::Vec3 end{eye.x + direction.x*range, eye.y + direction.y*range, eye.z + direction.z*range};
        bool intersects = false;
        return lunar::BoxClip(box, eye, end, intersects, hit) && intersects;
    }
    bool Attack() {
        rotation::Vec3 hit;
        if (!Ray(3, hit) || (teams::IsTeammate(target) || antibot::IsBot(target))) return false;
        return lunar::AttackEntity(target);
    }
    void InteractBlock() {
        rotation::Vec3 hit;
        if (Ray(8, hit)) lunar::InteractSwordBlock(target, hit, &targetPosition);
    }
};
}

void AutoBlock::PrepareHypixelInput() {
    if (!enabled_ || settings_.mode != AutoBlockMode::Hypixel) return;
    hypixelManualAttack_ |= lunar::PendingAttackClick();
    const bool canManage = lunar::HoldingSword() &&
        (!settings_.requireRight || lunar::PhysicalUseDown());
    if (hypixel_.isBlocking || (canManage && (killaura::HasTargets() || FindTarget()))) {
        lunar::ClearCombatClicks();
        // Native cancel-use hook preserves use state; follow the actual use binding.
        lunar::SetUseKeyDown(lunar::PhysicalUseDown());
    }
}

void AutoBlock::TickHypixel(bool auraEnabled, bool attackRequested) {
    if (!enabled_ || settings_.mode != AutoBlockMode::Hypixel) return;
    auto self = java::PlayerEntity();
    if (!self || lunar::EntityDead(self.raw()) || java::ScreenOpen()) { Reset(true); return; }
    const auto sequence = lunar::GrimResetSequence();
    if (hypixelResetSequence_ != sequence) { Reset(true); hypixelResetSequence_ = sequence; return; }
    HypixelIO io;
    hypixelManualAttack_ |= attackRequested;
    io.automaticAttack = auraEnabled;
    io.requireRight = settings_.requireRight;
    if (auraEnabled) {
        io.attackAllowed = killaura::ReadHypixelTarget(io.target, settings_.hypixel.range, io.blockTarget, io.normalDelay);
    } else {
        // The independent AutoBlock uses the same machine, but attacks only on
        // real left-click input. It must not become a second KillAura.
        io.target = FindTarget();
        io.blockTarget = io.attackAllowed = io.target.Valid();
        io.clicked = hypixelManualAttack_ || lunar::PendingAttackClick();
    }
    io.CaptureGeometry();
    hypixel_.Pre(settings_.hypixel, io);
    if (auraEnabled || io.consumed || !io.CanAttack()) hypixelManualAttack_ = false;
    blocking_ = lunar::HypixelPlayerBlocking();
    lunar::SetHypixelBlockManaged(hypixel_.isBlocking);
    Render();
    autoblock::SetCoverageActive(hypixel_.isBlocking);
}

void AutoBlock::PostHypixel() {
    if (!enabled_ || settings_.mode != AutoBlockMode::Hypixel || java::ScreenOpen()) return;
    HypixelIO io;
    hypixel_.Post(io);
}

bool AutoBlock::VelocityCanReduce(int phase) const {
    return !enabled_ || settings_.mode != AutoBlockMode::Hypixel ||
        hypixel_.VelocityCanReduce(settings_.hypixel, phase, hypixel_.blockTick);
}
bool AutoBlock::VelocityShouldBlock() const {
    return enabled_ && settings_.mode == AutoBlockMode::Hypixel && hypixel_.isBlocking &&
        lunar::HypixelPlayerBlocking() && !lunar::PredictionLiquid();
}

void AutoBlock::ResetHypixel() {
    HypixelIO io;
    hypixel_.Disable(settings_.hypixel, io);
    // Restore vanilla ownership on disable/GUI/world changes. The packet queue
    // must finish before vanilla is allowed to release the held item.
    lunar::SetHypixelBlockManaged(false);
    lunar::ResetHypixelPacketState();
    lunar::ClearCombatClicks();
    lunar::SetAutoBlockState(false, false);
    hypixel_ = {};
    hypixelManualAttack_ = false;
    hypixelResetSequence_ = lunar::GrimResetSequence();
}
}
