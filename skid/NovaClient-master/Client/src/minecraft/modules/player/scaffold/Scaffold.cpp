#include <algorithm>
#include <atomic>
#include <array>
#include <limits>
#include <cmath>
#include <chrono>
#include <format>
#include <memory>
#include <mutex>
#include <map>
#include <optional>
#include <tuple>
#include <vector>
#include <string>
#include <utility>

#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/java/api/game_rotation.h"
#include "utils/Keyboard.h"
#include "minecraft/utils/rotation/FallingPlayer.h"
#include "minecraft/utils/rotation/MathConst.h"
#include "minecraft/utils/rotation/Mth.h"
#include "minecraft/utils/rotation/Random.h"
#include "minecraft/utils/rotation/RayCastUtil.h"
#include "minecraft/utils/rotation/Rotation.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/RotationUtils.h"

// Scaffold: existing mode control with Rise 6.9.5 placement selection.
//
// It keeps a block under the player while they walk off an edge: every tick it
// selects an adjacent replaceable position nearest the player foot, aims at the face
// it found, and right-clicks it. When the player is falling faster than it can
// keep up with, or moving too fast to place reliably, it switches to a burst --
// up to nine placement attempts inside one tick, each one skipping the player's
// own tick so the rotation it just sent is what the next movement is built from.
//
// The ESP overlay the original carries is not ported; the game has no world render
// hook on this side for it to hang off.

// Hypixel angle helpers and both mode paths live in this translation unit.
namespace nova::module::scaffold {
namespace {

// @SNAP_WIRE_BEGIN@
struct SnapWireHistory {
    bool available = false, hasLook = false, rotated = false;
    bool pendingPlace = false, hasPlaced = false;
    float yaw = 0, pitch = 0, delta = 0, placedDelta = 0;

    void Observe(bool movement, bool look, float nextYaw, float nextPitch) {
        available = true;
        if (!movement) { pendingPlace = true; return; }
        // CheckManagerListener.handleFlying: places are drained first. The
        // incoming look is available to ray tests, but NOT RotationListener yet.
        if (pendingPlace) {
            if (rotated) { placedDelta = delta; hasPlaced = true; rotated = false; }
            pendingPlace = false;
        }
        if (look && std::isfinite(nextYaw) && std::isfinite(nextPitch)) {
            if (hasLook) { delta = std::fabs(nextYaw - yaw); rotated = true; }
            yaw = nextYaw; pitch = nextPitch; hasLook = true;
        }
    }
};
std::mutex snapWireMutex;
SnapWireHistory snapWireHistory;
SnapWireHistory ReadSnapWireHistory() {
    std::lock_guard lock(snapWireMutex);
    return snapWireHistory;
}
void ResetSnapWireHistory() {
    std::lock_guard lock(snapWireMutex);
    snapWireHistory = {};
}
// @SNAP_WIRE_END@

inline float AngleDelta(float a, float b) {
    return std::remainder(a - b, 360.0f);
}

inline float SnapTellyYaw(float yaw) {
    // Match Java Math.round, including negative half-way values.
    float wrapped = AngleDelta(yaw, 0.0f);
    if (wrapped >= 180.0f) wrapped -= 360.0f;
    return std::floor(wrapped / 45.0f + 0.5f) * 45.0f;
}

// Rise limits yaw and pitch independently before its RotationComponent stage.
// Nova's ordinary smoothing uses a shared vector budget, so compute this step
// here and leave only sensitivity correction to the shared rotation manager.
inline rotation::Rotation StepTellyRotation(const rotation::Rotation& current,
                                             const rotation::Rotation& target, float step) {
    return {current.yaw + std::clamp(AngleDelta(target.yaw, current.yaw), -step, step),
            std::clamp(current.pitch + std::clamp(target.pitch - current.pitch, -step, step),
                       -89.9f, 89.9f)};
}

// Rise findPlacementRotation/findPitchNear: four relative yaw candidates,
// seven pitches around 82, then rank the first valid pitch for each yaw.
template <typename Matches>
bool FindWatchdogPlacement(float cameraYaw, float movementYaw,
                          const rotation::Rotation& reference, bool diagonalOnly,
                          Matches&& matches, rotation::Rotation& result) {
    std::array<float, 4> yaws{};
    int count = 0;
    for (float yaw : {cameraYaw + 45.0f, cameraYaw + 90.0f, cameraYaw, movementYaw + 45.0f}) {
        const float snapped = SnapTellyYaw(yaw);
        if (diagonalOnly && std::fabs(std::remainder(snapped, 90.0f)) < 0.01f)
            continue;
        bool duplicate = false;
        for (int i = 0; i < count; ++i)
            duplicate |= std::fabs(AngleDelta(snapped, yaws[i])) < 0.1f;
        if (!duplicate)
            yaws[count++] = snapped;
    }
    bool found = false;
    float bestScore = std::numeric_limits<float>::max();
    for (int i = 0; i < count; ++i) {
        for (float offset : {0.0f, 1.5f, -1.5f, 3.0f, -3.0f, 4.5f, -4.5f}) {
            const rotation::Rotation candidate{yaws[i], std::clamp(82.0f + offset, -89.9f, 89.9f)};
            if (!matches(candidate))
                continue;
            const float yaw = AngleDelta(candidate.yaw, reference.yaw);
            const float pitch = candidate.pitch - reference.pitch;
            const float score = yaw * yaw + pitch * pitch +
                                std::fabs(AngleDelta(candidate.yaw, cameraYaw)) * 0.0005f;
            if (score < bestScore) {
                bestScore = score;
                result = candidate;
                found = true;
            }
            break;
        }
    }
    return found;
}

}

void ObserveLegacyScaffoldPacket(bool movement, bool hasLook, float yaw, float pitch) {
    std::lock_guard lock(snapWireMutex);
    snapWireHistory.Observe(movement, hasLook, yaw, pitch);
}
}

namespace nova::module {
namespace {

namespace event = nova::event;

// Direction's declaration order. rotation::BlockFace mirrors it and so does the
// face index the placement layer wants.
constexpr int kFaceDown = 0;
constexpr int kFaceUp = 1;

// The reach the original aims within, both for its own falling check and for the
// ray trace that validates a candidate angle.
constexpr double kReach = 4.5;

// How fast the aim is allowed to move when the module jumps straight to a chosen
// angle; the original writes the rotation field directly, which is the same as
// asking for an instant move.
constexpr double kInstantSpeed = 180.0;

// The speed above which the original gives up on normal placement and bursts.
constexpr double kBurstSpeed = 1.5;

// At most this many extra placement attempts inside one tick.
constexpr int kMaxBurst = 8;

// Append new choices so existing configs keep their Rise / Heypixel selection.
constexpr int kHypixelRotation = 2;

// Mirrored so KillAura and Velocity can ask without holding a reference to the
// module, the same way KillAura publishes whether it has targets.
std::atomic<bool> g_enabled{false};
std::atomic<float> g_fixedFov{0.0f};

[[nodiscard]] int FloorToInt(double value) { return static_cast<int>(std::floor(value)); }

[[nodiscard]] std::string WholeNumber(double value) { return std::format("{:.0f}", value); }

[[nodiscard]] std::string Hundredths(double value) { return std::format("{:.2f}", value); }

// MathUtils.getRandomInRange, over the same generator the rotation code uses.
[[nodiscard]] float RandomRange(float min, float max) {
    return min + static_cast<float>(rotation::Random()) * (max - min);
}

// RotationUtils.calculate(BlockPos, Direction): the centre of the named face, seen
// from the eye.
[[nodiscard]] rotation::Rotation CalculateFace(const java::BlockPos& pos, int facing,
                                               const rotation::Vec3& eye) {
    rotation::Vec3 target{pos.x + 0.5, pos.y + 0.5, pos.z + 0.5};
    target.x += static_cast<double>(java::FacingStepX(facing)) * 0.5;
    target.y += static_cast<double>(java::FacingStepY(facing)) * 0.5;
    target.z += static_cast<double>(java::FacingStepZ(facing)) * 0.5;
    return rotation::Calculate(eye, target);
}

// Rise Scaffold.getHitVec: uniform point on the chosen face, overridden by
// the actual ray intersection only when both the block and face match.
[[nodiscard]] rotation::Vec3 HitVector(const java::BlockPos& pos, int facing) {
    rotation::Vec3 hit{pos.x+rotation::Random(),pos.y+rotation::Random(),pos.z+rotation::Random()};
    switch (facing) {
    case 0: hit.y=pos.y; break;
    case 1: hit.y=pos.y+1; break;
    case 2: hit.z=pos.z; break;
    case 3: hit.z=pos.z+1; break;
    case 4: hit.x=pos.x; break;
    case 5: hit.x=pos.x+1; break;
    }
    const auto self=java::PlayerEntity();
    rotation::Vec3 eye,direction;
    java::BlockHitResultInfo trace;
    const auto angle=rotation::GetRotation();
    if (self.Valid() && self.EyePosition(1.f,eye) &&
        java::Vec3DirectionFromRotation(angle.pitch,angle.yaw,direction) &&
        java::WorldClip(eye,eye.Add(direction.x*kReach,direction.y*kReach,direction.z*kReach),self,trace) &&
        trace.kind==java::HitKind::Block && trace.hasBlockPos && trace.direction==facing &&
        trace.blockPos.x==pos.x && trace.blockPos.y==pos.y && trace.blockPos.z==pos.z &&
        std::isfinite(trace.location.x) && std::isfinite(trace.location.y) && std::isfinite(trace.location.z))
        hit=trace.location;
    return hit;
}

class ScaffoldModule final : public Module {
public:
    ScaffoldModule() : Module("Scaffold", Category::Player, "General", "General") {
        telly_ = &addBool("Telly", false);
        // Controls the sprint state used by the jump that follows this input.
        // It is intentionally available for both normal and Telly bridging.
        jumpSprint_ = &addBool("Jump Sprint", false);
        clutch_ = &addBool("Clutch", true);
        // Snap is a short placement turn, not a second Telly rotation mode.
        snap_ = &addBool("Snap", false, {}, [this] { return !telly_->get(); });
        degrees45_ = &addBool("45 Degrees", false);
        swing_ = &addBool("Swing", true);
        itemSpoof_ = &addBool("Item Spoof", true);
        keepFov_ = &addBool("Keep FOV", false);
        fixedFov_ = &addNumber("FOV", 30, 120, 1, 90, {}, WholeNumber,
                               [this] { return keepFov_->get(); });
        rotMod_ = &addMode("RotationMode", 0, {"Rise", "Heypixel", "Hypixel"});
        // The bounded normal-bridge path always validates the selected face.
        rayCast_ = &addMode("RayCast", 0, {"Normal", "Strict"}, {},
                            [this] { return !UseBoundedRotation() && !SnapActive(); });
        rotateSpeed_ = &addNumber("RotSpeed", 1, 180, 1, 127, {}, WholeNumber,
                                   [this] { return !SnapActive(); });
        rotateSpeed2_ = &addNumber("RotSpeed2", 1, 180, 1, 36, {}, WholeNumber,
                                   [this] { return rotMod_->get() == 1 && !SnapActive(); });
        rotateBackSpeed_ = &addNumber("BackSpeed", 1, 180, 1, 180, {}, WholeNumber,
                                      [this] { return TellyActive(); });
        tellyTick_ = &addNumber("TellyTicks", 0, 6, 1, 1, {}, WholeNumber,
                                [this] { return TellyActive(); });
        safeWalk_ = &addBool("SafeWalk", false, {}, [this] { return !TellyActive(); });
        edgeDist_ = &addNumber("Edge Dist", 0, 1, 0.01, 0.1, {}, Hundredths,
                               [this] { return safeWalk_->get() && !TellyActive(); });

        event::EventBus& bus = event::EventBus::Get();
        playerTick_ = bus.Subscribe<event::PlayerTickEvent>(
            [this](event::PlayerTickEvent& e) { OnPreTick(e); });
        // The highest priority, because this wants the input the player actually
        // asked for -- before the rotation bridge turns it into a movement fix.
        moveInput_ = bus.Subscribe<event::MoveInputEvent>(
            [this](event::MoveInputEvent& e) { OnMoveInput(e); }, event::Priority::Highest);
        respawn_ = bus.Subscribe<event::RespawnEvent>(
            [this](event::RespawnEvent&) {
                ResetSnap();
                scaffold::ResetSnapWireHistory();
                snapHasPlacedDelta_ = false;
                snapHasMotion_ = snapRotatedSincePlace_ = false;
                java::ReleaseScaffoldSlot();
                savedSlot_ = visibleSlot_ = -1;
            });
        snapMotion_ = bus.Subscribe<event::MotionEvent>(
            [this](event::MotionEvent& e) { OnSnapMotion(e); }, event::Priority::Lowest);
    }

private:
    [[nodiscard]] bool SnapActive() const {
        // Test the actual Telly option, even when Jump Sprint is disabled.
        return snap_->get() && !telly_->get();
    }

    [[nodiscard]] bool TellyActive() const {
        // Jump Sprint is the prerequisite for upward Telly. Turning it off
        // falls back to the normal scaffold path even when Telly remains selected.
        return telly_->get() && jumpSprint_->get();
    }

    [[nodiscard]] bool HypixelTelly() const {
        return TellyActive() && rotMod_->get() == kHypixelRotation;
    }

    [[nodiscard]] bool UseBoundedRotation() const {
        return rotMod_->get() == kHypixelRotation && !TellyActive();
    }

    // Starain's getSuffix: which of the three modes the module is in, shown beside
    // the name in the array list.
    [[nodiscard]] std::string suffix() const override {
        if (TellyActive())
            return "Telly";
        return SnapActive() ? "Snap" : "Normal";
    }

    void onUpdate() override {
        const double value = fixedFov_->get();
        g_fixedFov.store(keepFov_->get() && std::isfinite(value)
            ? static_cast<float>(std::clamp(value, 30.0, 120.0)) : 0.0f,
            std::memory_order_release);
    }

    void onEnable() override {
        ResetSnap();
        snapHasPlacedDelta_ = false;
        onUpdate();
        g_enabled.store(true, std::memory_order_release);

        ReleaseSlot();

        airTick_ = 0;
        tellyLayerValid_ = false;
        fallingCatch_ = false;
        hasBlockPos_ = false;
        enumFacing_ = -1;
        lastEnumFacing_ = -1;
        hasRotation_ = false;
        normalRotationActive_ = false;
        hypixelAimValid_ = false;
        tellyBoostPending_ = true;
        rotateCount_ = 0;
    }

    void onDisable() override {
        ResetSnap();
        snapHasPlacedDelta_ = false;
        g_enabled.store(false, std::memory_order_release);
        g_fixedFov.store(0.0f, std::memory_order_release);

        // The original restores the physical shift key rather than the option, so
        // that holding shift through the toggle keeps working.
        java::SetKeyShiftDown(nova::input::IsKeyDown(VK_SHIFT));

        ReleaseSlot();
        yLevel_ = 0;
        tellyBoostPending_ = true;
    }

    void ReleaseSlot() {
        java::ReleaseScaffoldSlot();
        int selected = -1;
        if (savedSlot_ >= 0 && java::SelectedSlot(selected) && selected == visibleSlot_)
            java::SetSelectedSlot(savedSlot_);
        savedSlot_ = visibleSlot_ = -1;
    }

    bool SelectSlot(int slot) {
        if (itemSpoof_->get()) {
            if (savedSlot_ >= 0) ReleaseSlot();
            return java::SetScaffoldSlot(slot);
        }
        java::ReleaseScaffoldSlot();
        int selected = -1;
        if (!java::SelectedSlot(selected)) return false;
        if (savedSlot_ < 0 || selected != visibleSlot_) savedSlot_ = selected;
        if (!java::SetSelectedSlot(slot)) return false;
        visibleSlot_ = slot;
        return true;
    }

    void OnPreTick(event::PlayerTickEvent& e) {
        if(automlg::IsWorking()){ResetSnap();ReleaseSlot();return;}
        if (!isEnabled())
            return;
        if (!BeginSnapTick()) return;

        const int slotId = java::FindPlaceableBlockSlot(1);
        if (slotId == -1) {
            ResetSnap();
            ReleaseSlot();
            return;
        }
        if (!SelectSlot(slotId)) {ResetSnap();return;}

        java::EntityHandle self = java::PlayerEntity();
        double playerY = 0.0;
        if (!self.Valid() || !self.GetY(playerY)) {
            ResetSnap();
            ReleaseSlot();
            return;
        }

        if (java::PlayerOnGround()) {
            airTick_ = 0;
            yLevel_ = FloorToInt(playerY) - 1;
        } else {
            ++airTick_;
        }

        if (!HypixelTelly()) {
            tellyLayerValid_ = false;
        } else if (java::PlayerOnGround() || !tellyLayerValid_) {
            yLevel_ = FloorToInt(playerY) - 1;
            tellyLayerValid_ = true;
        }

        GetBlockInfo();

        // Snap has exactly one normal interaction per tick. Never enter the
        // legacy clutch recursion or send additional movement/rotation packets.
        if (SnapActive()) {
            rotateCount_ = 0;
            HandleSnap();
            return;
        }

        // Falling faster than a whole block per tick, or moving faster than the
        // server will believe: either way the ordinary placement cannot keep up.
        bool reachable = true;
        rotation::Vec3 motion;
        if (java::PlayerDeltaMovement(motion)) {
            if (motion.y < -0.1 && hasBlockPos_) {
                rotation::FallingPlayer falling;
                if (rotation::MakeFallingPlayer(falling)) {
                    falling.Calculate(2);
                    if (static_cast<double>(blockPos_.y) > falling.y)
                        reachable = false;
                }
            }
        }
        const double strength = std::sqrt(motion.x * motion.x + motion.z * motion.z);

        int selected = 0;
        const bool holdingBlock =
            java::EffectiveSelectedSlot(selected) && java::HotbarSlotPlaceable(selected) && java::HotbarSlotCount(selected) > 1;

        if (clutch_->get() && (!reachable || strength >= kBurstSpeed) && rotateCount_ <= kMaxBurst &&
            java::CountPlaceableBlocks() >= 1 && holdingBlock) {
            if (UseBoundedRotation()) {
                // Rescue once with the effective angle. Re-entering the tick used
                // to advance airTick and send an unsmoothed angle up to nine times.
                rotateCount_ = 0;
                ComputeRotation();
                if (hypixelAimValid_) {
                    rotation::SetRotations(rotation_, kInstantSpeed);
                    Place(true);
                }
                return;
            }
            java::RequestPlayerTickSkip();
            ++rotateCount_;

            if (!hasBlockPos_) {
                OnPreTick(e);
                return;
            }

            const rotation::Rotation rotation = GetRotation();
            rotation::SetRotations(rotation, kInstantSpeed);
            java::SendMoveRotation(rotation.yaw, rotation.pitch, java::PlayerOnGround());

            const bool consumed =
                java::UseItemOnBlock(blockPos_, enumFacing_, HitVector(blockPos_, enumFacing_));
            if (consumed)
                java::SendSwingPacket();

            OnPreTick(e);
            return;
        }

        rotateCount_ = 0;

        if (!HypixelTelly())
            tellyBoostPending_ = true;
        if (TellyActive())
            HandleTelly();
        else
            HandleNormal();
    }

    void OnMoveInput(event::MoveInputEvent& e) {
        if(automlg::IsWorking())return;
        if (!isEnabled())
            return;

        const bool wasMoving = InputMoving();
        forwardInput_ = e.forward;
        strafeInput_ = e.strafe;

        float playerYaw = 0.0f;
        float playerPitch = 0.0f;
        if (!java::PlayerRotation(playerYaw, playerPitch))
            return;

        // The direction the player is asking to walk in, quantised to 45 degrees
        // for the aim but kept raw for the return swing.
        // With no input atan2(0, 0) would replace the last travel direction
        // with the camera yaw just as Telly needs its final placement turn.
        if (InputMoving() || !HypixelTelly()) {
            const double inputAngle =
                std::atan2(-static_cast<double>(strafeInput_), static_cast<double>(forwardInput_));
            inputYaw_ = rotation::mth::WrapDegrees(
                static_cast<float>(std::round((playerYaw + static_cast<float>(inputAngle * rotation::kToDegrees)) /
                                              45.0f) *
                                   45.0f));
            rawInputYaw_ = rotation::mth::WrapDegrees(
                static_cast<float>(std::round(playerYaw + static_cast<float>(inputAngle * rotation::kToDegrees))));
        }

        // PreTick already ran using the previous input. Finish the edge block
        // from this fresh release, before vanilla applies residual movement.
        if (HypixelTelly() && wasMoving && !InputMoving() && OnAir() &&
            !(rotation::State().active &&
              rotation::State().priority > rotation::RotationPriority::Medium)) {
            GetBlockInfo();
            if (hasBlockPos_ && enumFacing_ >= 0) {
                rotation::SetSmoothed(false);
                HandleTelly();
            }
        }

        if (java::PlayerOnGround() && !java::KeyJumpDown() && InputMoving() && TellyActive())
            e.jump = true;

        // PreTick sees the previous movement input. On an idle-to-forward start,
        // direction change or landing, its return angle can therefore be stale.
        // Align from this input before RotationBridge corrects it and vanilla
        // checks sprint/jump; merely setting sprint cannot fix a backward input.
        if (HypixelTelly() && java::PlayerOnGround() && e.jump &&
            e.forward > 0.001f && !e.sneak &&
            !(rotation::State().active &&
              rotation::State().priority > rotation::RotationPriority::Medium)) {
            int selected = 0;
            if (java::EffectiveSelectedSlot(selected) && java::HotbarSlotPlaceable(selected) && java::HotbarSlotCount(selected) > 1) {
                rotation_ = {rawInputYaw_, 85.0f};
                hasRotation_ = true;
                tellyBoostPending_ = true;
                // PreTick may already have smoothed an obsolete target this tick.
                // Re-evaluate the takeoff target, retaining shared sensitivity
                // correction. BackSpeed must not delay the sprint-jump direction.
                rotation::SetSmoothed(false);
                rotation::SetRotations(rotation_, 360.0);
            }
        }

        // In 1.8 the sprint flag is sampled when EntityPlayerSP performs the
        // jump. Set it on the input event, before that update runs, so the
        // setting affects both a manually requested jump and Telly's synthetic
        // jump. A sprint jump specifically means a forward jump, so strafe-only,
        // backwards and stationary jumps do not consume this setting. Releasing
        // sprint here also makes the disabled state deterministic when the Sprint
        // module or the physical key is held.
        if (java::PlayerOnGround() && e.jump && e.forward > 0.001f)
            java::SetPlayerSprinting(jumpSprint_->get());

        if (java::PlayerOnGround() && IsOnBlockEdge(static_cast<float>(edgeDist_->get())) &&
            safeWalk_->get() && !TellyActive()) {
            e.sneak = true;
        }
    }

    void HandleTelly() {
        normalRotationActive_ = false;
        if (HypixelTelly() && (!fallingCatch_ || !InputMoving())) {
            HandleWatchdogTelly();
            return;
        }
        if (fallingCatch_ && rotation::State().active &&
            rotation::State().priority > rotation::RotationPriority::Medium)
            return;
        if (java::PlayerOnGround() && (strafeInput_ != 0.0f || forwardInput_ != 0.0f)) {
            java::PlayerRotation(rotation_.yaw, rotation_.pitch);
            float pitch = hasRotation_ ? rotation_.pitch : java::PlayerPitch();
            rotation::SetRotations(rotation::Rotation{rawInputYaw_, pitch},
                                   rotateBackSpeed_->get());
            return;
        }

        ComputeRotation();
        double speed = rotateSpeed_->get();
        if (rotMod_->get() == 1)
            speed = airTick_ == 1 ? rotateSpeed_->get() : rotateSpeed2_->get();
        // Falling uses the same angle search and placement path as Rise. Only
        // refresh stale preparation and remove the airborne preparation wait.
        if (fallingCatch_)
            rotation::SetSmoothed(false);
        rotation::SetRotations(rotation_, speed);

        if (airTick_ > static_cast<int>(tellyTick_->get()) || fallingCatch_)
            Place();
    }

    void HandleWatchdogTelly() {
        // Do not consume the one-shot return boost if another module owns rotations.
        if (rotation::State().active &&
            rotation::State().priority > rotation::RotationPriority::Medium)
            return;
        float cameraYaw = 0.0f, cameraPitch = 0.0f;
        if (!java::PlayerRotation(cameraYaw, cameraPitch))
            return;
        const bool grounded = java::PlayerOnGround();
        const bool stopping = !InputMoving();
        if (grounded)
            tellyBoostPending_ = true;

        // Nova applies the placement yaw to both movement correction and the
        // sprint-jump impulse. Always face travel at takeoff, including diagonal
        // ascent; retaining a rearward placement yaw turns forward into back/side
        // input and removes or reverses the sprint boost. Aim after leaving ground.
        if (grounded && InputMoving()) {
            rotation_ = {rawInputYaw_, 85.0f};
            hasRotation_ = true;
            const auto step = scaffold::StepTellyRotation(rotation::GetRotation(), rotation_,
                                                         static_cast<float>(rotateBackSpeed_->get()));
            rotation::SetRotations(step, 360.0);
            return;
        }

        // Preserve takeoff preparation even while the placement target is absent.
        if (!hasRotation_) {
            rotation_ = {rotation::mth::WrapDegrees(cameraYaw - 135.0f), 82.0f};
            hasRotation_ = true;
        }
        ComputeRotation();
        const bool hasTarget = hasBlockPos_ && enumFacing_ >= 0;
        const bool targetReady = hasTarget && (rayCast_->get() == 1
            ? rotation::OverBlock(rotation_, blockPos_, static_cast<rotation::BlockFace>(enumFacing_))
            : rotation::OverBlock(rotation_, blockPos_));
        rotation::Vec3 motion;
        const bool falling = !grounded && airTick_ > 3 &&
                             java::PlayerDeltaMovement(motion) && motion.y < -0.1;
        const bool boost = !grounded && targetReady && tellyBoostPending_;
        float speed = boost ? RandomRange(122.0f, 128.0f) : RandomRange(35.0f, 38.0f);
        // Keep the default fast/slow cadence; RotSpeed scales both phases.
        speed = std::clamp(speed * static_cast<float>(rotateSpeed_->get() / 127.0), 1.0f, 180.0f);
        if (!targetReady || (falling && !clutch_->get()))
            speed = std::max(speed, 127.0f);
        // Stopping has no takeoff rotation to preserve. Complete the final
        // turn now instead of spending the remaining landing window waiting.
        if (stopping && targetReady)
            speed = 180.0f;
        const auto step = scaffold::StepTellyRotation(rotation::GetRotation(), rotation_, speed);
        rotation::SetRotations(step, 360.0);
        // A valid target is not proof that the smoothed rotation reached it.
        // Retain the boost until the effective ray is ready for placement.
        const auto effective = rotation::GetRotation();
        const bool reached = targetReady && (rayCast_->get() == 1
            ? rotation::OverBlock(effective, blockPos_, static_cast<rotation::BlockFace>(enumFacing_))
            : rotation::OverBlock(effective, blockPos_));
        if (boost && reached)
            tellyBoostPending_ = false;

        // Like Rise's falling path, stop waiting when descending. Place still
        // checks the current effective ray; this is one ordinary attempt per tick.
        // onGround can still describe the previous movement at an edge. An
        // idle Telly must finish an available block even on that last tick.
        if (stopping || (!grounded && (airTick_ > static_cast<int>(tellyTick_->get()) || falling)))
            Place();
    }

    void HandleNormal() {
        if (SnapActive()) {HandleSnap();return;}
        if (UseBoundedRotation()) {
            // Rise onEnable seeds the aim before a placement ray exists. Do the
            // same when switching from Telly to Normal while still enabled.
            if (!normalRotationActive_) {
                float yaw = 0.0f, pitch = 0.0f;
                if (!java::PlayerRotation(yaw, pitch))
                    return;
                rotation_ = {rotation::mth::WrapDegrees(yaw - 180.0f), 90.0f};
                hasRotation_ = true;
                normalRotationActive_ = true;
            }
            hypixelAimValid_ = false;
            if (OnAir())
                ComputeRotation();
            // Normal keeps aiming on supported ticks and failed placement rays.
            // Snap is an explicitly selected branch, never an implicit fallback.
            rotation::SetRotations(rotation_, rotateSpeed_->get());
            Place();
            return;
        }
        normalRotationActive_ = false;
        ComputeRotation();
        if (!UseBoundedRotation() || hypixelAimValid_)
            rotation::SetRotations(rotation_, rotateSpeed_->get());

        Place();
    }

    void ResetSnap() {
        if (rotation::OwnsExclusiveRotations(this)) {
            float yaw = 0, pitch = 0;
            // Restore the LIVE camera, not an angle saved before mouse movement.
            // This updates the shared movement fix too; no packet is sent here.
            if (java::PlayerRotation(yaw, pitch))
                rotation::SetExclusiveRotations(this, {yaw, pitch});
            rotation::ReleaseExclusiveRotations(this);
        }
        snapWaiting_ = snapMotionComplete_ = false;
        snapPlacementPending_ = false;
        snapHeldTicks_ = 0;
    }

    bool BeginSnapTick() {
        if (!SnapActive()) {ResetSnap();snapHasPlacedDelta_=false;return true;}
        if (!snapWaiting_) return true;
        if (!rotation::OwnsExclusiveRotations(this)) {
            ResetSnap();return true;
        }
        if (snapMotionComplete_) {ResetSnap();return true;}
        // A cancelled/skipped walking update must not leave an eternal turn or
        // repeat placement. Retry only after releasing this abandoned cycle.
        if (++snapHeldTicks_ >= 2) {ResetSnap();return false;}
        return false;
    }

    bool SyncSnapPacketHistory() {
        const auto wire = scaffold::ReadSnapWireHistory();
        if (!wire.available) return false;
        snapHasMotion_ = wire.hasLook;
        snapLastMotion_ = {wire.yaw, wire.pitch};
        snapRotatedSincePlace_ = wire.rotated;
        snapLatestDelta_ = wire.delta;
        snapHasPlacedDelta_ = wire.hasPlaced;
        snapPlacedDelta_ = wire.placedDelta;
        snapPlacementPending_ = wire.pendingPlace;
        return true;
    }

    void OnSnapMotion(event::MotionEvent& e) {
        if (e.type != event::EventType::Post || !std::isfinite(e.yaw) || !std::isfinite(e.pitch)) return;
        if (!SyncSnapPacketHistory()) {
            // Fallback for profiles without legacy dispatch observations.
            // Grim drains queued placements BEFORE onRotationUpdate.
            if (snapPlacementPending_) {
                if (snapRotatedSincePlace_) {
                    snapPlacedDelta_ = snapLatestDelta_;
                    snapHasPlacedDelta_ = true;
                    snapRotatedSincePlace_ = false;
                }
                snapPlacementPending_ = false;
            }
            const auto before = snapHasMotion_ ? snapLastMotion_ : rotation::Rotation{snapBaseYaw_, snapBasePitch_};
            if (e.yaw != before.yaw || e.pitch != before.pitch) {
                snapLatestDelta_ = std::fabs(e.yaw - before.yaw);
                snapRotatedSincePlace_ = true;
            }
            snapLastMotion_ = {e.yaw, e.pitch};
            snapHasMotion_ = true;
        }
        if (!snapWaiting_ || snapMotionComplete_ || !rotation::OwnsExclusiveRotations(this)) return;
        // Post is a local walking-update boundary, not a server ACK. Keep the
        // placement pitch through that boundary (Grim RotationPlace post check).
        if (std::fabs(scaffold::AngleDelta(e.yaw, snapRotation_.yaw)) < .01f &&
            std::fabs(e.pitch - snapRotation_.pitch) < .01f) {
            snapMotionComplete_ = true;
        }
    }

    bool FindSnapRotation(const java::EntityHandle& self, const rotation::Vec3& eye,
                          const rotation::Rotation& previous, double sensitivity,
                          rotation::Rotation& selected) {
        float cameraYaw = 0, cameraPitch = 0;
        if (!java::PlayerRotation(cameraYaw, cameraPitch)) return false;
        const bool moving = InputMoving();
        const float travelYaw = rotation::Direction(forwardInput_, strafeInput_, cameraYaw);
        double bestScore = std::numeric_limits<double>::max();
        bool found = false;
        auto consider = [&](rotation::Rotation target) {
            target = rotation::RelateToPlayerRotation(target, previous);
            const auto angle = rotation::ApplySensitivityPatch(target, previous, sensitivity);
            const auto base = snapHasMotion_ ? snapLastMotion_ : previous;
            const bool changesLook = angle.yaw != base.yaw || angle.pitch != base.pitch;
            const float delta = changesLook ? std::fabs(angle.yaw - base.yaw) : snapLatestDelta_;
            // This turn supplies the check's state for the NEXT placement.
            const bool hasConsumed = snapRotatedSincePlace_ || snapHasPlacedDelta_;
            const float consumed = snapRotatedSincePlace_ ? snapLatestDelta_ : snapPlacedDelta_;
            const bool repeatsNext = hasConsumed && changesLook && delta > 2.f &&
                std::fabs(delta - consumed) < .0001f;
            if (degrees45_->get() && std::fabs(std::remainder(angle.yaw, 90.f)) < .01f) return;
            java::BlockHitResultInfo hit;
            if (!TracePlacement(angle, self, eye, hit)) return;
            // Prefer an alternate ray, but do not discard the only valid face
            // because a FUTURE placement might require a preparation tick.
            double score = repeatsNext ? 1000000.0 : 0.0;
            if (moving) {
                rotation::MoveInput input{forwardInput_, strafeInput_};
                rotation::FixMovement(input, angle.yaw, cameraYaw);
                const double error = scaffold::AngleDelta(
                    rotation::Direction(input.forward, input.strafe, angle.yaw), travelYaw);
                // Digital movement correction otherwise rounds by as much as
                // 22.5 degrees. Prefer a ray that preserves world travel first.
                score += error * error;
                // Among equally aligned rays, keep forward input when geometry
                // permits. Grim loopVectors excludes backward/side-only sprint.
                if (forwardInput_ >= .8f && input.forward < .8f) score += .01;
            }
            const double yaw = scaffold::AngleDelta(angle.yaw, cameraYaw);
            const double pitch = angle.pitch - cameraPitch;
            score += (yaw * yaw + pitch * pitch) * 1e-8;
            if (score < bestScore) {bestScore = score;selected = angle;found = true;}
        };
        const auto center = HitVectorForSnap();
        constexpr double offsets[]{0., -.3, .3};
        for (double a : offsets) for (double b : offsets) {
            auto point = center;
            if (enumFacing_ <= 1) {point.x += a;point.z += b;}
            else if (enumFacing_ <= 3) {point.x += a;point.y += b;}
            else {point.y += a;point.z += b;}
            const auto geometric = rotation::Calculate(eye, point);
            // Keep every former candidate as a fallback. Additional rays align
            // with the eight actual movement directions; they must hit too.
            consider(geometric);
            if (moving) {
                const float aligned = travelYaw + std::round(
                    scaffold::AngleDelta(geometric.yaw, travelYaw) / 45.f) * 45.f;
                consider({aligned, geometric.pitch});
                consider({cameraYaw, geometric.pitch});
            }
        }
        return found;
    }

    void HandleSnap() {
        normalRotationActive_ = false;
        const bool observedPackets = SyncSnapPacketHistory();
        if (!SnapActive() || snapWaiting_ || automlg::IsWorking() ||
            !hasBlockPos_ || enumFacing_ < 0) return;
        const bool supported = !OnAir();
        if (supported) {
            // Prepare/place BEFORE the center leaves its support. Waiting for
            // an empty center column leaves less than one tick at sprint speed,
            // while a repeated stored delta may require a normal look tick.
            rotation::Vec3 position, velocity;
            if (!java::PlayerOnGround() || !InputMoving() || java::KeyJumpDown() ||
                !java::PlayerEntity().Position(position) || !java::PlayerDeltaMovement(velocity) ||
                !std::isfinite(velocity.x) || !std::isfinite(velocity.z)) return;
            const double horizontal = std::hypot(velocity.x, velocity.z);
            if (horizontal < .01) return;
            const double lead = std::min(2.0, .7 / horizontal);
            const auto next = position.Add(velocity.x * lead, 0, velocity.z * lead);
            if (!java::BlockStateReplaceable({FloorToInt(next.x), FloorToInt(position.y)-1, FloorToInt(next.z)})) return;
            GetBlockInfo(&next);
            if (!hasBlockPos_ || enumFacing_ < 0) return;
        }
        if (rotation::State().active && rotation::State().priority > rotation::RotationPriority::Medium) return;
        int slot = -1;
        if (!java::EffectiveSelectedSlot(slot) || !java::HotbarSlotPlaceable(slot) ||
            java::HotbarSlotCount(slot) <= 1) return;
        const auto self = java::PlayerEntity();
        rotation::Vec3 eye;
        if (!self.Valid() || !self.EyePosition(1.f, eye)) return;
        const auto& state = rotation::State();
        const auto previous = state.hasLastRotations ? state.lastRotations : rotation::GetRotation();
        double sensitivity = 0;
        if (!java::PlayerSensitivity(sensitivity) || !std::isfinite(sensitivity)) return;
        rotation::Rotation angle;
        const bool validRay = FindSnapRotation(self, eye, previous, sensitivity, angle);
        if (!validRay) {
            if (!supported) return;
            // A side face is not visible until we cross its plane. Orient on
            // the last supported ticks without attempting an occluded placement.
            angle = rotation::ApplySensitivityPatch(rotation::RelateToPlayerRotation(
                rotation::Calculate(eye, HitVectorForSnap()), previous), previous, sensitivity);
        }
        if (!rotation::SetExclusiveRotations(this, angle)) return;
        java::BlockHitResultInfo hit;
        // Revalidate the actual manager angle, including yaw normalization.
        if (validRay && !TracePlacement(rotation::GetRotation(), self, eye, hit)) {ResetSnap();return;}
        snapRotation_ = rotation::GetRotation();
        snapBaseYaw_ = previous.yaw;
        snapBasePitch_ = previous.pitch;
        snapWaiting_ = true;
        snapMotionComplete_ = false;
        snapHeldTicks_ = 0;
        rotation_ = snapRotation_;hasRotation_ = true;
        if (!validRay) return;
        // Changing the upcoming yaw cannot repair a duplicate already stored in
        // the server's rotation processor. Advance one ordinary look tick first.
        if (snapHasPlacedDelta_ && snapRotatedSincePlace_ && snapLatestDelta_ > 2.f &&
            std::fabs(snapLatestDelta_ - snapPlacedDelta_) < .0001f) return;
        // Lunar's cancellable right-click hook can return without sending C08.
        // Its dispatch observer, not the interaction return value, owns history.
        if (!observedPackets) snapPlacementPending_ = true;
        const bool consumed = java::UseItemOnBlock(hit.blockPos, hit.direction, hit.location);
        // Even a false interaction may have sent a packet. Retain its angle
        // for the walking update instead of immediately restoring the view.
        if (consumed) {
            if (swing_->get()) java::SwingMainHand();
            else java::SendSwingPacket();
        }
    }

    rotation::Vec3 HitVectorForSnap() const {
        return {blockPos_.x + .5 + java::FacingStepX(enumFacing_) * .5,
                blockPos_.y + .5 + java::FacingStepY(enumFacing_) * .5,
                blockPos_.z + .5 + java::FacingStepZ(enumFacing_) * .5};
    }

    // Starain calls getRotation() here, not the bare face calculation: the whole
    // point of that method is to search for an angle that raycasts onto the block,
    // and the geometric centre is only its last resort. Using the centre directly
    // -- which this did -- throws the search away and aims dead at the face.
    void ComputeRotation() {
        if(automlg::IsWorking())return;
        if (UseBoundedRotation()) {
            ComputeHypixelRotation();
            return;
        }
        // Telly must start its fast airborne return even before the target face
        // exists. Placement validity is checked separately by Place().
        if ((!hasBlockPos_ || enumFacing_ < 0) && !HypixelTelly())
            return;

        rotation_ = GetRotation();
        hasRotation_ = true;
    }

    bool TracePlacement(const rotation::Rotation& angle, const java::EntityHandle& self,
                        const rotation::Vec3& eye, java::BlockHitResultInfo& hit) const {
        rotation::Vec3 direction;
        if (!java::Vec3DirectionFromRotation(angle.pitch, angle.yaw, direction) ||
            !java::WorldClip(eye, eye.Add(direction.x * kReach, direction.y * kReach,
                                        direction.z * kReach), self, hit))
            return false;
        return hit.kind == java::HitKind::Block && hit.hasBlockPos &&
               hit.blockPos.x == blockPos_.x && hit.blockPos.y == blockPos_.y &&
               hit.blockPos.z == blockPos_.z && hit.direction == enumFacing_ &&
               std::isfinite(hit.location.x) && std::isfinite(hit.location.y) &&
               std::isfinite(hit.location.z);
    }

    void ComputeHypixelRotation() {
        hypixelAimValid_ = false;
        if (!hasBlockPos_ || enumFacing_ < 0)
            return;
        const java::EntityHandle self = java::PlayerEntity();
        rotation::Vec3 eye, player;
        float cameraYaw = 0.0f, cameraPitch = 0.0f;
        if (!self.Valid() || !self.EyePosition(1.0f, eye) || !self.Position(player) ||
            !java::PlayerRotation(cameraYaw, cameraPitch))
            return;

        const auto current = rotation::GetRotation();
        java::BlockHitResultInfo hit;
        // Rise Normal only updates its target when the effective ray misses.
        if (TracePlacement(current, self, eye, hit)) {
            if (!hasRotation_)
                rotation_ = current;
            hasRotation_ = true;
            hypixelAimValid_ = true;
            return;
        }

        // Rise D(0): cast horizontally at the target's fractional height, then
        // aim from the real eye at each intersection. Use an explicit ray origin
        // rather than temporarily moving the live player as the Java code does.
        const double targetY = blockPos_.y + java::FacingStepY(enumFacing_) +
                               player.y - std::floor(player.y);
        const rotation::Vec3 origin{eye.x, targetY + 0.5 + (rotation::Random() - 0.5) * 0.1, eye.z};
        const float wrapped = rotation::mth::WrapDegrees(cameraYaw);
        const float cardinalDistance = std::fabs(std::fmod(wrapped, 90.0f));
        const bool cardinal = cardinalDistance <= 10.0f || cardinalDistance >= 80.0f;
        const auto allowed = [&](const rotation::Rotation& angle) {
            return !degrees45_->get() || std::fabs(std::remainder(angle.yaw, 90.0f)) > 0.01f;
        };
        bool found = false;
        double bestScore = 0.0;
        rotation::Rotation selected;
        for (int offset = cardinal ? -135 : -180; offset <= (cardinal ? 135 : 180); offset += 45) {
            rotation::Vec3 direction;
            java::BlockHitResultInfo horizontal;
            if (!java::Vec3DirectionFromRotation(0.0f, cameraYaw + offset * 3.0f, direction) ||
                !java::WorldClip(origin, origin.Add(direction.x * 5.5, 0.0, direction.z * 5.5), self, horizontal) ||
                horizontal.kind != java::HitKind::Block || !horizontal.hasBlockPos ||
                !std::isfinite(horizontal.location.x) || !std::isfinite(horizontal.location.y) ||
                !std::isfinite(horizontal.location.z))
                continue;
            const auto candidate = rotation::Calculate(eye, horizontal.location);
            if (!allowed(candidate) || !TracePlacement(candidate, self, eye, hit))
                continue;
            const double yaw = rotation::mth::WrapDegrees(candidate.yaw - current.yaw);
            const double pitch = candidate.pitch - current.pitch;
            const double score = yaw * yaw + pitch * pitch;
            if (!found || score < bestScore) {
                found = true;
                bestScore = score;
                selected = candidate;
            }
        }
        if (found) {
            rotation_ = selected;
        } else if (!hasRotation_ || !TracePlacement(rotation_, self, eye, hit)) {
            // Rise prepares the geometric face angle even before it is hittable.
            // Keeping rotation and permission to place separate avoids Snap behaviour.
            const auto fallback = CalculateFace(blockPos_, enumFacing_, eye);
            if (allowed(fallback))
                rotation_ = fallback;
            else
                return;
        }
        hasRotation_ = true;
        hypixelAimValid_ = TracePlacement(rotation_, self, eye, hit);
    }

    // Starain's getRotation: try eight fixed yaws, ordered by how close each is to
    // the direction the player is walking (so the aim looks like mouse movement),
    // and for each one look for any pitch that actually lands on the block.
    rotation::Rotation GetRotation() {
        float playerYaw = 0.0f;
        float playerPitch = 0.0f;
        if (!java::PlayerRotation(playerYaw, playerPitch))
            return rotation_;

        if (!hasRotation_ && !fallingCatch_)
            return rotation::Rotation{rotation::mth::WrapDegrees(playerYaw - 135.0f), 82.0f};

        if (!OnAir() || !hasBlockPos_ || enumFacing_ < 0)
            return rotation_;

        java::EntityHandle self = java::PlayerEntity();
        rotation::Vec3 eye;
        if (!self.Valid() || !self.EyePosition(1.0f, eye))
            return rotation_;

        const rotation::Rotation calculated = CalculateFace(blockPos_, enumFacing_, eye);

        float yaws[8] = {-135.0f, -90.0f, -45.0f, 0.0f, 45.0f, 90.0f, 135.0f, 180.0f};
        const float baseYaw = rotation::mth::WrapDegrees(inputYaw_ - 180.0f);
        for (int i = 1; i < 8; ++i) {
            const float key = yaws[i];
            int j = i - 1;
            while (j >= 0 && std::fabs(rotation::mth::WrapDegrees(baseYaw - yaws[j])) >
                                std::fabs(rotation::mth::WrapDegrees(baseYaw - key))) {
                yaws[j + 1] = yaws[j];
                --j;
            }
            yaws[j + 1] = key;
        }

        const float pitches[3] = {75.0f, 82.0f, 87.0f};
        const bool strict = rayCast_->get() == 1;

        auto matches = [&](const rotation::Rotation& candidate) {
            return strict ? rotation::OverBlock(candidate, blockPos_,
                                                static_cast<rotation::BlockFace>(enumFacing_))
                          : rotation::OverBlock(candidate, blockPos_);
        };

        if (HypixelTelly() && !fallingCatch_) {
            // Keep an effective angle while it still hits, then score Rise's
            // relative candidates instead of choosing the first world-fixed yaw.
            const bool allowed = !degrees45_->get() ||
                std::fabs(std::remainder(rotation_.yaw, 90.0f)) > 0.01f;
            if (allowed && matches(rotation_))
                return rotation_;
            rotation::Rotation selected;
            if (scaffold::FindWatchdogPlacement(playerYaw, rawInputYaw_, rotation_,
                                                degrees45_->get(), matches, selected))
                return selected;
            // Geometric fallback also prepares the turn; Place separately checks
            // the effective angle, so a failed search cannot place blindly.
            return calculated;
        }

        for (int yi = 0; yi <= 8; ++yi) {
            const float yaw = yi < 8 ? yaws[yi] : calculated.yaw;
            if (degrees45_->get() && std::fmod(yaw, 90.0f) == 0.0f)
                continue;

            for (float pitch : pitches) {
                const rotation::Rotation candidate{
                    yaw + RandomRange(-0.3f, 0.3f), pitch + RandomRange(-0.3f, 0.3f)};
                if (matches(candidate))
                    return candidate;
            }

            for (int pitch = -90; pitch < 90; ++pitch) {
                const rotation::Rotation candidate{yaw, static_cast<float>(pitch)};
                if (matches(candidate))
                    return candidate;
            }
        }

        return calculated;
    }

    void Place(bool sendRotation = false) {
        if(automlg::IsWorking())return;
        if (!OnAir() || !hasBlockPos_ || enumFacing_ < 0)
            return;

        int selected = 0;
        if (!java::EffectiveSelectedSlot(selected) || !java::HotbarSlotPlaceable(selected) || java::HotbarSlotCount(selected) <= 1)
            return;

        const rotation::Rotation current = rotation::GetRotation();
        if (UseBoundedRotation()) {
            const java::EntityHandle self = java::PlayerEntity();
            rotation::Vec3 eye;
            java::BlockHitResultInfo hit;
            if (!hypixelAimValid_ || !self.Valid() || !self.EyePosition(1.0f, eye) ||
                !TracePlacement(current, self, eye, hit))
                return;
            if (sendRotation)
                java::SendMoveRotation(current.yaw, current.pitch, java::PlayerOnGround());
            if (java::UseItemOnBlock(hit.blockPos, hit.direction, hit.location)) {
                if (swing_->get()) java::SwingMainHand();
                else java::SendSwingPacket();
            }
            return;
        }
        if (rayCast_->get() == 1) {
            if (!rotation::OverBlock(current, blockPos_,
                                     static_cast<rotation::BlockFace>(enumFacing_)))
                return;
        } else {
            if (!rotation::OverBlock(current, blockPos_))
                return;
        }

        const bool consumed =
            java::UseItemOnBlock(blockPos_, enumFacing_, HitVector(blockPos_, enumFacing_));
        // The original swings the hand here, not just sends the packet: the arm
        // animation is visible to the player and to anyone watching. The burst path
        // above keeps the packet-only form, which is what the original does there.
        if (consumed) {
            if (swing_->get()) java::SwingMainHand();
            else java::SendSwingPacket();
        }
    }

    // Rise PlayerUtil.a: generate candidates from six neighbours of every
    // permitted solid origin, then stable-sort by distance to the desired foot.
    void GetBlockInfo(const rotation::Vec3* targetPosition = nullptr) {
        hasBlockPos_ = false;
        enumFacing_ = -1;
        fallingCatch_ = false;
        const auto self = java::PlayerEntity();
        rotation::Vec3 player;
        if (!self.Valid() || !self.Position(player)) return;
        rotation::Vec3 desired=player;
        if (targetPosition) {
            desired=*targetPosition;
            // Rise offsets the desired foot and scan radius, not the lattice
            // origin or the player's five-block candidate distance check.
            if (rotMod_->get()!=kHypixelRotation) player=desired;
        }
        const auto offset=desired.Subtract(player);
        const int searchRadius=static_cast<int>(5.0+std::abs(offset.x)+std::abs(offset.z));

        rotation::Vec3 fallMotion;
        // Do not wait for a takeoff-height threshold: enabling in mid-fall or
        // reaching a side ledge can leave only this tick to place. In this phase
        // use the same Rise candidates. This flag controls rotation timing only.
        fallingCatch_ = HypixelTelly() && !clutch_->get() && !java::PlayerOnGround() &&
            java::PlayerDeltaMovement(fallMotion) && fallMotion.y < -0.1;

        // Rise's Same Y defaults to Off; Telly alone does not constrain search
        // height. Preserve Nova's existing height policy in the other modes.
        std::optional<int> fixedStartY;
        if (rotMod_->get() != kHypixelRotation &&
            !java::KeyJumpDown() && InputMoving() && TellyActive() &&
            std::fabs(double(yLevel_ - (FloorToInt(player.y)-1))) <= 1.25)
            fixedStartY = GetYLevel()+1;

        // Cache repeated neighbour reads within this search. Each origin is
        // visited once, so the blacklist needs no separate cache or cell type.
        std::map<std::tuple<int, int, int>, bool> replaceable;
        auto read = [&](int x, int y, int z) {
            const auto key = std::tuple{x, y, z};
            auto it = replaceable.find(key);
            if (it == replaceable.end())
                it = replaceable.emplace(key, java::BlockStateReplaceable({x, y, z})).first;
            return it->second;
        };
        std::vector<rotation::Vec3> candidates;
        for (int x=-searchRadius;x<=searchRadius;++x) for (int y=-searchRadius;y<=searchRadius;++y) for (int z=-searchRadius;z<=searchRadius;++z) {
            const rotation::Vec3 at{player.x+x,player.y+y,player.z+z};
            const int bx=FloorToInt(at.x),by=FloorToInt(at.y),bz=FloorToInt(at.z);
            if (read(bx, by, bz) || java::BlockStateScaffoldExcluded({bx, by, bz}))
                continue;
            // Rise order: x-, x+, y-, y+, z-, z+. Keep fractional coordinates.
            for (int facing : {4,5,0,1,2,3})
                candidates.push_back(at.Add(java::FacingStepX(facing),java::FacingStepY(facing),java::FacingStepZ(facing)));
        }
        std::erase_if(candidates,[&](const rotation::Vec3& p) {
            const auto d=p.Subtract(player);
            // Entity.getDistance and MathHelper.sqrt_double return float in 1.8.
            return static_cast<float>(std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z))>5.0f ||
                !read(FloorToInt(p.x),FloorToInt(p.y),FloorToInt(p.z)) ||
                (fixedStartY && std::floor(p.y+1.0)!=*fixedStartY);
        });
        auto distance=[&](const rotation::Vec3& p) {
            const double x=desired.x-p.x,y=desired.y-1.0-p.y,z=desired.z-p.z;
            return static_cast<float>(std::sqrt(x*x+y*y+z*z));
        };
        std::stable_sort(candidates.begin(),candidates.end(),[&](const auto& a,const auto& b){return distance(a)<distance(b);});
        if (candidates.empty()) return;
        const auto target=candidates.front();
        const java::BlockPos pos{FloorToInt(target.x),FloorToInt(target.y),FloorToInt(target.z)};
        const float yaw=rotation::GetRotation().yaw;
        auto score=[&](int neighbour) {
            const double degrees=std::fmod(std::atan2(double(java::FacingStepZ(neighbour)),double(java::FacingStepX(neighbour)))*rotation::kToDegrees,360.0);
            const double direction=std::fmod(yaw,360.f)+90.f;
            // Literal MathUtil.n from this Rise source, including signed terms.
            return std::abs(std::min(std::abs(degrees-direction),std::min(
                std::abs(degrees-360.0)-std::abs(direction),std::abs(direction-360.0)-std::abs(degrees))));
        };
        std::vector<int> horizontal;
        for (int neighbour : {2,3,4,5})
            if (!read(pos.x+java::FacingStepX(neighbour),pos.y,pos.z+java::FacingStepZ(neighbour)))
                horizontal.push_back(neighbour);
        std::stable_sort(horizontal.begin(),horizontal.end(),[&](int a,int b){return score(a)<score(b);});
        if (!horizontal.empty()) enumFacing_=java::FacingOpposite(horizontal.front());
        else if (!read(pos.x,pos.y-1,pos.z)) enumFacing_=kFaceUp;
        // Rise retries with the opposite allow-ceiling flag if the first lookup
        // failed, then falls back to lastEnumFacing. No second target is tried.
        else if (!read(pos.x,pos.y+1,pos.z)) enumFacing_=kFaceDown;
        else enumFacing_=lastEnumFacing_;
        if (enumFacing_<0) return;
        lastEnumFacing_=enumFacing_;
        blockPos_={pos.x-java::FacingStepX(enumFacing_),pos.y-java::FacingStepY(enumFacing_),pos.z-java::FacingStepZ(enumFacing_)};
        hasBlockPos_=true;
    }

    // Telly holds the level it started at for as long as the player is walking and
    // not jumping, so the tower stays on one line.
    [[nodiscard]] int GetYLevel() {
        java::EntityHandle self = java::PlayerEntity();
        double playerY = 0.0;
        if (!self.Valid() || !self.GetY(playerY))
            return yLevel_;

        const int groundLevel = FloorToInt(playerY) - 1;
        if (!java::KeyJumpDown() && InputMoving() && TellyActive() &&
            std::fabs(static_cast<double>(yLevel_ - groundLevel)) <= 1.25) {
            return yLevel_;
        }
        return groundLevel;
    }

    // Whether the column the eye is in can be built into -- if something is already
    // there, there is nothing to scaffold.
    [[nodiscard]] bool OnAir() {
        java::EntityHandle self = java::PlayerEntity();
        rotation::Vec3 eye;
        if (!self.Valid() || !self.EyePosition(1.0f, eye))
            return false;
        const java::BlockPos base{FloorToInt(eye.x), GetYLevel(), FloorToInt(eye.z)};
        return java::BlockStateReplaceable(base);
    }

    // Starain's MoveUtils.isMoving, read off the input cached when the move event
    // was last posted -- the same two fields the game itself consults.
    [[nodiscard]] bool InputMoving() const {
        return forwardInput_ != 0.0f || strafeInput_ != 0.0f;
    }

    [[nodiscard]] static bool IsOnBlockEdge(float sensitivity) {
        if (!java::PlayerOnGround())
            return false;

        java::EntityHandle self = java::PlayerEntity();
        rotation::Vec3 position;
        if (!self.Valid() || !self.Position(position))
            return false;

        const double relX = position.x - std::floor(position.x);
        const double relZ = position.z - std::floor(position.z);
        const double closestX = std::min(relX, 1.0 - relX);
        const double closestZ = std::min(relZ, 1.0 - relZ);
        return std::min(closestX, closestZ) <= static_cast<double>(sensitivity);
    }

    Properties::BoolProperty* telly_ = nullptr;
    Properties::BoolProperty* jumpSprint_ = nullptr;
    Properties::BoolProperty* clutch_ = nullptr;
    Properties::BoolProperty* snap_ = nullptr;
    Properties::BoolProperty* degrees45_ = nullptr;
    Properties::BoolProperty* swing_ = nullptr;
    Properties::BoolProperty* itemSpoof_ = nullptr;
    Properties::BoolProperty* keepFov_ = nullptr;
    Properties::NumberProp* fixedFov_ = nullptr;
    int savedSlot_ = -1, visibleSlot_ = -1;
    Properties::ModeProperty* rotMod_ = nullptr;
    Properties::ModeProperty* rayCast_ = nullptr;
    Properties::NumberProp* rotateSpeed_ = nullptr;
    Properties::NumberProp* rotateSpeed2_ = nullptr;
    Properties::NumberProp* rotateBackSpeed_ = nullptr;
    Properties::NumberProp* tellyTick_ = nullptr;
    Properties::BoolProperty* safeWalk_ = nullptr;
    Properties::NumberProp* edgeDist_ = nullptr;

    event::Subscription playerTick_;
    event::Subscription moveInput_;
    event::Subscription respawn_;
    event::Subscription snapMotion_;
    bool snapWaiting_ = false, snapMotionComplete_ = false;
    bool snapHasPlacedDelta_ = false;
    float snapBaseYaw_ = 0, snapBasePitch_ = 0, snapPlacedDelta_ = 0;
    bool snapHasMotion_ = false, snapRotatedSincePlace_ = false;
    bool snapPlacementPending_ = false;
    float snapLatestDelta_ = 0;
    rotation::Rotation snapLastMotion_{};
    int snapHeldTicks_ = 0;
    rotation::Rotation snapRotation_{};

    int airTick_ = 0;
    bool tellyLayerValid_ = false;
    int yLevel_ = 0;
    java::BlockPos blockPos_{};
    bool hasBlockPos_ = false;
    int enumFacing_ = -1;
    int lastEnumFacing_ = -1;
    rotation::Rotation rotation_{};
    bool hasRotation_ = false;
    bool normalRotationActive_ = false;
    bool hypixelAimValid_ = false;
    bool fallingCatch_ = false;
    bool tellyBoostPending_ = true;
    int rotateCount_ = 0;
    float forwardInput_ = 0.0f;
    float strafeInput_ = 0.0f;
    float inputYaw_ = 0.0f;
    float rawInputYaw_ = 0.0f;
};

}

namespace scaffold {

bool IsEnabled() { return g_enabled.load(std::memory_order_acquire); }

// Render-thread state, shared by all camera FOV consumers. Elapsed time rather
// than call count keeps zoom speed stable across FPS and repeated camera queries.
double AnimateKeepFov(double original, double target, double now) {
    struct State { double value = 0, time = 0; bool active = false; };
    static State state;
    if (!std::isfinite(original)) return original;
    const bool enabled = target > 0;
    if (!state.active) {
        state.value = original;
        state.time = now;
        state.active = enabled;
        return original;
    }
    const double elapsed = std::clamp(now - state.time, 0.0, 0.1);
    state.time = now;
    const double destination = enabled ? target : original;
    state.value += (destination - state.value) * -std::expm1(-14.0 * elapsed);
    if (std::abs(destination - state.value) < 0.001) {
        state.value = destination;
        if (!enabled) state.active = false;
    }
    return state.value;
}

double AnimatedWorldFov(double original) {
    const float fixed = g_fixedFov.load(std::memory_order_acquire);
    const double now = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    return AnimateKeepFov(original, IsEnabled() ? fixed : 0.0, now);
}

jfloat JNICALL KeepFovFloatHook(JNIEnv*, jclass, jfloat original, jboolean world) {
    return world ? static_cast<float>(AnimatedWorldFov(original)) : original;
}
jdouble JNICALL KeepFovDoubleHook(JNIEnv*, jclass, jdouble original, jboolean world) {
    return world ? AnimatedWorldFov(original) : original;
}
jfloat JNICALL KeepCameraFovHook(JNIEnv* env, jclass cls, jfloat original) {
    return KeepFovFloatHook(env, cls, original, JNI_TRUE);
}

}

std::unique_ptr<Module> MakeScaffoldModule() {
    return std::make_unique<ScaffoldModule>();
}

}
