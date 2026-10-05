#include "minecraft/modules/player/breaker/Breaker.h"
#include "minecraft/modules/misc/antibot/AntiBot.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <format>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/combat/killaura/KillAura.h"
#include "minecraft/modules/combat/legitcrystalaura/LegitCrystalAura.h"
#include "minecraft/modules/combat/shared/ExplosionCoordinator.h"
#include "minecraft/modules/combat/killaura/CriticalSprint.h"
#include "minecraft/modules/combat/autoblock/AutoBlockModuleBase.h"
#include "minecraft/modules/combat/autoblock/AutoBlockRuntime.h"
#include "minecraft/modules/misc/teams/Teams.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "ui/clickgui/ClickGui.h"
#include "minecraft/modules/combat/velocity/Velocity.h"
#include "minecraft/modules/combat/antikb/AntiKB.h"
#include "minecraft/modules/combat/antikb/prediction/PredictionRuntime.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/combat/automace/AutoMace.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_critical.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_player.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/utils/rotation/AimUtils.h"
#include "minecraft/utils/rotation/Mth.h"
#include "minecraft/utils/rotation/Random.h"
#include "minecraft/utils/rotation/RayCastUtil.h"
#include "minecraft/utils/rotation/Rotation.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/host/render/WorldProjection.h"
#include "minecraft/host/render/WorldGeometry.h"
#include "imgui.h"

namespace nova::module {
namespace {

namespace event = nova::event;

constexpr double kSearchMargin = 2.0;
constexpr double kRaycastRange = 3.0;
constexpr double kCpsDeviation = 5.0;

// Velocity asks whether KillAura is currently aiming before it aims on the
// client's behalf; two writers would fight over the same rotation. Mirrored
// rather than exposed as a member because the question arrives from outside.
std::atomic<bool> g_hasTargets{false};

// Who it is aiming at, for the target hud. Zero whenever `g_hasTargets` is false,
// so a reader that sees a live flag always has an id to resolve.
std::atomic<int> g_targetId{0};

// The whole list, when the aura is switching. Behind an SRW lock because the reader
// is the hud, which asks once a tick from the game thread but may be drawing at the
// time. The atomic above is the single answer; this is the plural one.
SRWLOCK g_switchLock = SRWLOCK_INIT;
std::vector<int> g_switchTargets;

void ClearSwitchTargets() {
    AcquireSRWLockExclusive(&g_switchLock);
    g_switchTargets.clear();
    ReleaseSRWLockExclusive(&g_switchLock);
}

void ClearTargets() {
    g_hasTargets.store(false, std::memory_order_release);
    g_targetId.store(0, std::memory_order_release);
    ClearSwitchTargets();
}

struct Candidate {
    java::EntityHandle entity;
    double sortKey = 0.0;
};

class KillAuraModule;
KillAuraModule* auraInstance = nullptr;

[[nodiscard]] long long NowMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

[[nodiscard]] std::string WholeNumber(double value) {
    return std::format("{:.0f}", value);
}

[[nodiscard]] std::string Hundredths(double value) {
    return std::format("{:.2f}", value);
}

[[nodiscard]] double AttackDelay(double cps) {
    return 1000.0 / std::max(0.01, rotation::Gaussian(cps, kCpsDeviation));
}

[[nodiscard]] java::Box SearchArea(const rotation::Vec3& position, double range) {
    const double extent = range + kSearchMargin;
    java::Box box;
    box.minX = position.x - extent;
    box.minY = position.y - extent;
    box.minZ = position.z - extent;
    box.maxX = position.x + extent;
    box.maxY = position.y + extent;
    box.maxZ = position.z + extent;
    return box;
}

class KillAuraModule final : public AutoBlockModuleBase {
public:
    KillAuraModule() : AutoBlockModuleBase("KillAura") {
        enableOverlay();
        auraInstance = this;
        targetPlayer_ = &addBool("Player", true);
        targetInvisibles_ = &addBool("Invisibles");
        targetAnimals_ = &addBool("Animals");
        targetMobs_ = &addBool("Mobs");
        targetVillager_ = &addBool("Villager");
        hitSel_ = &addBool("Hit select", true);
        attackInvisible_ = &addBool("Attack Invisible", true);
        cpsMode_ = &addMode("CPS Mode", 0, {"Default", "1.9+"});
        auto criticalSound = subBool("Critical Sound", false);
        criticalSound_ = static_cast<Properties::BoolProperty*>(criticalSound.getProp());
        auto interruptSprint = subBool("Interrupt Sprint", false);
        interruptSprint_ = static_cast<Properties::BoolProperty*>(interruptSprint.getProp());
        waitCritical_ = &addBool("Wait Critical", false, {interruptSprint, criticalSound},
                                [this] { return cpsMode_->get() == 1; });
        cps_ = &addNumber("CPS", 1, 20, 1, 12, {}, WholeNumber,
                          [this] { return cpsMode_->get() == 0; });
        aimRange_ = &addNumber("Aim range", 3, 6, 0.01, 4, {}, Hundredths);
        fov_ = &addNumber("FOV", 0, 360, 1, 180, {}, WholeNumber);
        auto noise = subNumber("Noise", 0, 5, 0.1, 0,
            [](double value) { return std::format("{:.1f} deg", value); });
        rotationNoise_ = static_cast<Properties::NumberProp*>(noise.getProp());
        rotationSpeed_ = &addNumber("Rotation speed", 0, 180, 1, 180, {noise}, WholeNumber);
        aimPointEsp_ = &addBool("Aim Point ESP", true);
        mode_ = &addMode("Mode", 0, {"Single", "Switch"});
        switchDelay_ = &addNumber("Switch delay", 0, 500, 1, 100, {}, WholeNumber,
                                  [this] { return mode_->get() == 1; });
        priority_ = &addMode("Priority", 2, {"Health", "FoV", "Range", "None"});

        autoBlock_ = &addBool("AutoBlock", false, BuildBlockSettings());

        // Keep the existing config keys while grouping target filters in the UI.
        configRows_ = settings();
        std::vector<Properties::PropDesc> targetRows, displayRows;
        for (const auto& row : configRows_) {
            const auto* property = row.getProp();
            if (property == targetPlayer_ || property == targetInvisibles_ ||
                property == targetAnimals_ || property == targetMobs_ || property == targetVillager_)
                targetRows.push_back(row);
        }
        for (const auto& row : configRows_) {
            const auto* property = row.getProp();
            if (property == targetPlayer_)
                displayRows.emplace_back("Target", nullptr, std::move(targetRows));
            if (property == targetPlayer_ || property == targetInvisibles_ ||
                property == targetAnimals_ || property == targetMobs_ || property == targetVillager_)
                continue;
            displayRows.push_back(row);
        }
        replaceSettings(std::move(displayRows));

        event::EventBus& bus = event::EventBus::Get();
        criticalTick_ = bus.Subscribe<event::GameTickEvent>(
            [this](event::GameTickEvent& e) { OnCriticalSprintTick(e); }, event::Priority::High);
        criticalInput_ = bus.Subscribe<event::MoveInputEvent>(
            [this](event::MoveInputEvent& e) { OnCriticalSprintInput(e); });
        gameTick_ = bus.Subscribe<event::GameTickEvent>(
            [this](event::GameTickEvent& e) { OnGameTick(e); });
        playerTick_ = bus.Subscribe<event::PlayerTickEvent>(
            [this](event::PlayerTickEvent&) { OnPlayerTick(); });
        interact_ = bus.Subscribe<event::PrePlayerInteractEvent>(
            [this](event::PrePlayerInteractEvent&) { OnPrePlayerInteract(); });
    }

    const std::vector<Properties::PropDesc>& configSettings() const override { return configRows_; }

    bool ReadHypixelTarget(java::EntityHandle& target, double blockRange, bool& hasBlockTarget, int& normalDelay) {
        hasBlockTarget = false;
        if (!isEnabled()) return false;
        normalDelay = static_cast<int>(1000.0 / std::max(1.0, cps_->get()));
        auto self = java::PlayerEntity();
        if (!self || targetNumber_ >= targets_.size()) return false;
        target = java::EntityById(g_targetId.load());
        if (!target || !IsTarget(target, self)) return false;
        hasBlockTarget = HasBlockTarget(blockRange);
        return !compat::lunar::GameModeDestroying() ||
            (breaker::OwnsMining() && !breaker::BlocksAura());
    }

    bool HasBlockTarget(double blockRange) const {
        auto self = java::PlayerEntity();
        rotation::Vec3 eye;
        if (!self || !self.EyePosition(1, eye)) return false;
        std::vector<java::EntityHandle> entities;
        if (!compat::lunar::WorldLoadedEntities(entities)) return false;
        for (const auto& entity : entities) {
            java::Box box;
            double border = 0;
            if (!IsTarget(entity, self) || !entity.BoundingBox(box) || !entity.PickRadius(border)) continue;
            const double dx = eye.x - std::clamp(eye.x, box.minX-border, box.maxX+border);
            const double dy = eye.y - std::clamp(eye.y, box.minY-border, box.maxY+border);
            const double dz = eye.z - std::clamp(eye.z, box.minZ-border, box.maxZ+border);
            if (dx*dx + dy*dy + dz*dz <= blockRange*blockRange) return true;
        }
        return false;
    }

    bool HasBlockTarget() const { return HasBlockTarget(ReadBlockSettings().range); }

    bool CriticalSoundEnabled() const {
        return isEnabled() && UsesAttackCooldown() && waitCritical_->get() && criticalSound_->get();
    }

    bool ReadHypixelGeometry(java::Box& box, rotation::Vec3& position) const {
        if (!hypixelGeometryValid_ || hypixelGeometryId_ != g_targetId.load()) return false;
        box = hypixelBox_; position = hypixelPosition_;
        return true;
    }

private:
    bool CriticalSprintEnabled() const {
        return isEnabled() && !crystal::IsWorking() && !explosive::autoCrystalWorking && !explosive::anchorWorking && !explosive::feetWorking && UsesAttackCooldown() && waitCritical_->get() && interruptSprint_->get() &&
            !scaffold::IsEnabled() && !automace::IsWorking() && !automlg::IsWorking() && !autocharge::IsWorking() && !breaker::BlocksAura() && !clickgui::IsOpen() && !java::ScreenOpen();
    }

    void OnCriticalSprintTick(event::GameTickEvent& e) {
        if (e.type != event::EventType::Pre) return;
        criticalSprint_.Reset();
        if (!CriticalSprintEnabled() || !g_hasTargets.load(std::memory_order_acquire) ||
            velocity::AttackQueue() > 0 || antikb::AttackQueue() > 0) return;
        java::CriticalAttackState state;
        if (!java::ReadCriticalAttackState(state) || state.ground || state.water || state.web) return;
        criticalSprint_.Tick(state.fallDistance,true,0);
    }

    void OnCriticalSprintInput(event::MoveInputEvent& e) {
        if (!CriticalSprintEnabled() || !g_hasTargets.load(std::memory_order_acquire) ||
            velocity::AttackQueue() > 0 || antikb::AttackQueue() > 0) {
            criticalSprint_.Reset();
            return;
        }
        // The input hook runs before vanilla aiStep checks sprint continuation.
        // Do not write key bindings or force a sprint state/packet here.
        criticalSprint_.Input(e.forward);
    }

    [[nodiscard]] bool UsesAttackCooldown() const {
        return cpsMode_->get() == 1 && !compat::IsLegacyLunar();
    }

    void onDisable() override {
        ClearAimPoint();
        noiseTracking_ = false;
        criticalSprint_.Reset();
        autoblock::ConfigureAura(false, ReadBlockSettings());
        targets_.clear();
        ClearTargets();
        attacks_ = 0;
    }

    void onUpdate() override {
        autoblock::ConfigureAura(autoBlock_->get(), ReadBlockSettings());
        // Modern timing is checked on the game thread immediately before the
        // attack. Never accumulate render-frame clicks while charging.
        if (UsesAttackCooldown()) {
            attacks_ = 0;
            return;
        }
        if (targets_.empty())
            return;
        // Hypixel / Lag New own attack cadence, including swings that miss.
        if (autoblock::AttackCycleActive()) { attacks_ = 0; return; }

        // While Velocity is holding a knockback it does its own attacking, and
        // it wants the queue to itself.
        if (velocity::AttackQueue() > 0 || antikb::AttackQueue() > 0)
            return;

        if (hitSel_->get() && HoveredIsPlayer())
            return;

        const long long now = NowMillis();
        if (now - lastAttackTime_ >= AttackDelay(cps_->get())) {
            ++attacks_;
            lastAttackTime_ = now;
        }
    }

    void OnGameTick(event::GameTickEvent& e) {
        if (e.type != event::EventType::Pre) return;
        ClearAimPoint();
        // Any tick that yields to another module or loses its target breaks the
        // noise sequence, so reacquisition cannot inherit an old aim offset.
        const bool continueNoise = std::exchange(noiseTracking_, false);
        hypixelGeometryValid_ = false;
        if (!isEnabled()) {
            ClearTargets();
            return;
        }

        // Starain stands KillAura down entirely while the scaffold is placing: the
        // two would otherwise fight over the same rotation.
        if (crystal::IsWorking() || explosive::autoCrystalWorking || explosive::anchorWorking || explosive::feetWorking || scaffold::IsEnabled() || automace::IsWorking() || automlg::IsWorking() || autocharge::IsWorking() || breaker::BlocksAura()) {
            ClearTargets();
            return;
        }

        java::EntityHandle self = java::PlayerEntity();
        if (!self.Valid()) {
            ClearTargets();
            return;
        }

        rotation::AimFrame frame = rotation::ReadAimFrame();
        if (!frame.valid) {
            ClearTargets();
            return;
        }

        float playerYaw = 0.0f;
        float playerPitch = 0.0f;
        java::PlayerRotation(playerYaw, playerPitch);

        const double range = autoblock::AttackCycleActive()
            ? std::max(aimRange_->get(), autoblock::HypixelRange()) : aimRange_->get();
        const float fovValue = static_cast<float>(fov_->get());
        const bool attackInvisible = attackInvisible_->get();

        if (!CollectTargets(self, frame.eye, playerYaw, range, fovValue, attackInvisible)) {
            ClearTargets();
            return;
        }

        SortTargets(self, frame, playerYaw, range);

        // A knockback in flight outranks the configured priority: whoever sits
        // closest to the direction it came from is who we want to be facing.
        if (velocity::IsDelaying() || antikb::IsDelaying())
            SortTowardsVelocityYaw();

        AdvanceTargetNumber();

        if (targetNumber_ >= targets_.size())
            targetNumber_ = 0;

        if (targets_.empty()) {
            ClearTargets();
            return;
        }

        const java::EntityHandle* target = &targets_[targetNumber_].entity;

        // Published before the two checks below, which can both drop the aura out
        // of this tick: the hud is reporting who the aura has picked, not whether
        // the ray happened to reach them.
        int targetId = 0;
        if (java::EntityId(*target, targetId))
            g_targetId.store(targetId, std::memory_order_release);
        else
            g_targetId.store(0, std::memory_order_release);
        g_hasTargets.store(true, std::memory_order_release);
        if (autoblock::AttackCycleActive()) {
            double border = 0;
            hypixelGeometryValid_ = target->BoundingBox(hypixelBox_) &&
                target->PickRadius(border) && target->Position(hypixelPosition_);
            if (hypixelGeometryValid_) {
                hypixelBox_ = java::BoxInflate(hypixelBox_, border);
                hypixelGeometryId_ = targetId;
            }
        }
        PublishSwitchTargets();

        const rotation::Rotation calculate =
            rotation::CalculateEntity(*target, true, range);
        if (rotation::RayCast(calculate, range).type == rotation::HitResultType::Block)
            return;

        rotation::SetRotations(
            NoisyAim(calculate, *target, targetId, range, continueNoise),
            static_cast<double>(static_cast<int>(rotationSpeed_->get())),
            [target](const rotation::Rotation& rotation) {
                const rotation::HitResult result = rotation::RayCast(rotation, kRaycastRange);
                return result.type == rotation::HitResultType::Entity &&
                       result.entity.IsSame(*target);
            }, rotation::RotationPriority::Medium, rotationNoise_->get() <= 0.0);

        if (aimPointEsp_->get()) {
            const auto hit = rotation::RayCast(rotation::GetRotation(), range);
            rotation::Vec3 position;
            auto world = java::LevelEntity();
            if (hit.type == rotation::HitResultType::Entity && hit.entity.IsSame(*target) &&
                target->Position(position) && world) {
                std::lock_guard lock(aimPointMutex_);
                aimPointWorld_ = std::move(world);
                aimPointOffset_ = hit.location.Subtract(position);
                aimPointTarget_ = targetId;
                aimPointUpdated_ = NowMillis();
                aimPointValid_ = true;
            }
        }

        if (autoblock::AttackCycleActive() || UsesAttackCooldown() || !hitSel_->get())
            return;

        java::EntityHandle hovered;
        if (!java::AttackTargetEntity(hovered) || !hovered.IsPlayer() || (teams::IsTeammate(hovered) || antibot::IsBot(hovered)))
            return;
        if (velocity::AttackQueue() > 0 || antikb::AttackQueue() > 0)
            return;

        const long long now = NowMillis();
        if (now - lastAttackTime_ < AttackDelay(cps_->get()))
            return;

        int hurtTime = 0;
        if (!hovered.HurtTime(hurtTime))
            return;

        if (hurtTime <= 1 || (java::PlayerHurtTime() >= 6 && !antikb::IsEnabled())) {
            ++attacks_;
            lastAttackTime_ = now;
        }
    }

    void ClearAimPoint() {
        std::lock_guard lock(aimPointMutex_);
        aimPointValid_ = false;
        aimPointWorld_.Reset();
    }

    void onRender() override {
        if (!aimPointEsp_->get() || !ImGui::GetCurrentContext()) return;
        auto world = java::LevelEntity();
        if (!world || !java::PlayerEntity()) return;
        rotation::Vec3 offset;
        int id;
        {
            std::lock_guard lock(aimPointMutex_);
            if (!aimPointValid_ || NowMillis()-aimPointUpdated_>200 ||
                !aimPointWorld_ || !aimPointWorld_.IsSame(world)) return;
            offset=aimPointOffset_;id=aimPointTarget_;
        }
        auto target=java::EntityById(id);
        rotation::Vec3 now,before;
        if (!target || !target.IsAlive() || !target.RenderPositionPair(now,before)) return;
        const float partial=java::FrameTime();
        java::CameraView camera;
        const auto display=ImGui::GetIO().DisplaySize;
        if (!std::isfinite(partial) || display.x<=1 || display.y<=1 || !java::ReadCameraView(partial,camera)) return;
        // Smooth the local hit position at render frequency, while following the
        // entity's interpolated motion directly so the marker does not trail it.
        const double time=ImGui::GetTime();
        if (aimPointRenderTarget_!=id || time-aimPointRenderTime_>.2 || aimPointRenderTime_<0) {
            aimPointRenderOffset_=offset;
        } else {
            const double blend=1.0-std::exp(-22.0*std::clamp(time-aimPointRenderTime_,0.0,.1));
            aimPointRenderOffset_=aimPointRenderOffset_.Add(
                (offset.x-aimPointRenderOffset_.x)*blend,
                (offset.y-aimPointRenderOffset_.y)*blend,
                (offset.z-aimPointRenderOffset_.z)*blend);
        }
        aimPointRenderTarget_=id;aimPointRenderTime_=time;
        const auto center=before.Add((now.x-before.x)*partial+aimPointRenderOffset_.x,
            (now.y-before.y)*partial+aimPointRenderOffset_.y,(now.z-before.z)*partial+aimPointRenderOffset_.z);
        const auto basis=minecraft::projection::MakeBasis(camera,partial);
        constexpr double radius=.045;
        const double x=center.x,y=center.y,z=center.z,r=radius;
        const std::array<rotation::Vec3,8> corners{{{x-r,y-r,z-r},{x+r,y-r,z-r},
            {x+r,y-r,z+r},{x-r,y-r,z+r},{x-r,y+r,z-r},{x+r,y+r,z-r},
            {x+r,y+r,z+r},{x-r,y+r,z+r}}};
        const auto project=[&](rotation::Vec3 point,ImVec2& out) {
            minecraft::projection::ScreenPoint screen;
            if (!minecraft::projection::Project(basis,point,display,screen) ||
                !std::isfinite(screen.x) || !std::isfinite(screen.y)) return false;
            out={screen.x,screen.y};return true;
        };
        auto* draw=ImGui::GetBackgroundDrawList();
        minecraft::projection::DrawClippedBox<ImVec2>(corners,project,
            [&](const auto& polygon) { draw->AddConvexPolyFilled(polygon.data(),static_cast<int>(polygon.size()),IM_COL32(135,235,245,255)); },
            [](ImVec2,ImVec2) {});
    }

    rotation::Rotation NoisyAim(const rotation::Rotation& aim, const java::EntityHandle& target,
                                int targetId, double range, bool continuing) {
        const double sigma = rotationNoise_->get();
        if (sigma <= 0.0) return aim;
        if (!continuing || noiseTargetId_ != targetId) noiseOffset_ = {};
        noiseTracking_ = true;
        noiseTargetId_ = targetId;
        // Correlated normal samples: rho=.8, sqrt(1-rho^2)=.6. This preserves
        // the configured stationary standard deviation without frame-rate noise.
        noiseOffset_.yaw = static_cast<float>(std::clamp(
            .8 * noiseOffset_.yaw + .6 * rotation::Gaussian(0.0, 1.0), -3.0, 3.0));
        noiseOffset_.pitch = static_cast<float>(std::clamp(
            .8 * noiseOffset_.pitch + .6 * rotation::Gaussian(0.0, 1.0), -3.0, 3.0));
        const rotation::Rotation candidate{
            static_cast<float>(aim.yaw + sigma * noiseOffset_.yaw),
            rotation::ClampPitch(static_cast<float>(aim.pitch + sigma * noiseOffset_.pitch))};
        const auto hit = rotation::RayCast(candidate, range);
        // Noise must not select a neighbouring entity or aim into a wall. The
        // manager still owns speed limits, sensitivity, continuous yaw and move fix.
        return hit.type == rotation::HitResultType::Entity && hit.entity.IsSame(target)
            ? candidate : aim;
    }

    void OnPlayerTick() {
        if (!isEnabled())
            return;
        if (crystal::IsWorking() || explosive::autoCrystalWorking || explosive::anchorWorking || explosive::feetWorking) { attacks_ = 0; return; }
        if (breaker::BlocksAura() || prediction::StoppedBlock() || prediction::CancelsAura()) { attacks_ = 0; return; }
        if (antikb::AttackQueue() > 0) return;
        if (autoblock::Active()) return;

        if (java::ScreenOpen() || clickgui::IsOpen() || scaffold::IsEnabled() || automace::IsWorking() || automlg::IsWorking() || autocharge::IsWorking()) {
            attacks_ = 0;
            return;
        }
        if (UsesAttackCooldown()) {
            // Discard legacy requests as well when switching modes. A charged
            // weapon may issue at most one attack in this callback.
            attacks_ = 0;
            if (!g_hasTargets.load(std::memory_order_acquire) ||
                velocity::AttackQueue() > 0 || java::PlayerUsingItem()) return;
            java::EntityHandle hovered;
            if (!java::AttackTargetEntity(hovered) || (teams::IsTeammate(hovered) || antibot::IsBot(hovered))) return;
            if (hitSel_->get() && hovered.IsPlayer()) {
                int hurtTime = 0;
                if (!hovered.HurtTime(hurtTime)) return;
                if (hurtTime > 1 && !(java::PlayerHurtTime() >= 6 && !antikb::IsEnabled())) return;
            }
            if (!java::PlayerAttackCooldownReady()) return;
            const bool attacked = waitCritical_->get()
                ? java::AttackWithCriticalWait(hovered)
                : java::AttackAndSwing(hovered);
            if (attacked) lastAttackTime_ = NowMillis();
            return;
        }
        while (attacks_ > 0) {
            --attacks_;
            if (java::PlayerUsingItem()) return;
            java::EntityHandle hovered;
            if (java::AttackTargetEntity(hovered) && !(teams::IsTeammate(hovered) || antibot::IsBot(hovered))) java::AttackAndSwing(hovered);
        }
    }

    void OnPrePlayerInteract() {
        if (!isEnabled() || !autoblock::Active()) return;
        if (crystal::IsWorking() || explosive::autoCrystalWorking || explosive::anchorWorking || explosive::feetWorking) { attacks_ = 0; return; }
        if (breaker::BlocksAura()) { attacks_ = 0; return; }
        if (autoblock::AttackCycleActive()) { attacks_ = 0; return; }
        if (java::ScreenOpen() || clickgui::IsOpen() || scaffold::IsEnabled() || automace::IsWorking() || automlg::IsWorking() || autocharge::IsWorking()) {
            attacks_ = 0;
            return;
        }
        if (antikb::AttackQueue() > 0) return;
        java::EntityHandle hovered;
        if (java::AttackTargetEntity(hovered) && (teams::IsTeammate(hovered) || antibot::IsBot(hovered))) {
            attacks_ = 0;
            return;
        }
        autoblock::SubmitAttacks(std::exchange(attacks_, 0));
    }

    [[nodiscard]] bool CollectTargets(const java::EntityHandle& self,
                                      const rotation::Vec3& eye, float playerYaw,
                                      double range, float fovValue, bool attackInvisible) {
        rotation::ClearRaycast();
        targets_.clear();

        rotation::Vec3 position;
        if (!self.Position(position))
            return false;

        std::vector<java::EntityHandle> entities;
        if (!java::WorldEntities(self, SearchArea(position, range), entities))
            return false;

        for (java::EntityHandle& entity : entities) {
            if (!IsTarget(entity, self))
                continue;

            if (!attackInvisible && IsInvisiblePlayer(entity))
                continue;

            if (rotation::EntityDistance(eye, entity) > range)
                continue;

            const rotation::Rotation rotations = rotation::CalculateEntity(entity);
            if (std::abs(rotation::mth::WrapDegrees(playerYaw - rotations.yaw)) > fovValue)
                continue;

            Candidate candidate;
            candidate.entity = std::move(entity);
            targets_.push_back(std::move(candidate));
        }

        return true;
    }

    void SortTargets(const java::EntityHandle& self, const rotation::AimFrame& frame,
                     float playerYaw, double range) {
        const int priority = priority_->get();
        if (priority == 3)
            return;

        if (priority == 0) {
            for (Candidate& candidate : targets_) {
                float health = 0.0f;
                float absorption = 0.0f;
                candidate.sortKey = candidate.entity.Health(health, absorption)
                                        ? static_cast<double>(health)
                                        : 0.0;
            }
        } else if (priority == 1) {
            for (Candidate& candidate : targets_) {
                const rotation::Rotation rotations = rotation::CalculateEntity(candidate.entity);
                candidate.sortKey =
                    static_cast<double>(std::abs(rotation::mth::WrapDegrees(playerYaw - rotations.yaw)));
            }
        } else {
            for (Candidate& candidate : targets_) {
                double distance = 0.0;
                candidate.sortKey = candidate.entity.DistanceTo(self, distance)
                                        ? distance
                                        : 0.0;
            }
        }

        std::stable_sort(targets_.begin(), targets_.end(),
                         [](const Candidate& a, const Candidate& b) {
                             return a.sortKey < b.sortKey;
                         });
    }

    // Velocity's override: while it holds a knockback the target list is ordered
    // by how close each candidate sits to the direction the hit came from.
    void SortTowardsVelocityYaw() {
        const bool epsilon = antikb::IsDelaying();
        const float yaw = epsilon ? antikb::Yaw() : velocity::Yaw();
        for (Candidate& candidate : targets_) {
            const rotation::Rotation rotations = rotation::CalculateEntity(candidate.entity);
            candidate.sortKey = static_cast<double>(
                std::fabs(epsilon ? yaw - rotations.yaw :
                    rotation::mth::WrapDegrees(yaw - rotations.yaw)));
        }

        std::stable_sort(targets_.begin(), targets_.end(),
                         [](const Candidate& a, const Candidate& b) {
                             return a.sortKey < b.sortKey;
                         });
    }

    // The list the target hud draws a card per entry for. Only a switching aura has
    // one worth showing: Single holds a target at a time and the hud already draws
    // that one, so it publishes nothing and the hud stays a single card.
    void PublishSwitchTargets() {
        if (mode_->get() != 1) {
            ClearSwitchTargets();
            return;
        }

        std::vector<int> ids;
        ids.reserve(targets_.size());
        for (const Candidate& candidate : targets_) {
            int id = 0;
            if (java::EntityId(candidate.entity, id))
                ids.push_back(id);
        }

        AcquireSRWLockExclusive(&g_switchLock);
        g_switchTargets = std::move(ids);
        ReleaseSRWLockExclusive(&g_switchLock);
    }

    void AdvanceTargetNumber() {
        if (mode_->get() != 1)
            targetNumber_ = 0;
        else if (NowMillis() - switchTimer_ >= static_cast<long long>(switchDelay_->get())) {
            switchTimer_ = NowMillis();
            if (++targetNumber_ >= targets_.size())
                targetNumber_ = 0;
        }
    }

    [[nodiscard]] bool HoveredIsPlayer() {
        java::EntityHandle hovered;
        return java::AttackTargetEntity(hovered) && hovered.IsPlayer() && !(teams::IsTeammate(hovered) || antibot::IsBot(hovered));
    }

    [[nodiscard]] bool IsInvisiblePlayer(const java::EntityHandle& entity) const {
        unsigned types = 0;
        bool invisible = false;
        return java::EntityTypes(entity, types) && (types & java::kTypePlayer) != 0 &&
               entity.IsInvisible(invisible) && invisible;
    }

    [[nodiscard]] bool IsTarget(const java::EntityHandle& entity,
                                const java::EntityHandle& self) const {
        unsigned types = 0;
        if (!java::EntityTypes(entity, types) || !(types & java::kTypeLiving))
            return false;

        if (entity.IsSame(self))
            return false;
        if (teams::IsTeammate(self, entity) || antibot::IsBot(entity))
            return false;
        if ((types & java::kTypePlayer) && entity.IsSpectator())
            return false;
        if (!entity.IsAlive())
            return false;

        bool invisible = false;
        if (entity.IsInvisible(invisible) && invisible && !targetInvisibles_->get())
            return false;

        if (types & java::kTypePlayer)
            return targetPlayer_->get();
        if (types & java::kTypeVillager)
            return targetVillager_->get();
        if (types & java::kTypeMonster)
            return targetMobs_->get();
        if (types & java::kTypeAnimal)
            return targetAnimals_->get();
        return false;
    }

    Properties::BoolProperty* targetPlayer_ = nullptr;
    std::vector<Properties::PropDesc> configRows_;
    Properties::BoolProperty* targetInvisibles_ = nullptr;
    Properties::BoolProperty* targetAnimals_ = nullptr;
    Properties::BoolProperty* targetMobs_ = nullptr;
    Properties::BoolProperty* targetVillager_ = nullptr;
    Properties::BoolProperty* hitSel_ = nullptr;
    Properties::BoolProperty* attackInvisible_ = nullptr;
    Properties::ModeProperty* cpsMode_ = nullptr;
    Properties::BoolProperty* waitCritical_ = nullptr;
    Properties::BoolProperty* interruptSprint_ = nullptr;
    Properties::BoolProperty* criticalSound_ = nullptr;
    Properties::NumberProp* cps_ = nullptr;
    Properties::NumberProp* aimRange_ = nullptr;
    Properties::NumberProp* fov_ = nullptr;
    Properties::NumberProp* rotationSpeed_ = nullptr;
    Properties::NumberProp* rotationNoise_ = nullptr;
    Properties::BoolProperty* aimPointEsp_ = nullptr;
    std::mutex aimPointMutex_;
    java::EntityHandle aimPointWorld_;
    rotation::Vec3 aimPointOffset_;
    int aimPointTarget_ = 0;
    long long aimPointUpdated_ = 0;
    bool aimPointValid_ = false;
    // Render-thread-only animation state; tick snapshots remain mutex-protected.
    rotation::Vec3 aimPointRenderOffset_;
    int aimPointRenderTarget_ = 0;
    double aimPointRenderTime_ = -1;
    rotation::Rotation noiseOffset_;
    int noiseTargetId_ = 0;
    bool noiseTracking_ = false;
    Properties::ModeProperty* mode_ = nullptr;
    Properties::NumberProp* switchDelay_ = nullptr;
    Properties::ModeProperty* priority_ = nullptr;
    Properties::BoolProperty* autoBlock_ = nullptr;
    event::Subscription gameTick_;
    event::Subscription criticalTick_, criticalInput_;
    killaura::CriticalSprint criticalSprint_;
    event::Subscription playerTick_;
    event::Subscription interact_;

    java::Box hypixelBox_;
    rotation::Vec3 hypixelPosition_;
    int hypixelGeometryId_ = 0;
    bool hypixelGeometryValid_ = false;
    std::vector<Candidate> targets_;
    std::size_t targetNumber_ = 0;
    int attacks_ = 0;
    long long lastAttackTime_ = 0;
    long long switchTimer_ = 0;
};

}

namespace killaura {
bool CriticalSoundEnabled() { return auraInstance && auraInstance->CriticalSoundEnabled(); }
bool IsEnabled() { return auraInstance && auraInstance->isEnabled(); }
bool HasBlockTarget() { return auraInstance && auraInstance->HasBlockTarget(); }
bool ReadHypixelGeometry(java::Box& box, rotation::Vec3& position) {
    return auraInstance && auraInstance->ReadHypixelGeometry(box, position);
}

bool HasTargets() { return g_hasTargets.load(std::memory_order_acquire) && g_targetId.load(std::memory_order_acquire) != 0; }

bool QueueAutoBlockAttack(bool releaseEarly) {
    return !breaker::BlocksAura() && autoblock::QueueAttack(releaseEarly);
}

int TargetId() { return g_targetId.load(std::memory_order_acquire); }
double AttackRange() { return kRaycastRange; }

bool ReadHypixelTarget(java::EntityHandle& target, double range, bool& hasBlockTarget, int& normalDelay) {
    return !breaker::BlocksAura() && auraInstance && auraInstance->ReadHypixelTarget(target, range, hasBlockTarget, normalDelay);
}

bool SwitchTargets(std::vector<int>& out) {
    AcquireSRWLockShared(&g_switchLock);
    out = g_switchTargets;
    ReleaseSRWLockShared(&g_switchLock);
    return !out.empty();
}

}

std::unique_ptr<Module> MakeKillAuraModule() {
    return std::make_unique<KillAuraModule>();
}

}
