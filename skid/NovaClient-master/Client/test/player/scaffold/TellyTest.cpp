#include <cassert>
#include <iostream>
#include "minecraft/utils/rotation/Rotation.h"
#include "minecraft/utils/rotation/Vec3.h"
#include "minecraft/utils/rotation/Mth.h"
#include "minecraft/utils/rotation/MathConst.h"
#include "minecraft/utils/rotation/MovementFix.h"
#include "minecraft/utils/rotation/RotationUtils.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include "minecraft/utils/rotation/Rotation.h"
// @HYPIXEL_HELPERS@

namespace rotation = nova::rotation;
namespace scaffold = nova::module::scaffold;
namespace automlg {bool working=false; bool IsWorking(){return working;}}
namespace event {
enum class EventType { Pre, Post };
struct MotionEvent { EventType type; float yaw, pitch; };
struct PlayerTickEvent {};
struct MoveInputEvent { float forward=0, strafe=0; bool jump=false, sneak=false; };
}
namespace nova::rotation {
double Random() { return .5; }
struct FallingPlayer { double y=0; void Calculate(int) {} };
bool MakeFallingPlayer(FallingPlayer&) { return false; }
enum class RotationPriority { Medium, Highest };
struct RotationState {
    bool active = false;
    RotationPriority priority = RotationPriority::Medium;
    bool hasRotations = false;
    Rotation rotations{};
    bool hasLastRotations = false;
    Rotation lastRotations{};
} state;
RotationState& g_state = state;
RotationState& State() { return state; }
// These legacy cases do not submit a modern interaction; the shared jump
// handler still needs the inactive lock represented in its test fixture.
struct {bool locked=false;Rotation angle{};} interaction;
// @PRODUCTION_JUMP@
enum class BlockFace { Down, Up, North, South, West, East };
Rotation effective{}, requested{};
double speed = 0;
int requests = 0, strictRays = 0, normalRays = 0;
bool strictHit = true, normalHit = true;
bool requireTargetAngle = false, freezeEffective = false;
int smoothingResets = 0;
void SetSmoothed(bool value) { freezeEffective = value; if (!value) ++smoothingResets; }
bool AngleHits(Rotation value) {
    return !requireTargetAngle || (std::fabs(scaffold::AngleDelta(value.yaw, -115)) < 1 &&
                                   std::fabs(value.pitch - 82) < 1);
}
void SetRotations(Rotation value, double step) {
    requested = value;
    if (!freezeEffective) effective = value;
    state.active = state.hasRotations = true;
    state.rotations = effective;
    speed = step;
    ++requests;
}
Rotation GetRotation() { return effective; }
const void* exclusiveOwner = nullptr;
bool OwnsExclusiveRotations(const void* owner) { return owner && exclusiveOwner == owner; }
bool SetExclusiveRotations(const void* owner, Rotation value) {
    if (exclusiveOwner && exclusiveOwner != owner) return false;
    exclusiveOwner = owner;
    effective = state.rotations = value;
    state.active = state.hasRotations = true;
    return true;
}
void ReleaseExclusiveRotations(const void* owner) {
    if (OwnsExclusiveRotations(owner)) exclusiveOwner = nullptr;
}
template <typename Position> bool OverBlock(Rotation value, Position) {
    ++normalRays;
    return normalHit && AngleHits(value);
}
template <typename Position> bool OverBlock(Rotation value, Position, BlockFace) {
    ++strictRays;
    return strictHit && AngleHits(value);
}
}
namespace java {
double playerY = 65;
int availableSlot = 0, skippedTicks = 0;
struct BlockPos { int x = 0, y = 0, z = 0; };
struct BlockHitResultInfo {
    BlockPos blockPos;
    int direction = 0;
    rotation::Vec3 location;
};
struct EntityHandle {
    bool Valid() const { return true; }
    bool EyePosition(float, rotation::Vec3&) const { return true; }
    bool Position(rotation::Vec3& value) const { value={0,65,0};return true; }
    bool GetY(double& out) const { out = playerY; return true; }
};
bool grounded = false;
bool BlockStateReplaceable(BlockPos) { return false; }
bool jumpHeld = false;
bool sprinting = false, blockInHand = true;
int sprintWrites = 0;
void SetPlayerSprinting(bool value) { sprinting = value; ++sprintWrites; }
float cameraYaw = 20;
rotation::Vec3 motion{};
int placements = 0, swings = 0, explicitRotations = 0;
bool KeyJumpDown() { return jumpHeld; }
bool PlayerDeltaMovement(rotation::Vec3& out) { out = motion; return true; }
bool PlayerOnGround() { return grounded; }
bool PlayerRotation(float& yaw, float& pitch) { yaw = cameraYaw; pitch = 10; return true; }
bool sensitivityAvailable = true;
bool PlayerSensitivity(double& value) { value = .5; return sensitivityAvailable; }
int FacingStepX(int face) { return face == 4 ? -1 : face == 5 ? 1 : 0; }
int FacingStepY(int face) { return face == 0 ? -1 : face == 1 ? 1 : 0; }
int FacingStepZ(int face) { return face == 2 ? -1 : face == 3 ? 1 : 0; }
float PlayerPitch() { return 10; }
int physicalSlot=0;
bool SelectedSlot(int& slot) { slot = physicalSlot; return true; }
int spoofSlot=-1, physicalWrites=0, slotReleases=0;
bool SetScaffoldSlot(int slot) { spoofSlot=slot; return true; }
void ReleaseScaffoldSlot() { spoofSlot=-1; ++slotReleases; }
bool EffectiveSelectedSlot(int& slot) { slot=spoofSlot<0?physicalSlot:spoofSlot; return true; }
int stackCount=64;
int HotbarSlotCount(int) { return stackCount; }
int FindPlaceableBlockSlot(int reserve) { return stackCount > reserve ? availableSlot : -1; }
bool SetSelectedSlot(int slot) { physicalSlot=slot; ++physicalWrites; return true; }
int CountPlaceableBlocks() { return 64; }
void RequestPlayerTickSkip() { ++skippedTicks; }
int swingPackets=0;
void SendSwingPacket() { ++swingPackets; }
bool HotbarSlotPlaceable(int) { return blockInHand; }
EntityHandle PlayerEntity() { return {}; }
void SendMoveRotation(float, float, bool) { ++explicitRotations; }
bool consumeInteraction = true;
BlockHitResultInfo placedHit{};
bool UseItemOnBlock(BlockPos pos, int face, rotation::Vec3 point) {
    placedHit = {pos, face, point}; ++placements; return consumeInteraction;
}
void SwingMainHand() { ++swings; }
}
struct Property {
    double value = 0;
    double get() const { return value; }
};
constexpr int kHypixelRotation = 2;
constexpr double kBurstSpeed = 1.5, kInstantSpeed = 180;
constexpr int kMaxBurst = 8;
int FloorToInt(double value) { return static_cast<int>(std::floor(value)); }
float RandomRange(float a, float b) { return (a + b) * 0.5f; }
rotation::Vec3 HitVector(java::BlockPos, int) { return {}; }

struct Fixture {
    Property itemSpoof{1};
    Property* itemSpoof_ = &itemSpoof;
    int savedSlot_ = -1, visibleSlot_ = -1;
    Property snap{0}, degrees45{0};
    Property *snap_ = &snap, *degrees45_ = &degrees45;
    bool snapWaiting_ = false, snapMotionComplete_ = false;
    bool snapHasPlacedDelta_ = false;
    float snapBaseYaw_ = 0, snapBasePitch_ = 0, snapPlacedDelta_ = 0;
    bool snapHasMotion_ = false, snapRotatedSincePlace_ = false;
    bool snapPlacementPending_ = false;
    float snapLatestDelta_ = 0;
    rotation::Rotation snapLastMotion_{};
    int snapHeldTicks_ = 0;
    rotation::Rotation snapRotation_{};
    Property safeWalk{0}, edge{0.1};
    Property *safeWalk_ = &safeWalk, *edgeDist_ = &edge;
    Property telly{1}, sprint{1}, mode{2}, back{180}, tick{1}, speed{127}, speed2{36}, ray{0}, swing{1}, clutch{0};
    Property *telly_ = &telly, *jumpSprint_ = &sprint, *rotMod_ = &mode,
             *rotateBackSpeed_ = &back, *tellyTick_ = &tick, *rotateSpeed_ = &speed,
             *rotateSpeed2_ = &speed2, *rayCast_ = &ray, *swing_ = &swing, *clutch_ = &clutch;
    bool hasRotation_ = false, hasBlockPos_ = false, hypixelAimValid_ = false;
    bool normalRotationActive_ = false;
    bool tellyBoostPending_ = true;
    bool tellyLayerValid_ = false;
    bool fallingCatch_ = false;
    int yLevel_ = 0, rotateCount_ = 0, blockSearches = 0;
    bool isEnabled() const { return true; }
    void GetBlockInfo(const rotation::Vec3* = nullptr) { ++blockSearches; }
    int airTick_ = 1, enumFacing_ = -1, legacySearches = 0, boundedSearches = 0, traces = 0;
    float strafeInput_ = 0, forwardInput_ = 1, rawInputYaw_ = 20, inputYaw_ = 0;
    rotation::Rotation rotation_{};
    java::BlockPos blockPos_{};
    bool onAir = true;
    bool OnAir() { return onAir; }
    bool IsOnBlockEdge(float) { return false; }
    bool InputMoving() { return forwardInput_ != 0 || strafeInput_ != 0; }
    // Search geometry has separate tests; here track which path production chooses.
    rotation::Rotation GetRotation() { ++legacySearches; return {-115, 82}; }
    void ComputeHypixelRotation() { ++boundedSearches; hypixelAimValid_ = false; }
    bool snapRayHit = false, rejectActualRay = false;
    rotation::Rotation tracedAngle{};
    bool TracePlacement(rotation::Rotation angle, java::EntityHandle, rotation::Vec3,
                        java::BlockHitResultInfo& hit) {
        ++traces; tracedAngle = angle;
        hit = {blockPos_, enumFacing_, {.125, .75, .875}};
        return snapRayHit && !(rejectActualRay && rotation::OwnsExclusiveRotations(this));
    }
    // @PRODUCTION_METHODS@
};

int main() {
    {
        Fixture slot;
        java::physicalSlot=2;
        assert(slot.SelectSlot(5) && java::physicalSlot==2 && java::spoofSlot==5);
        slot.itemSpoof.value=0;
        assert(slot.SelectSlot(5) && java::physicalSlot==5 && java::spoofSlot==-1);
        slot.itemSpoof.value=1;
        assert(slot.SelectSlot(6) && java::physicalSlot==2 && java::spoofSlot==6);
        slot.ReleaseSlot();
        assert(java::physicalSlot==2 && java::spoofSlot==-1);
        slot.itemSpoof.value=0;
        assert(slot.SelectSlot(5));
        java::physicalSlot=3;
        slot.ReleaseSlot();
        assert(java::physicalSlot==3);
        java::physicalSlot=0; java::physicalWrites=0;
    }

    // Normal keeps submitting the previous aim across supported ground,
    // failed target searches, and temporary target loss. Explicit Snap opts out.
    {
        Fixture normal;
        normal.telly.value = 0;
        // Fresh activation has no previously successful ray.
        assert(!normal.hasRotation_);
        normal.onAir = false;
        normal.HandleNormal();
        assert(rotation::requests == 1 && normal.boundedSearches == 0);
        assert(rotation::requested.yaw == -160 && rotation::requested.pitch == 90 && java::placements == 0);
        normal.onAir = true;
        normal.hasBlockPos_ = true;
        normal.enumFacing_ = 3;
        normal.HandleNormal();
        assert(rotation::requests == 2 && normal.boundedSearches == 1);
        assert(!normal.hypixelAimValid_ && java::placements == 0);
        normal.hasBlockPos_ = false;
        normal.HandleNormal();
        assert(rotation::requests == 3 && rotation::requested.pitch == 90);
        normal.snap.value = 1;
        java::grounded = true;
        normal.HandleNormal();
        assert(rotation::requests == 3);
        java::grounded = false;
        normal.snap.value = 0;
        normal.telly.value = 1;
        normal.HandleTelly();
        normal.telly.value = 0;
        normal.onAir = false;
        normal.HandleNormal();
        assert(rotation::requested.yaw == -160 && rotation::requested.pitch == 90);
        assert(normal.normalRotationActive_ && java::placements == 0);
        rotation::requests = 0;
        rotation::state = {};
        rotation::effective = {};
    }
    Fixture f;
    // No target at takeoff: prepare at fast speed, but never place blindly.
    f.HandleTelly();
    assert(f.legacySearches == 1 && f.boundedSearches == 0 && f.hasRotation_);
    assert(rotation::requests == 1 && f.tellyBoostPending_);
    assert(rotation::requested.yaw == -115 && java::placements == 0);

    // Still no face on the next tick: continue turning, keep placement blocked.
    f.airTick_ = 2;
    f.HandleTelly();
    assert(rotation::requests == 2 && f.tellyBoostPending_ && java::placements == 0);

    // A block-level hit in Normal mode must not be rejected by the new strict path.
    f.hasBlockPos_ = true;
    f.enumFacing_ = 3;
    rotation::strictHit = false;
    rotation::effective = {60, 0};
    f.HandleTelly();
    assert(java::placements == 1 && f.traces == 0 && !f.tellyBoostPending_);
    assert(rotation::speed == 360); // The per-axis step is already applied.
    assert(rotation::requested.yaw == -65); // First valid target gets 125, even after tick one.
    f.HandleTelly();
    assert(rotation::requested.yaw == -101.5f); // Then 36.5, applied per axis.
    assert(java::placements == 2);

    // Respect an explicitly selected Strict mode.
    f.ray.value = 1;
    f.HandleTelly();
    assert(java::placements == 2 && rotation::strictRays > 0);
    rotation::strictHit = true;
    f.HandleTelly();
    assert(java::placements == 3 && java::swings == 3);

    // TellyTicks is still the placement gate even if a face already exists.
    f.airTick_ = 1;
    f.HandleTelly();
    assert(java::placements == 3);

    java::grounded = true;
    f.HandleTelly();
    assert(rotation::requested.yaw == 20 && rotation::requested.pitch == 85);
    assert(f.tellyBoostPending_ && java::placements == 3);

    // Cover W, W+A, W+D across all diagonal quadrants and the yaw wrap boundary.
    for (float camera : {45.0f, -45.0f, 135.0f, -135.0f, 179.0f}) {
        for (float strafe : {-1.0f, 0.0f, 1.0f}) {
            java::cameraYaw = camera;
            f.strafeInput_ = strafe;
            const float intended = rotation::Direction(1, strafe, camera);
            f.rawInputYaw_ = rotation::mth::WrapDegrees(intended);
            rotation::effective = {intended - 180, 85};
            f.HandleTelly();
            rotation::MoveInput input{1, strafe};
            rotation::FixMovement(input, rotation::effective.yaw, camera);
            assert(input.forward >= 0.8f);
            const float travel = rotation::Direction(input.forward, input.strafe, rotation::effective.yaw);
            assert(std::fabs(scaffold::AngleDelta(travel, intended)) < 0.01f);
            float jumpYaw = camera;
            rotation::OnJump(jumpYaw);
            assert(std::fabs(scaffold::AngleDelta(jumpYaw, intended)) < 0.01f);
            assert(f.tellyBoostPending_ && java::placements == 3);
        }
    }

    // Diagonal takeoff must preserve a positive forward input after movement fix.
    java::jumpHeld = true;
    java::cameraYaw = 45;
    f.rawInputYaw_ = 45;
    const int searches = f.legacySearches;
    rotation::effective = {-135, 85};
    f.HandleTelly();
    rotation::MoveInput takeoffInput{1, 0};
    rotation::FixMovement(takeoffInput, rotation::effective.yaw, java::cameraYaw);
    assert(takeoffInput.forward >= 0.8f);
    assert(std::fabs(scaffold::AngleDelta(rotation::effective.yaw, 45)) < 0.01f);
    assert(f.legacySearches == searches);
    assert(f.tellyBoostPending_ && java::placements == 3);

    // Falling with Clutch off gets a fast ordinary turn and no Telly delay.
    java::grounded = false;
    java::motion.y = -0.3;
    f.airTick_ = 4;
    f.tick.value = 6;
    f.tellyBoostPending_ = false;
    rotation::effective = {60, 0};
    f.HandleTelly();
    assert(rotation::requested.yaw == -67 && java::placements == 4);
    assert(java::explicitRotations == 0);

    // Even the falling path must wait for the effective-ray check to pass.
    rotation::strictHit = false;
    f.HandleTelly();
    assert(java::placements == 4);
    rotation::strictHit = true;

    // A high-fall landing catch must turn in this tick even if earlier aim was
    // cached. It remains one ordinary ray-checked placement, without Clutch.
    {
        Fixture catchFall;
        catchFall.hasBlockPos_ = catchFall.hasRotation_ = true;
        catchFall.enumFacing_ = 3;
        catchFall.airTick_ = 2; // Enabling mid-fall must not restart the Telly wait.
        catchFall.tick.value = 20;
        catchFall.ray.value = 1;
        catchFall.fallingCatch_ = true;
        catchFall.tellyBoostPending_ = false;
        rotation::requireTargetAngle = true;
        rotation::effective = {65,-65};
        rotation::freezeEffective = true;
        rotation::state = {};
        java::motion.y = -1.2;
        const int before = java::placements, sent = java::explicitRotations, skipped = java::skippedTicks;
        catchFall.HandleTelly();
        assert(rotation::AngleHits(rotation::effective) && java::placements == before + 1);
        assert(java::explicitRotations == sent && java::skippedTicks == skipped);
        rotation::strictHit = false;
        catchFall.HandleTelly();
        assert(java::placements == before + 1);
        rotation::strictHit = true;
        rotation::state.priority = rotation::RotationPriority::Highest;
        const int resets = rotation::smoothingResets;
        catchFall.HandleTelly();
        assert(java::placements == before + 1 && rotation::smoothingResets == resets);
        rotation::state = {};
        rotation::requireTargetAngle = false;
    }

    // A valid target can be 175 degrees away: the first 125-degree step
    // misses. Keep the boost for the second tick in Normal and Strict alike.
    for (int rayMode : {0, 1}) {
        Fixture turn;
        turn.hasBlockPos_ = true;
        turn.enumFacing_ = 3;
        turn.airTick_ = 2;
        turn.ray.value = rayMode;
        java::motion = {};
        rotation::effective = {60, 0};
        rotation::requireTargetAngle = true;
        const int before = java::placements;
        turn.HandleTelly();
        assert(rotation::requested.yaw == -65 && turn.tellyBoostPending_);
        assert(java::placements == before);
        turn.HandleTelly();
        assert(rotation::requested.yaw == -115 && !turn.tellyBoostPending_);
        assert(java::placements == before + 1);

        // Another rotation submission may have already smoothed this tick.
        // Requested angles must not stand in for the actual effective ray.
        turn.tellyBoostPending_ = true;
        rotation::effective = {0, 0};
        rotation::freezeEffective = true;
        turn.HandleTelly();
        assert(turn.tellyBoostPending_ && java::placements == before + 1);
        rotation::freezeEffective = false;
        turn.HandleTelly();
        assert(!turn.tellyBoostPending_ && java::placements == before + 2);
        rotation::requireTargetAngle = false;
    }

    // A higher-priority rotation owner must not consume the pending boost.
    rotation::state = {true, rotation::RotationPriority::Highest};
    f.tellyBoostPending_ = true;
    const int requests = rotation::requests;
    f.HandleTelly();
    assert(f.tellyBoostPending_ && rotation::requests == requests);
    rotation::state = {};

    // The same predicate routes search, placement and rescue away from bounded mode.
    assert(f.HypixelTelly() && !f.UseBoundedRotation());
    f.telly.value = 0;
    assert(!f.HypixelTelly() && f.UseBoundedRotation());
    f.ComputeRotation();
    assert(f.boundedSearches == 1);
    f.telly.value = 1;
    f.sprint.value = 0;
    assert(!f.HypixelTelly() && f.UseBoundedRotation());
    f.mode.value = 0;
    assert(!f.UseBoundedRotation());
    // Execute the real tick method: keep the takeoff layer through descent,
    // refresh it on landing, and initialize it when enabling Telly midair.
    {
        Fixture cycle;
        event::PlayerTickEvent event;
        java::grounded = true;
        java::playerY = 65;
        java::motion = {};
        rotation::state = {};
        cycle.OnPreTick(event);
        assert(cycle.tellyLayerValid_ && cycle.yLevel_ == 64 && cycle.airTick_ == 0);
        java::grounded = false;
        for (double height : {65.4, 66.0, 65.0, 63.4, 61.4}) {
            java::playerY = height;
            cycle.OnPreTick(event);
            assert(cycle.yLevel_ == 64 && cycle.tellyLayerValid_);
        }
        assert(cycle.airTick_ == 5 && cycle.blockSearches == 6 && java::skippedTicks == 0);
        java::grounded = true; java::playerY = 66;
        cycle.OnPreTick(event);
        assert(cycle.yLevel_ == 65 && cycle.airTick_ == 0);
        cycle.telly.value = 0;
        cycle.OnPreTick(event);
        assert(!cycle.tellyLayerValid_);
        cycle.telly.value = 1; java::grounded = false; java::playerY = 68.4;
        cycle.OnPreTick(event);
        assert(cycle.tellyLayerValid_ && cycle.yLevel_ == 67);
        java::availableSlot = -1;
        cycle.OnPreTick(event);
        assert(cycle.airTick_ == 1);
        assert(java::spoofSlot == -1 && java::slotReleases > 0);
        java::availableSlot = 5;
        cycle.OnPreTick(event);
        assert(java::spoofSlot == 5 && java::physicalWrites == 0);
    }
    // Reproduce tick -> fresh input -> movement fix -> sprint check -> jump.
    // The first tick sees idle/old input, and smoothing is already cached when
    // W or W+A/D arrives. Even a deliberately low BackSpeed must not delay takeoff.
    java::availableSlot = 0;
    for (float camera : {0.f, 45.f, -45.f, 135.f, -135.f, 179.f}) {
        for (float strafe : {-1.f, 0.f, 1.f}) {
            for (bool manualJump : {false, true}) {
                Fixture start;
                start.back.value = 1;
                start.forwardInput_ = 0;
                start.rawInputYaw_ = camera - 180;
                java::grounded = true;
                java::cameraYaw = camera;
                java::jumpHeld = manualJump;
                java::sprinting = false;
                rotation::state = {};
                rotation::effective = {camera - 180, 82};
                event::PlayerTickEvent tickEvent;
                start.OnPreTick(tickEvent);
                rotation::freezeEffective = true;
                event::MoveInputEvent input{1, strafe, manualJump, false};
                start.OnMoveInput(input);
                rotation::MoveInput fixed{input.forward, input.strafe};
                rotation::FixMovement(fixed, rotation::effective.yaw, camera);
                const float intended = rotation::Direction(1, strafe, camera);
                assert(input.jump && java::sprinting && fixed.forward >= 0.8f);
                assert(std::fabs(scaffold::AngleDelta(
                    rotation::Direction(fixed.forward, fixed.strafe, rotation::effective.yaw), intended)) < 0.6f);
                float jumpYaw = camera;
                rotation::OnJump(jumpYaw);
                assert(std::fabs(scaffold::AngleDelta(jumpYaw, intended)) < 0.6f);

                // Fresh W+A/D change on the next landing must replace the cached
                // previous direction before the next movement correction too.
                rotation::freezeEffective = true;
                input = {1, -strafe, true, false};
                start.OnMoveInput(input);
                rotation::OnJump(jumpYaw);
                assert(std::fabs(scaffold::AngleDelta(jumpYaw,
                    rotation::Direction(1, -strafe, camera))) < 0.6f);
            }
        }
    }
    // Keep the airborne placement turn, sneak, non-forward input, other modes,
    // empty hand and higher-priority rotation ownership outside the correction.
    for (int scenario = 0; scenario < 8; ++scenario) {
        Fixture gated;
        event::MoveInputEvent input{1, 0, true, false};
        java::grounded = scenario != 0;
        java::blockInHand = scenario != 1;
        java::jumpHeld = true;
        rotation::state = {};
        if (scenario == 2) rotation::state = {true, rotation::RotationPriority::Highest};
        if (scenario == 3) input.sneak = true;
        if (scenario == 4) gated.sprint.value = 0;
        if (scenario == 5) gated.mode.value = 0;
        if (scenario == 6) input.forward = 0;
        if (scenario == 7) input.forward = -1;
        const int requestsBefore = rotation::requests, resetsBefore = rotation::smoothingResets;
        const int sprintBefore = java::sprintWrites;
        gated.OnMoveInput(input);
        assert(rotation::requests == requestsBefore && rotation::smoothingResets == resetsBefore);
        if (scenario >= 6) assert(java::sprintWrites == sprintBefore);
        if (scenario == 4) assert(!java::sprinting);
    }
    {
        Fixture quiet;
        quiet.mode.value=0;
        quiet.hasBlockPos_=true;
        quiet.enumFacing_=1;
        quiet.swing.value=0;
        java::blockInHand=true;
        rotation::requireTargetAngle=false;
        rotation::normalHit=true;
        const int packets=java::swingPackets, arms=java::swings, placed=java::placements;
        quiet.Place();
        assert(java::placements==placed+1 && java::swingPackets==packets+1 && java::swings==arms);
        java::stackCount=1;
        quiet.Place();
        assert(java::placements==placed+1 && java::swingPackets==packets+1);
        java::stackCount=2;
        quiet.swing.value=1;
        quiet.Place();
        assert(java::swings==arms+1 && java::swingPackets==packets+1);
        java::stackCount=64;
        quiet.hasBlockPos_=false;
        quiet.swing.value=0;
        quiet.Place();
        assert(java::swingPackets==packets+1);
    }
    // Release arrives after PreTick has already used the previous input. The
    // last edge block must not wait for onGround to clear or TellyTicks to pass.
    java::blockInHand = true;
    java::stackCount = 64;
    java::jumpHeld = false;
    rotation::strictHit = true;
    rotation::normalHit = true;
    rotation::requireTargetAngle = true;
    for (bool grounded : {false, true}) for (bool falling : {false, true}) {
        Fixture stop;
        stop.hasBlockPos_ = stop.hasRotation_ = true;
        stop.enumFacing_ = 3;
        stop.ray.value = 1;
        stop.airTick_ = 0;
        stop.tick.value = 6;
        stop.inputYaw_ = 45;
        stop.rawInputYaw_ = 43;
        stop.tellyBoostPending_ = false;
        stop.fallingCatch_ = falling;
        java::grounded = grounded;
        java::motion = {.1, falling ? -1.2 : 0.0, 0};
        rotation::state = {};
        rotation::effective = {65, -65};
        rotation::freezeEffective = true;
        const int before = java::placements;
        event::MoveInputEvent released{0, 0, false, false};
        stop.OnMoveInput(released);
        assert(java::placements == before + 1 && rotation::AngleHits(rotation::effective));
        assert(stop.inputYaw_ == 45 && stop.rawInputYaw_ == 43 && !released.jump);
        assert(stop.blockSearches == 1);
        // Once supported, remaining idle must not add more blocks.
        stop.onAir = false;
        stop.OnMoveInput(released);
        stop.HandleTelly();
        assert(java::placements == before + 1);
    }
    // Fresh-input retry still obeys the ray, inventory and rotation owner gates.
    for (int scenario = 0; scenario < 5; ++scenario) {
        Fixture stop;
        stop.hasBlockPos_ = stop.hasRotation_ = true;
        stop.enumFacing_ = 3; stop.ray.value = 1;
        java::grounded = true; java::motion = {};
        rotation::state = {}; rotation::freezeEffective = true;
        rotation::effective = {65, -65};
        rotation::strictHit = scenario != 0;
        java::blockInHand = scenario != 1;
        if (scenario == 2) rotation::state = {true, rotation::RotationPriority::Highest};
        if (scenario == 3) stop.mode.value = 0;
        if (scenario == 4) stop.hasBlockPos_ = false;
        const int before = java::placements;
        event::MoveInputEvent released{0, 0, false, false};
        stop.OnMoveInput(released);
        assert(java::placements == before && !released.jump);
    }
    // Snap runs production methods against the JNI/rotation boundary doubles.
    // It must retain the placement angle through Post, and restore LIVE camera
    // only at the next tick; no burst, recursive tick or fake movement packet.
    java::blockInHand = true;
    java::stackCount = 64;
    java::grounded = true;
    rotation::state = {};
    rotation::freezeEffective = false;
    {
        Fixture snap;
        snap.snap.value = 1;
        snap.telly.value = 1;
        snap.sprint.value = 0;
        assert(!snap.SnapActive()); // Raw Telly selection always forbids Snap.
        snap.telly.value = 0;
        snap.hasBlockPos_ = true;
        snap.enumFacing_ = 3;
        snap.blockPos_ = {1, -1, 2};
        const int before = java::placements;
        snap.HandleSnap(); // No matching ray: no turn, no packet.
        assert(java::placements == before && !snap.snapWaiting_);
        snap.snapRayHit = true;
        snap.HandleSnap();
        assert(java::placements == before + 1 && snap.snapWaiting_);
        assert(rotation::OwnsExclusiveRotations(&snap));
        assert(java::placedHit.blockPos.x == 1 && java::placedHit.direction == 3);
        assert(java::placedHit.location.x == .125 && java::placedHit.location.z == .875);
        // Actual post-quantization angle was validated, not a geometric guess.
        assert(snap.tracedAngle.yaw == rotation::effective.yaw);
        assert(snap.tracedAngle.pitch == rotation::effective.pitch);
        snap.HandleSnap();
        assert(java::placements == before + 1);
        event::MotionEvent pre{event::EventType::Pre, snap.snapRotation_.yaw, snap.snapRotation_.pitch};
        snap.OnSnapMotion(pre);
        assert(!snap.snapMotionComplete_ && !snap.snapHasPlacedDelta_);
        event::MotionEvent wrong{event::EventType::Post, snap.snapRotation_.yaw, snap.snapRotation_.pitch - 10};
        snap.OnSnapMotion(wrong);
        assert(!snap.snapMotionComplete_ && !snap.BeginSnapTick());
        event::MotionEvent post{event::EventType::Post, snap.snapRotation_.yaw + 360, snap.snapRotation_.pitch};
        snap.OnSnapMotion(post);
        assert(snap.snapMotionComplete_ && rotation::OwnsExclusiveRotations(&snap));
        assert(!snap.snapHasPlacedDelta_); // First placement precedes first look update.
        const float placedDelta = snap.snapPlacedDelta_;
        java::cameraYaw = 72;
        assert(snap.BeginSnapTick());
        assert(!snap.snapWaiting_ && !rotation::OwnsExclusiveRotations(&snap));
        assert(!snap.snapHasPlacedDelta_ && snap.snapPlacedDelta_ == placedDelta);
        assert(rotation::effective.yaw == 72 && rotation::effective.pitch == 10);
        snap.onAir = false;
        snap.HandleSnap();
        assert(java::placements == before + 1);
        // Failed interaction can still emit a packet: hold its angle as well.
        snap.onAir = true;
        java::consumeInteraction = false;
        const int swings = java::swings;
        snap.HandleSnap();
        assert(snap.snapWaiting_ && java::swings == swings);
        assert(!snap.BeginSnapTick());
        assert(!snap.BeginSnapTick()); // Bounded recovery when Post never arrives.
        assert(!snap.snapWaiting_ && !rotation::OwnsExclusiveRotations(&snap));
        java::consumeInteraction = true;
        // Turning Snap off or selecting Telly releases this module only.
        snap.HandleSnap();
        snap.telly.value = 1;
        assert(snap.BeginSnapTick() && !snap.snapWaiting_);
        assert(!snap.snapHasPlacedDelta_);
        snap.telly.value = 0;
        snap.HandleSnap();
        snap.snap.value = 0;
        assert(snap.BeginSnapTick() && !snap.snapWaiting_);
    }
    {
        Fixture snap;
        snap.snap.value=1;snap.telly.value=0;
        // A normal return followed by a position-only placement must use the
        // last LOOK delta, not zero or the manager's overwritten base angle.
        event::MotionEvent first{event::EventType::Post,170,80};snap.OnSnapMotion(first);
        event::MotionEvent returned{event::EventType::Post,0,10};snap.OnSnapMotion(returned);
        assert(snap.snapLatestDelta_==170 && snap.snapRotatedSincePlace_);
        snap.snapWaiting_=true;snap.snapRotation_={0,10};snap.snapBaseYaw_=0;
        snap.snapPlacementPending_=true;
        rotation::SetExclusiveRotations(&snap,{0,10});
        snap.OnSnapMotion(returned);
        assert(snap.snapPlacedDelta_==170 && !snap.snapRotatedSincePlace_);
        snap.ResetSnap();
        assert(snap.snapHasMotion_ && snap.snapLastMotion_.yaw==0);
    }
    for (int scenario = 0; scenario < 8; ++scenario) {
        Fixture snap;
        snap.snap.value = 1; snap.telly.value = 0;
        snap.hasBlockPos_ = true; snap.enumFacing_ = 3;
        snap.snapRayHit = true;
        rotation::state = {};
        java::blockInHand = true; java::stackCount = 64;
        java::sensitivityAvailable = true;
        if (scenario == 0) java::blockInHand = false;
        if (scenario == 1) java::stackCount = 1;
        if (scenario == 2) rotation::state = {true, rotation::RotationPriority::Highest};
        if (scenario == 3) java::sensitivityAvailable = false;
        if (scenario == 4) snap.rejectActualRay = true;
        if (scenario == 5) rotation::exclusiveOwner = &f;
        if (scenario == 6) automlg::working = true;
        if (scenario == 7) snap.hasBlockPos_ = false;
        const int before = java::placements;
        snap.HandleSnap();
        assert(java::placements == before && !snap.snapWaiting_);
        snap.ResetSnap();
        if (scenario == 5) assert(rotation::OwnsExclusiveRotations(&f));
        rotation::exclusiveOwner = nullptr;
        automlg::working = false;
    }
    {
        // Independent server order: queued C08 first, then RotationUpdate.
        // This intentionally differs from the old rotation-before-place test.
        struct Server {
            float yaw=0, delta=0, placed=0;
            bool rotated=false, queued=false;
            int flags=0, places=0;
            void Place(){queued=true;}
            void Flying(float next, bool look=true){
                if(queued){
                    ++places;
                    if(rotated){
                        if(delta>2 && std::fabs(delta-placed)<.0001f) ++flags;
                        placed=delta;rotated=false;
                    }
                    queued=false;
                }
                if(look){delta=std::fabs(next-yaw);yaw=next;rotated=true;}
            }
        } oldServer, server;
        oldServer.Flying(170);oldServer.Place();oldServer.Flying(0);
        oldServer.Place();oldServer.Flying(175);
        assert(oldServer.flags==1); // New 175-degree turn cannot repair old 170.

        scaffold::ResetSnapWireHistory();
        auto& wire=scaffold::snapWireHistory;
        wire.Observe(true,true,0,10);
        auto flying=[&](float yaw,float pitch,bool look=true){
            server.Flying(yaw,look);
            wire.Observe(true,look,yaw,pitch);
            assert(wire.delta==server.delta && wire.rotated==server.rotated);
            assert(wire.placedDelta==server.placed);
        };
        flying(170,80);
        server.Place();wire.Observe(false,false,0,0);flying(0,10);
        Fixture snap;
        snap.snap.value=1;snap.telly.value=0;
        snap.hasBlockPos_=snap.snapRayHit=true;snap.enumFacing_=3;
        snap.blockPos_={1,-1,2};
        java::cameraYaw=0;java::blockInHand=true;java::stackCount=64;
        java::sensitivityAvailable=true;rotation::state={};
        rotation::effective={0,10};
        const int before=java::placements;
        snap.HandleSnap();
        assert(snap.snapWaiting_ && java::placements==before);
        // A normal turn-only tick prepares the next placement without C08.
        flying(snap.snapRotation_.yaw,snap.snapRotation_.pitch);
        event::MotionEvent post{event::EventType::Post,wire.yaw,wire.pitch};
        snap.OnSnapMotion(post);assert(snap.BeginSnapTick());
        int accepted=0;
        for(int tick=0;tick<256;++tick){
            rotation::state.lastRotations={wire.yaw,wire.pitch};
            rotation::state.hasLastRotations=true;
            const int count=java::placements;
            snap.HandleSnap();
            const bool attempted=java::placements!=count;
            // Some Lunar callbacks cancel locally; other false results have
            // already sent C08. Dispatch observation handles both cases.
            const bool emitted=attempted && tick%11!=0;
            if(emitted){server.Place();wire.Observe(false,false,0,0);++accepted;}
            const auto angle=rotation::effective;
            const bool look=angle.yaw!=wire.yaw || angle.pitch!=wire.pitch;
            flying(angle.yaw,angle.pitch,look);
            post={event::EventType::Post,angle.yaw,angle.pitch};
            snap.OnSnapMotion(post);
            assert(snap.snapPlacedDelta_==server.placed && server.flags==0);
            assert(snap.BeginSnapTick());
            // Vanilla return/position-only frames between placement cycles.
            if(tick%3==0){
                flying(java::cameraYaw,10,wire.yaw!=java::cameraYaw || wire.pitch!=10);
                post={event::EventType::Post,java::cameraYaw,10};snap.OnSnapMotion(post);
                flying(java::cameraYaw,10,false);snap.OnSnapMotion(post);
            }
            if(tick%17==0)java::cameraYaw+=.75f;
        }
        assert(accepted>100 && server.flags==0);
        // Buffered packets do not change history until dispatch. Repeated-look
        // packets still cause a RotationUpdate (unlike a position-only packet).
        auto saved=wire;
        post={event::EventType::Post,900,80};snap.OnSnapMotion(post);
        assert(wire.yaw==saved.yaw && snap.snapLastMotion_.yaw==saved.yaw);
        flying(wire.yaw,wire.pitch,true);assert(wire.delta==0 && wire.rotated);
        server.Place();wire.Observe(false,false,0,0);flying(wire.yaw,wire.pitch,false);
        assert(wire.placedDelta==0 && !wire.rotated);
        snap.ResetSnap();scaffold::ResetSnapWireHistory();
        std::cout<<"PASS: Lunar dispatch history, cancel/replay and real Grim queue order; old repro=1 flag, fixed cycles="<<accepted<<", flags=0\n";
    }
    // OnPreTick must route Snap before legacy Clutch recursion, even at high speed.
    {
        Fixture snap;
        snap.snap.value = 1; snap.telly.value = 0; snap.clutch.value = 1;
        snap.hasBlockPos_ = snap.snapRayHit = true; snap.enumFacing_ = 3;
        java::blockInHand = true; java::stackCount = 64; java::sensitivityAvailable = true;
        java::motion = {2, -2, 0}; rotation::state = {};
        event::PlayerTickEvent tick;
        const int before = java::placements;
        snap.OnPreTick(tick);
        assert(java::placements == before + 1 && snap.snapWaiting_);
        snap.OnPreTick(tick);
        assert(java::placements == before + 1);
        snap.ResetSnap();
    }
    // RotSpeed scales both placement phases; BackSpeed independently limits return.
    {
        Fixture tuned;
        tuned.hasBlockPos_ = true; tuned.enumFacing_ = 3; tuned.airTick_ = 2;
        java::grounded = false; java::motion = {};
        rotation::requireTargetAngle = rotation::freezeEffective = false;
        rotation::strictHit = rotation::normalHit = true;
        rotation::state = {}; rotation::effective = {60,82};
        tuned.speed.value = 63.5;
        tuned.HandleTelly();
        assert(std::fabs(rotation::requested.yaw + 2.5f) < .001f);
        rotation::effective = {60,82};
        tuned.HandleTelly();
        assert(std::fabs(rotation::requested.yaw - 41.75f) < .001f);
        tuned.speed.value = 180; tuned.tellyBoostPending_ = true;
        rotation::effective = {60,82};
        tuned.HandleTelly();
        assert(std::fabs(rotation::requested.yaw + 115.f) < .001f);
        java::grounded = true; tuned.back.value = 20;
        tuned.rawInputYaw_ = 60; rotation::effective = {-115,82};
        tuned.HandleTelly();
        assert(rotation::requested.yaw == -95 && rotation::requested.pitch == 85);
        java::grounded = false; java::motion.y = -.3;
        tuned.airTick_ = 4; tuned.speed.value = 1; tuned.tellyBoostPending_ = false;
        rotation::effective = {60,82};
        tuned.HandleTelly();
        assert(rotation::requested.yaw == -67); // Existing rescue floor survives low speeds.
    }
    assert(java::explicitRotations == 0 && java::skippedTicks == 0);
    std::cout << "PASS: Snap strict ray gating, motion lifecycle, live camera return, timeout, Telly exclusion, slot/owner gates, no burst packets\n";
    std::cout << "PASS: fresh-input Telly stop, grounded/airborne final placement, retained heading and stop safety gates\n";
    std::cout << "PASS: Telly delayed target boost, fresh-input takeoff and cached smoothing, diagonal sprint input and jump yaw, falling catch, ray gates, rotation priority, mode routing\n";
}
