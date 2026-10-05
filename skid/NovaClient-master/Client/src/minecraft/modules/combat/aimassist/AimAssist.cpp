#include "minecraft/modules/misc/antibot/AntiBot.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/misc/teams/Teams.h"
#include "minecraft/modules/combat/killaura/KillAura.h"
#include "minecraft/modules/combat/aimassist/AimTargetSelection.h"
#include "minecraft/modules/combat/aimassist/AimPoint.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_input.h"
#include "minecraft/java/api/game_player.h"
#include "minecraft/java/api/game_rotation.h"
#include "ui/clickgui/ClickGui.h"
#include "utils/Keyboard.h"
#include "minecraft/utils/rotation/AimUtils.h"
#include "minecraft/utils/rotation/Mth.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include "minecraft/utils/rotation/Rotation.h"
#include "minecraft/utils/rotation/RotationManager.h"

namespace nova::module {
namespace {

struct Candidate {
    java::EntityHandle entity;
    double sortKey = 0.0;
    double distanceSq = 0.0;
};

[[nodiscard]] long long NowMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

[[nodiscard]] std::string WholeNumber(double value) {
    return std::format("{:.0f}", value);
}

[[nodiscard]] std::string Percent(double value) {
    return std::format("{:.0f}%", value);
}

[[nodiscard]] std::string Milliseconds(double value) {
    return std::format("{:.0f} ms", value);
}

[[nodiscard]] bool IsMining() {
    if (!nova::input::IsMouseButtonDown(nova::input::MouseButton::Left))
        return false;
    return nova::java::TargetedBlockBreakable();
}

class AimAssistModule final : public Module {
public:
    AimAssistModule() : Module("AimAssist", Category::Combat, "General", "General") {
        auto aim = subMode("Aim", 0, {"Single", "Switch"});
        aimMode_ = static_cast<Properties::ModeProperty*>(aim.getProp());
        auto switchDelay = subNumber("Switch delay", 0, 1000, 50, 250, Milliseconds);
        switchDelay_ = static_cast<Properties::NumberProp*>(switchDelay.getProp());
        std::vector<Properties::PropDesc> aimSettings;
        aimSettings.push_back(std::move(aim));
        aimSettings.emplace_back(switchDelay.getName(), switchDelay.getProp(),
            std::vector<Properties::PropDesc>{}, [this] { return aimMode_->get() == 1; });
        auto aimPart = subMode("Aim part", 0, {"Head", "Chest", "Legs"});
        aimPart_ = static_cast<Properties::ModeProperty*>(aimPart.getProp());
        aimSettings.push_back(std::move(aimPart));
        mode_ = &addMode("Mode", 0, {"Normal", "Silent"}, std::move(aimSettings));
        backSpeed_ = &addNumber("Rotation back speed", 10, 720, 10, 180, {}, [](double value) {
            return std::format("{:.0f} deg/s", value);
        }, [this] { return mode_->get() == 1; });
        auto degrees = [](double value) { return std::format("{:.0f} deg/s", value); };
        auto normal = [this] { return mode_->get() == 0; };
        horizontalSpeed_ = &addNumber("Horizontal speed", 0, 720, 10, 180, {}, degrees);
        verticalSpeed_ = &addNumber("Vertical speed", 0, 720, 10, 180, {}, degrees);
        auto smoothAmount = subNumber("Smooth amount", 0, 100, 1, 50, Percent);
        smoothAmount_ = static_cast<Properties::NumberProp*>(smoothAmount.getProp());
        smooth_ = &addBool("Smooth", false, {std::move(smoothAmount)}, normal);
        multipointHorizontal_ = &addNumber("Multipoint horizontal", 0, 100, 1, 0, {}, Percent);
        multipointVertical_ = &addNumber("Multipoint vertical", 0, 100, 1, 0, {}, Percent);
        fov_ = &addNumber("FOV", 15, 360, 1, 90, {}, WholeNumber);
        range_ = &addNumber("Range", 0, 5, 0.1, 4.5, {}, [](double value) {
            return std::format("{:.1f}", value);
        });
        sort_ = &addMode("Sort", 1, {"Health", "Angle", "Hurt time", "Distance"});
        ignoreBehindWalls_ = &addBool("Ignore behind walls");
        ignoreBehindEntities_ = &addBool("Ignore behind entities");
        aimInvis_ = &addBool("Aim invis");
        clickAim_ = &addBool("Require mouse", true);
        stopWhenBreaking_ = &addBool("Stop when breaking");
        keepMoveDirection_ = &addBool("Keep move direction", true, {},
                                      [this] { return mode_->get() == 1; });
        hoverDelay_ = &addNumber("Hover delay", 0, 500, 10, 100, {}, Milliseconds,
                                 [this] { return stopWhenBreaking_->get(); });
        weaponOnly_ = &addBool("Weapon only");
    }

private:
    void onEnable() override { miningStartTime_ = -1; ClearAim(true); }
    void onDisable() override { miningStartTime_ = -1; ClearAim(IsCircuitOpen()); }

    void ReleaseSilent(bool immediate = false) {
        if (silentRequested_) {
            if (immediate) rotation::ReleaseLinearRotations();
            else rotation::ReturnLinearRotations(backSpeed_->get());
        }
        silentRequested_ = false;
    }

    void ClearAim(bool immediate = false) {
        ReleaseSilent(immediate);
        frameClock_.Reset();
        target_.Reset();
        targetSelection_.Reset();
        lastTick_ = -1;
    }

    void onUpdate() override {
        if (lastMode_ != mode_->get()) {
            // Mode changes must not leave a silent return driving Normal aim.
            if (rotation::State().linearReturning) rotation::ReleaseLinearRotations();
            ClearAim(true);
            lastMode_ = mode_->get();
        }
        // Conditions are checked every frame so mouse release/menu opening does
        // not leave an old tick segment driving the camera.
        if (!ConditionsMet()) {
            ClearAim();
            return;
        }
        const int tick = java::PlayerTickCount();
        if (tick < lastTick_) ClearAim(true);
        const bool newTick = tick != lastTick_;
        const bool silent = mode_->get() == 1;
        if (newTick) {
            lastTick_ = tick;
            auto nextTarget = GetEnemy();
            target_ = std::move(nextTarget);
        }
        if (!target_.Valid() || !target_.IsAlive()) {
            frameClock_.Reset();
            ReleaseSilent();
            return;
        }

        auto frame = rotation::ReadAimFrame();
        float yaw = 0.0f, pitch = 0.0f;
        rotation::Vec3 point;
        if (!frame.valid || !java::PlayerRotation(yaw, pitch) ||
            !FindAimPoint(target_, frame, {yaw, pitch}, point)) {
            // Revalidate against the current camera and current FOV before
            // every turn, including frames between selection ticks.
            ClearAim();
            return;
        }
        if (!std::isfinite(yaw) || !std::isfinite(pitch) ||
            !std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z)) {
            frameClock_.Reset();
            ReleaseSilent();
            return;
        }
        const rotation::Rotation current = silent ? rotation::GetRotation() : rotation::Rotation{yaw, pitch};
        const double smoothAmount = !silent && smooth_->get() ? smoothAmount_->get() : 0.0;
        if (smoothAmount > 0) {
            const double partial = java::FrameTime();
            rotation::Vec3 now, before;
            // Interpolate both sides, so moving the local player does not
            // reintroduce tick-sized jumps in the relative aim direction.
            if (target_.RenderPositionPair(now, before)) {
                const auto offset = aimassist::SmoothRenderOffset(now, before, partial, smoothAmount);
                point = point.Add(offset.x, offset.y, offset.z);
            }
            const auto self = java::PlayerEntity();
            if (self.RenderPositionPair(now, before)) {
                const auto offset = aimassist::SmoothRenderOffset(now, before, partial, smoothAmount);
                frame.eye = frame.eye.Add(offset.x, offset.y, offset.z);
            }
            if (!aimassist::PointInFov({yaw, pitch}, frame.eye, point, fov_->get())) {
                ClearAim();
                return;
            }
        }
        const double dx = point.x - frame.eye.x;
        const double dy = point.y - frame.eye.y;
        const double dz = point.z - frame.eye.z;
        const double horizontal = std::hypot(dx, dz);
        constexpr double toDegrees = 57.29577951308232;
        const rotation::Rotation desired{
            horizontal < 1e-8 ? current.yaw : static_cast<float>(std::atan2(dz, dx) * toDegrees - 90.0),
            static_cast<float>(-std::atan2(dy, horizontal) * toDegrees)};

        if (silent) {
            frameClock_.Reset();
            if (newTick && rotation::SetLinearRotations(desired, horizontalSpeed_->get(), verticalSpeed_->get())) {
                silentRequested_ = true;
                rotation::SetMoveInputFix(keepMoveDirection_->get());
            }
            return;
        }

        ReleaseSilent();
        const double now = std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        const double elapsed = frameClock_.Advance(now);
        if (elapsed <= 0.0) return;
        // Always start at the actual camera: the user's mouse motion is not
        // overwritten with an old endpoint. Smooth never scales angular speed.
        const auto next = rotation::AxisLinearStep(current, desired,
            horizontalSpeed_->get() * elapsed, verticalSpeed_->get() * elapsed);
        if (smoothAmount <= 0 || !java::SetPlayerCameraRotation(next.yaw, next.pitch, smoothAmount))
            java::SetPlayerRotation(next.yaw, next.pitch);
    }

    bool FindAimPoint(const java::EntityHandle& target, const rotation::AimFrame& frame,
                      rotation::Rotation view, rotation::Vec3& out) {
        const double range = range_->get();
        if (!(rotation::DistanceSqFromEyeToClosestOnAABB(target) <= range * range)) return false;
        java::Box box;
        if (!target.BoundingBox(box) || !aimassist::PreferredPoint(box, aimPart_->get(), frame.eye,
            multipointHorizontal_->get(), multipointVertical_->get(), out)) return false;
        const bool throughBlocks = !ignoreBehindWalls_->get();
        const bool throughEntities = !ignoreBehindEntities_->get();
        auto inFov = [&](const rotation::Vec3& point) {
            return aimassist::PointInFov(view, frame.eye, point, fov_->get());
        };
        if (throughBlocks && throughEntities) return inFov(out);
        if (inFov(out) && rotation::CanAimAtPoint(target, frame.eye, out, range, throughBlocks, throughEntities))
            return true;
        const auto preferred = out;
        auto backups = rotation::BuildBackupPoints(target, frame.eye);
        std::stable_sort(backups.begin(), backups.end(), [&preferred](const auto& a, const auto& b) {
            return preferred.DistanceToSqr(a) < preferred.DistanceToSqr(b);
        });
        for (const auto& point : backups) {
            if (inFov(point) && rotation::CanAimAtPoint(target, frame.eye, point, range, throughBlocks, throughEntities)) {
                out = point;
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool ConditionsMet() {
        if (java::ScreenOpen() || nova::clickgui::IsOpen() || scaffold::IsEnabled() || automlg::IsWorking() || autocharge::IsWorking() ||
            killaura::HasTargets() || (rotation::IsActive() && !rotation::State().linear)) {
            miningStartTime_ = -1;
            return false;
        }
        if (!java::MouseGrabbed()) {
            miningStartTime_ = -1;
            return false;
        }
        if (weaponOnly_->get() && !java::HoldingWeapon()) {
            miningStartTime_ = -1;
            return false;
        }
        if (clickAim_->get() &&
            !nova::input::IsMouseButtonDown(nova::input::MouseButton::Left)) {
            miningStartTime_ = -1;
            return false;
        }

        if (stopWhenBreaking_->get() && IsMining()) {
            if (miningStartTime_ < 0)
                miningStartTime_ = NowMillis();
            if (NowMillis() - miningStartTime_ >= static_cast<long long>(hoverDelay_->get()))
                return false;
        } else {
            miningStartTime_ = -1;
        }

        return true;
    }

    [[nodiscard]] java::EntityHandle GetEnemy() {
        const rotation::AimFrame frame = rotation::ReadAimFrame();
        if (!frame.valid)
            return java::EntityHandle();

        float playerYaw = 0.0f;
        float playerPitch = 0.0f;
        if (!java::PlayerRotation(playerYaw, playerPitch))
            return java::EntityHandle();

        java::EntityHandle self = java::PlayerEntity();
        if (!self.Valid())
            return java::EntityHandle();

        std::vector<java::EntityHandle> players;
        if (!java::WorldPlayers(players))
            return java::EntityHandle();

        const double rangeValue = range_->get();
        const double rangeSq = rangeValue * rangeValue;
        const bool ignoreInvisible = !aimInvis_->get();
        const int sortMode = sort_->get();

        std::vector<Candidate> candidates;
        for (java::EntityHandle& entity : players) {
            if (!entity.Valid() || entity.IsSame(self) || !entity.IsAlive() || entity.IsSpectator())
                continue;

            int deathTime = 0;
            if (!entity.DeathTime(deathTime) || deathTime != 0)
                continue;

            float health = 0.0f;
            float absorption = 0.0f;
            if (!entity.Health(health, absorption))
                continue;

            if (teams::IsTeammate(self, entity) || antibot::IsBot(entity))
                continue;

            bool invisible = false;
            if (ignoreInvisible && entity.IsInvisible(invisible) && invisible)
                continue;

            const double distanceSq = rotation::DistanceSqFromEyeToClosestOnAABB(entity);
            if (!(distanceSq <= rangeSq))
                continue;

            rotation::Vec3 position;
            if (!entity.Position(position))
                continue;

            Candidate candidate;
            candidate.distanceSq = distanceSq;

            switch (sortMode) {
                case 0:
                    candidate.sortKey = static_cast<double>(health + absorption);
                    break;
                case 2: {
                    int hurtTime = 0;
                    entity.HurtTime(hurtTime);
                    candidate.sortKey = static_cast<double>(hurtTime);
                    break;
                }
                case 3: {
                    double squared = distanceSq;
                    entity.DistanceSqTo(self, squared);
                    candidate.sortKey = squared;
                    break;
                }
                default: {
                    double eyeHeight = 0.0;
                    entity.EyeHeight(eyeHeight);
                    candidate.sortKey =
                        std::fabs(rotation::AimYawDifference(playerYaw, frame.feet, position)) +
                        std::fabs(rotation::AimPitchDifference(playerPitch, frame.feet,
                                                               frame.eyeHeight, position,
                                                               eyeHeight));
                    break;
                }
            }

            candidate.entity = std::move(entity);
            candidates.push_back(std::move(candidate));
        }

        std::stable_sort(candidates.begin(), candidates.end(),
                         [](const Candidate& a, const Candidate& b) {
                             if (a.sortKey != b.sortKey)
                                 return a.sortKey < b.sortKey;
                             return a.distanceSq < b.distanceSq;
                         });

        std::erase_if(candidates, [&](const Candidate& candidate) {
            rotation::Vec3 point;
            return !FindAimPoint(candidate.entity, frame, {playerYaw, playerPitch}, point);
        });

        if (candidates.empty()) {
            targetSelection_.Reset();
            return java::EntityHandle();
        }

        // Track entity identity rather than a persistent vector index: sorting,
        // deaths and new players can change the list between game ticks.
        std::size_t current = candidates.size();
        if (target_.Valid()) {
            for (std::size_t i = 0; i < candidates.size(); ++i) {
                if (candidates[i].entity.IsSame(target_)) { current = i; break; }
            }
        }
        const auto selected = targetSelection_.Choose(candidates.size(), current,
            aimMode_->get() == 1, NowMillis(), static_cast<long long>(switchDelay_->get()));
        return std::move(candidates[selected].entity);
    }

    Properties::ModeProperty* mode_ = nullptr;
    Properties::ModeProperty* aimMode_ = nullptr;
    Properties::ModeProperty* aimPart_ = nullptr;
    Properties::NumberProp* switchDelay_ = nullptr;
    Properties::NumberProp* backSpeed_ = nullptr;
    Properties::NumberProp* horizontalSpeed_ = nullptr;
    Properties::NumberProp* verticalSpeed_ = nullptr;
    Properties::BoolProperty* smooth_ = nullptr;
    Properties::NumberProp* smoothAmount_ = nullptr;
    Properties::NumberProp* multipointHorizontal_ = nullptr;
    Properties::NumberProp* multipointVertical_ = nullptr;
    Properties::NumberProp* fov_ = nullptr;
    Properties::NumberProp* range_ = nullptr;
    Properties::ModeProperty* sort_ = nullptr;
    Properties::BoolProperty* ignoreBehindWalls_ = nullptr;
    Properties::BoolProperty* ignoreBehindEntities_ = nullptr;
    Properties::BoolProperty* aimInvis_ = nullptr;
    Properties::BoolProperty* clickAim_ = nullptr;
    Properties::BoolProperty* stopWhenBreaking_ = nullptr;
    Properties::BoolProperty* keepMoveDirection_ = nullptr;
    Properties::NumberProp* hoverDelay_ = nullptr;
    Properties::BoolProperty* weaponOnly_ = nullptr;

    long long miningStartTime_ = -1;
    int lastTick_ = -1;
    int lastMode_ = -1;
    bool silentRequested_ = false;
    java::EntityHandle target_;
    aimassist::TargetSelection targetSelection_;
    rotation::AimFrameClock frameClock_;
};

}

std::unique_ptr<Module> MakeAimAssistModule() {
    return std::make_unique<AimAssistModule>();
}

}
