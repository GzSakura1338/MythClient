#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <format>
#include <memory>
#include <utility>
#include <vector>

#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/ModuleManager.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/combat/throwableaura/ThrowableVisuals.h"
#include "minecraft/modules/combat/throwableaura/Prediction.h"
#include "minecraft/modules/combat/autorod/AutoRod.h"
#include "minecraft/modules/combat/autoblock/AutoBlockRuntime.h"
#include "minecraft/modules/combat/killaura/KillAura.h"
#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/modules/misc/teams/Teams.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/modules/player/blockin/BlockIn.h"
#include "minecraft/modules/player/breaker/Breaker.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_itemuse.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_player.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/utils/rotation/MathConst.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "ui/clickgui/ClickGui.h"

namespace nova::module {
namespace {
namespace lunar = compat::lunar;

// Rise-derived selection/delay with automatic predicted aim before item use.
// Track continuously
// through the shared smooth rotation path; delay controls only item use.
class ThrowableAuraModule final : public Module {
public:
    ThrowableAuraModule() : Module("ThrowableAura", Category::Combat, "General", "General") {
        const auto decimal = [](double value) { return std::format("{:.1f}", value); };
        const auto integer = [](double value) { return std::format("{:.0f}", value); };
        rotationRange_ = &addNumber("Rotation Range", 1, 15, .1, 8, {}, decimal);
        rotationSpeed_ = &addNumber("Rotation speed", 1, 180, 1, 60, {}, integer);
        range_ = &addNumber("Range", 1, 12, .1, 6, {}, decimal);
        minimumRange_ = &addNumber("Minimum Range", 1, 10, .1, 3, {}, decimal);
        throwDelay_ = &addNumber("Throw Delay", 0, 100, 1, 5, {}, integer);
        fov_ = &addNumber("FOV", 30, 360, 1, 180, {}, integer);
        players_ = &addBool("Players", true);
        mobs_ = &addBool("Mobs", true);
        autoSwitch_ = &addBool("Auto Switch", true);
        prediction_ = &addBool("Prediction", true);
        moduleCheck_ = &addBool("Module Check", true);
        antiBot_ = &addBool("AntiBot", true);
        snowballs_ = &addBool("Snowballs", true);
        eggs_ = &addBool("Eggs", true);
        throughWalls_ = &addBool("Through Walls", true);
        esp_ = &addBool("ESP", true);
        trajectory_ = &addBool("Trajectory", true);
        enableOverlay();
        auto& bus = event::EventBus::Get();
        tick_ = bus.Subscribe<event::ClientRotationEvent>(
            [this](auto&) { OnPreUpdate(); }, event::Priority::Lowest);
        interact_ = bus.Subscribe<event::PrePlayerInteractEvent>(
            [this](auto&) { OnInteract(); }, event::Priority::Lowest);
    }

private:
    void onEnable() override {
        resetRequested_.store(true);
    }

    void onDisable() override {
        // Rotation state and delayed slot restoration belong to the game thread.
        resetRequested_.store(true);
    }

    void onRender() override { visuals_.Render(esp_->get(),trajectory_->get()); }

    bool CombatOwnsItemUse() const {
        return (killaura::IsEnabled() && killaura::HasTargets()) || autoblock::OwnsItemUse() ||
            java::PlayerUsingItem() || lunar::BlockLagging() || lunar::HypixelBlockLagging() ||
            lunar::SlinkyBlockLagging();
    }

    bool CanThrow() const {
        // AutoRod's accepted hold and pending slot restore take precedence.
        // Its enabled toggle alone must not disable this independent module.
        if (autorod::OwnsItemUse()) return false;
        if (scaffold::IsEnabled() || automlg::IsWorking() || autocharge::IsWorking() || blockin::IsPlacing() || breaker::OwnsMining() ||
            java::ScreenOpen() || clickgui::IsOpen()) return false;
        // A temporary target-id change does not suspend KillAura's rotations,
        // queued attacks or the shared AutoBlock engine. Yield the entire use
        // transaction before changing either slot or rotation, even with the
        // optional Module Check disabled.
        if (CombatOwnsItemUse()) return false;
        if (!moduleCheck_->get()) return true;
        // Backtrack and AimBacktrack are different modules in Rise. Do not
        // silently substitute Nova's packet Backtrack for a missing AimBacktrack.
        const auto* module = ModuleManager::Get().Find("AimBacktrack");
        return !module || !module->isEnabled();
    }

    static double DotNormalized(const rotation::Vec3& a, const rotation::Vec3& b) {
        // Vec3.normalize in 1.8 returns zero below 1e-4.
        const double al = std::sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
        const double bl = std::sqrt(b.x*b.x + b.y*b.y + b.z*b.z);
        if (al < 1e-4 || bl < 1e-4) return 0;
        return (a.x*b.x + a.y*b.y + a.z*b.z) / (al*bl);
    }

    bool AimPosition(const java::EntityHandle& entity, const java::EntityHandle& self,
                     bool predict, rotation::Vec3& out) const {
        double height = 0;
        if (!entity.Position(out) || !entity.EyeHeight(height)) return false;
        out.y += height * .5;
        if (predict) {
            double distance = 0;
            rotation::Vec3 velocity;
            if (!self.DistanceTo(entity, distance) || !lunar::EntityMotion(entity.raw(), velocity))
                return false;
            out = out.Add(velocity.x * distance / 1.5, velocity.y * distance / 1.5,
                          velocity.z * distance / 1.5);
        }
        return true;
    }

    bool InFov(const java::EntityHandle& entity, const java::EntityHandle& self) const {
        if (fov_->get() >= 360) return true;
        rotation::Vec3 point, eye, look;
        if (!AimPosition(entity, self, false, point) ||
            !self.EyePosition(1.f, eye) || !lunar::ThrowableLook(look)) return false;
        const double dot = DotNormalized(look, point.Subtract(eye));
        return std::acos(std::clamp(dot, -1.0, 1.0)) * rotation::kToDegrees <= fov_->get() / 2;
    }

    java::EntityHandle GetTarget(const java::EntityHandle& self) const {
        std::vector<java::EntityHandle> entities;
        if (!lunar::WorldLoadedEntities(entities)) return {};
        java::EntityHandle best;
        double bestDistance = 0;
        for (auto& entity : entities) {
            unsigned types = 0;
            double distance = 0, distanceSq = 0;
            if (!java::EntityTypes(entity, types) || !(types & java::kTypeLiving) ||
                entity.IsSame(self) || !entity.IsAlive() || !self.DistanceTo(entity, distance) ||
                distance > rotationRange_->get() || distance <= minimumRange_->get() ||
                !InFov(entity, self)) continue;
            if (!throughWalls_->get() && !lunar::ThrowableCanSee(entity.raw())) continue;
            if (antiBot_->get() && antibot::IsBot(entity)) continue;
            if (types & java::kTypePlayer) {
                if (!players_->get() || teams::IsTeammate(self, entity))
                    continue;
            } else if (!lunar::ThrowableMob(entity.raw()) || !mobs_->get()) continue;
            if (!self.DistanceSqTo(entity, distanceSq)) continue;
            if (!best || distanceSq < bestDistance) {
                bestDistance = distanceSq;
                best = std::move(entity);
            }
        }
        return best;
    }

    static java::KnownItem SlotItem(int slot) {
        JniAttach attach;
        jobject stack = nullptr;
        java::ItemFacts facts;
        if (!attach || !java::InventorySlot(slot, stack)) return java::KnownItem::Nothing;
        const bool read = stack && java::ReadItemFacts(stack, facts);
        if (stack) attach.env->DeleteLocalRef(stack);
        return read && !facts.empty ? facts.item : java::KnownItem::Nothing;
    }

    int FindThrowableSlot() const {
        for (auto item : {java::KnownItem::Snowball, java::KnownItem::Egg}) {
            if (item == java::KnownItem::Snowball ? !snowballs_->get() : !eggs_->get()) continue;
            for (int i = 0; i < 9; ++i)
                if (SlotItem(i) == item) return i;
        }
        return -1;
    }

    bool IsThrowable(java::KnownItem item) const {
        return (snowballs_->get() && item == java::KnownItem::Snowball) ||
               (eggs_->get() && item == java::KnownItem::Egg);
    }

    bool ReadMotion(const java::EntityHandle& entity, throwableaura::TargetMotion& motion) const {
        java::Box box;
        int id=-1;
        if (!entity.BoundingBox(box) || !entity.OnGround(motion.grounded) || !java::EntityId(entity,id)) return false;
        motion.bounds={{box.minX,box.minY,box.minZ},{box.maxX,box.maxY,box.maxZ}};
        if (history_.valid && history_.id==id) motion.velocity=history_.velocity;
        else {
            rotation::Vec3 now,before;
            if (!entity.PositionPair(now,before)) return false;
            motion.velocity=now.Subtract(before);
        }
        return throwableaura::Finite(motion.velocity);
    }

    void SampleMotion(const java::EntityHandle& self) {
        auto next=GetTarget(self);
        auto world=java::LevelEntity();
        if (!next || !world) { history_.Reset(); return; }
        if (!sampleWorld_ || !sampleWorld_.IsSame(world)) {
            history_.Reset(); sampleWorld_=std::move(world);
        }
        int id=-1,tick=0; bool ground=false;
        rotation::Vec3 now,before;
        if (!java::EntityId(next,id) || !self.TickCount(tick) || !next.PositionPair(now,before) || !next.OnGround(ground)) {
            history_.Reset(); return;
        }
        history_.Sample(id,tick,now,now.Subtract(before),ground);
    }

    bool ThrowRotation(const java::EntityHandle& target, const java::EntityHandle& self,
                       float& aimYaw, float& aimPitch) const {
        rotation::Vec3 point, eye;
        if (prediction_->get()) {
            throwableaura::TargetMotion motion;
            if (!self.EyePosition(1.f,eye) || !ReadMotion(target,motion)) return false;
            const auto solution=throwableaura::SolveIntercept(eye,motion);
            aimYaw=solution.aim.yaw; aimPitch=solution.aim.pitch;
            return solution.valid;
        }
        if (!self.EyePosition(1.f, eye) || !AimPosition(target, self, prediction_->get(), point)) return false;
        const auto delta = point.Subtract(eye);
        aimYaw = static_cast<float>(std::atan2(delta.z, delta.x) * rotation::kToDegrees - 90);
        aimPitch = static_cast<float>(-std::atan2(delta.y, std::hypot(delta.x, delta.z)) * rotation::kToDegrees);
        return std::isfinite(aimYaw) && std::isfinite(aimPitch);
    }

    bool OwnsRotation() const {
        const auto& state = rotation::State();
        return ownsRotation_ && !state.direct && !state.linear && state.hasRotations &&
            state.priority == rotation::RotationPriority::Medium &&
            state.rotations.yaw == applied_.yaw && state.rotations.pitch == applied_.pitch;
    }

    bool AimHitsTarget(const java::EntityHandle& self, float yaw, float pitch) const {
        java::Box box;
        rotation::Vec3 eye, position, point;
        double height = 0;
        if (!target_ || !target_.BoundingBox(box) || !target_.Position(position) ||
            !target_.EyeHeight(height) || !self.EyePosition(1.f, eye) ||
            !AimPosition(target_, self, prediction_->get(), point)) return false;
        // Translate the whole collision box by the same prediction as the aim
        // point. A jump changes the eye height, not the size of the target.
        const auto offset = point.Subtract(position.Add(0, height * .5, 0));
        constexpr double radius = .3; // Vanilla EntityThrowable collision border.
        const throwableaura::Bounds bounds{
            {box.minX + offset.x - radius, box.minY + offset.y - radius, box.minZ + offset.z - radius},
            {box.maxX + offset.x + radius, box.maxY + offset.y + radius, box.maxZ + offset.z + radius}};
        const double y = yaw * rotation::kToRadians, p = pitch * rotation::kToRadians;
        const auto start = eye.Add(-std::cos(y) * .16, -.1, -std::sin(y) * .16);
        const double length = eye.DistanceTo(point) + 2;
        const auto end = start.Add(-std::sin(y) * std::cos(p) * length,
            -std::sin(p) * length, std::cos(y) * std::cos(p) * length);
        double fraction = 0;
        return throwableaura::Intersect(bounds, start, end, fraction);
    }

    bool PredictedThrowReady(const java::EntityHandle& self,float yaw,float pitch) const {
        throwableaura::TargetMotion motion;
        rotation::Vec3 eye;
        int id=-1;
        if (!history_.Ready() || !java::EntityId(target_,id) || history_.id!=id ||
            !ReadMotion(target_,motion) || !self.EyePosition(1.f,eye)) return false;
        const auto solution=throwableaura::SolveIntercept(eye,motion);
        if (!solution.valid || delayTicks_<=throwableaura::AdaptiveDelay(
            static_cast<int>(throwDelay_->get()),solution.ticks,history_.uncertainty)) return false;
        if (!throwableaura::RobustHit(eye,{yaw,pitch},motion,history_.uncertainty) ||
            !throwableaura::RobustHit(eye,applied_,motion,history_.uncertainty)) return false;
        return WorldPathHits(self,eye,{yaw,pitch},motion);
    }

    bool WorldPathHits(const java::EntityHandle& self,rotation::Vec3 eye,
                       rotation::Rotation aim,const throwableaura::TargetMotion& motion) const {
        // A wall or failed world read postpones use. Visibility alone does not
        // ensure that the lower, offset projectile launch clears an obstacle.
        return throwableaura::PredictedHit(eye,aim,motion,
            [&](rotation::Vec3 from,rotation::Vec3 to,bool& blocked,double& fraction) {
                java::BlockHitResultInfo hit;
                if (!lunar::WorldClip(from,to,self,hit)) return false;
                blocked=hit.kind==java::HitKind::Block;
                if (blocked && !throwableaura::Finite(hit.location)) return false;
                const double length=from.DistanceTo(to);
                fraction=blocked && length>1e-9?std::clamp(from.DistanceTo(hit.location)/length,0.,1.):1;
                return true;
            });
    }

    void ReleaseRotation() {
        // As in KillAura, stop submitting requests and let the manager return
        // toward the camera naturally. Never hard-reset another owner's state.
        ownsRotation_ = false;
    }

    void CancelAim() {
        target_.Reset();
        reportedAimReady_ = false;
        ReleaseRotation();
    }

    void UpdateVisuals(const java::EntityHandle& self) {
        if (!esp_->get() && !trajectory_->get()) { visuals_.Clear(); return; }
        int selected = -1;
        const bool available = autoSwitch_->get() ? FindThrowableSlot() >= 0 :
            java::SelectedSlot(selected) && IsThrowable(SlotItem(selected));
        if (!available) { visuals_.Clear(); return; }
        // Preview selection never changes the throw cooldown, slots or rotations.
        auto target = GetTarget(self);
        double distance = 0;
        float yaw = 0, pitch = 0;
        if (!target || !self.DistanceTo(target,distance) || distance > range_->get() ||
            !ThrowRotation(target,self,yaw,pitch)) { visuals_.Clear(); return; }
        visuals_.Update(self,target,yaw,pitch,trajectory_->get());
    }

    bool SameWorld() const {
        auto self = java::PlayerEntity(), world = java::LevelEntity();
        return self && world && player_ && world_ && self.IsSame(player_) && world.IsSame(world_);
    }

    bool RestoreSlot() {
        if (previousSlot_ < 0) return true;
        int current = -1;
        if (!SameWorld()) { previousSlot_ = -1; return true; }
        if (!java::SelectedSlot(current)) return false;
        // A manual scroll or another item owner supersedes our saved slot.
        if (current != throwableSlot_ && current != previousSlot_) { previousSlot_ = -1; return true; }
        if (lunar::BlockInputMovementSequence() <= useMovement_ || !lunar::HeldSlotChangeReady() ||
            lunar::BlockLagging() || lunar::HypixelBlockLagging() || lunar::SlinkyBlockLagging()) return false;
        if (!java::SetSelectedSlot(previousSlot_)) return false;
        // Synchronize before this tick's other actions, never after the C08.
        if (!java::SyncSelectedItem()) return false;
        previousSlot_ = -1;
        return true;
    }

    void OnPreUpdate() {
        if (resetRequested_.exchange(false)) {
            CancelAim(); delayTicks_ = 0; combatYield_ = false;
            history_.Reset(); sampleWorld_.Reset();
            visuals_.Clear();
        }
        if (player_ && !SameWorld()) {
            previousSlot_ = -1; CancelAim(); player_.Reset(); world_.Reset();
        }
        if (!RestoreSlot()) { visuals_.Clear(); return; }
        if (!isEnabled() || !compat::IsLegacyLunar()) { CancelAim(); visuals_.Clear(); return; }
        auto self = java::PlayerEntity();
        if (!self || !java::LevelEntity() || !self.IsAlive()) {
            history_.Reset(); sampleWorld_.Reset();
            CancelAim(); visuals_.Clear(); combatYield_ = false; return;
        }
        SampleMotion(self);
        UpdateVisuals(self);
        if (!CanThrow()) { combatYield_ |= CombatOwnsItemUse(); CancelAim(); return; }
        // Let vanilla finish one clean movement/use-release interval after a
        // combat owner drains its queue. Do not throw into its final release.
        if (std::exchange(combatYield_, false)) return;
        // Yield to other rotation users, including Aim and building modules.
        // Active also covers the automatic return after another module stops.
        // Only a request already smoothed this tick reserves that owner's turn.
        const auto& rotationState = rotation::State();
        if (rotation::IsActive() && !OwnsRotation() &&
            (rotation::IsSmoothed() || rotationState.direct || rotationState.linear)) { CancelAim(); return; }
        auto next = GetTarget(self);
        if (!next) { CancelAim(); return; }
        throwableSlot_ = FindThrowableSlot();
        if (throwableSlot_ < 0) { CancelAim(); return; }
        int current = -1;
        if (!autoSwitch_->get() && (!java::SelectedSlot(current) || !IsThrowable(SlotItem(current)))) {
            CancelAim(); return;
        }
        // Item use occurs before this tick's walking update, so validate the
        // LAST published smooth step, not the new (as yet unsent) request.
        reportedAimReady_ = target_ && next.IsSame(target_) && OwnsRotation() &&
            lunar::BlockInputMovementSequence() > aimMovement_;
        previousApplied_ = applied_;
        target_ = std::move(next);
        if (!ThrowRotation(target_, self, desired_.yaw, desired_.pitch)) { CancelAim(); return; }
        if (!rotation::State().hasRotations || !rotation::State().hasLastRotations)
            rotation::UpdateGlobalYaw();
        rotation::SetRotations(desired_, rotationSpeed_->get());
        applied_ = rotation::GetRotation();
        ownsRotation_ = true;
        rotation::SetMoveInputFix(true);
        player_ = java::PlayerEntity(); world_ = java::LevelEntity();
        aimMovement_ = lunar::BlockInputMovementSequence();
        delayTicks_=std::min(delayTicks_+1,200);
    }

    void OnInteract() {
        if (!target_) return;
        if (!isEnabled() || resetRequested_.load() || !SameWorld() || !CanThrow() || !OwnsRotation()) {
            CancelAim(); return;
        }
        // Only vanilla's walking update publishes this aim. Never manufacture
        // an extra C05 tick, or throw from a Motion/PostPlayerTick callback.
        if (!reportedAimReady_ || previousSlot_ >= 0 ||
            delayTicks_ <= static_cast<int>(throwDelay_->get())) return;
        float yaw = 0, pitch = 0;
        if (!lunar::PlayerReportedRotation(yaw, pitch) ||
            !std::isfinite(yaw) || !std::isfinite(pitch) ||
            std::abs(std::remainder(yaw - previousApplied_.yaw, 360.f)) > .01f ||
            std::abs(pitch - previousApplied_.pitch) > .01f) return;
        const auto self = java::PlayerEntity();
        auto liveTarget = GetTarget(self);
        double distance = 0;
        if (!liveTarget || !liveTarget.IsSame(target_) || !self.DistanceTo(target_, distance)) { CancelAim(); return; }
        if (distance > range_->get() || distance <= minimumRange_->get()) return;
        // The already-published angle must aim through the live predicted body,
        // not within a fixed two-degree cone around its center. The latter
        // rejects usable throws throughout the rising/falling part of a jump.
        if (prediction_->get()) {
            if (!PredictedThrowReady(self,yaw,pitch)) return;
        } else {
            if (!AimHitsTarget(self, yaw, pitch) || !AimHitsTarget(self, applied_.yaw, applied_.pitch)) return;
            if (!throughWalls_->get()) {
                java::Box box;
                rotation::Vec3 eye;
                if (!target_.BoundingBox(box) || !self.EyePosition(1.f,eye)) return;
                const throwableaura::TargetMotion stationary{
                    {{box.minX,box.minY,box.minZ},{box.maxX,box.maxY,box.maxZ}},{},true};
                if (!WorldPathHits(self,eye,{yaw,pitch},stationary)) return;
            }
        }
        if (!lunar::HeldSlotChangeReady() || lunar::PendingAttackClick() ||
            lunar::PhysicalMouseDown(0) || lunar::PhysicalUseDown()) { CancelAim(); return; }
        const int slot = FindThrowableSlot();
        if (slot < 0) { CancelAim(); return; }
        int current = 0;
        if (!java::SelectedSlot(current)) return;
        if (autoSwitch_->get() && current != slot) {
            if (!java::SetSelectedSlot(slot)) { CancelAim(); return; }
            previousSlot_ = current;
            throwableSlot_ = slot;
        }
        useMovement_ = lunar::BlockInputMovementSequence();
        int selected = 0;
        if (java::SelectedSlot(selected) && IsThrowable(SlotItem(selected))) lunar::ThrowHeldItem();
        // Keep the aim and selected item through vanilla movement. Restore at
        // the next ClientRotation phase, after the use interval has ended.
        delayTicks_ = 0;
        reportedAimReady_ = false;
    }

    Properties::NumberProp *rotationRange_, *rotationSpeed_, *range_, *minimumRange_, *throwDelay_, *fov_;
    Properties::BoolProperty *players_, *mobs_, *autoSwitch_, *prediction_,
        *moduleCheck_, *antiBot_, *snowballs_, *eggs_, *throughWalls_;
    Properties::BoolProperty *esp_, *trajectory_;
    throwableaura::Visuals visuals_;
    event::Subscription tick_, interact_;
    int delayTicks_ = 0, previousSlot_ = -1, throwableSlot_ = -1;
    bool combatYield_ = false, ownsRotation_ = false, reportedAimReady_ = false;
    std::atomic<bool> resetRequested_{false};
    std::uint64_t aimMovement_ = 0, useMovement_ = 0;
    rotation::Rotation applied_, previousApplied_, desired_;
    throwableaura::MotionHistory history_;
    java::EntityHandle target_, player_, world_, sampleWorld_;
};
}

std::unique_ptr<Module> MakeThrowableAuraModule() { return std::make_unique<ThrowableAuraModule>(); }
}
