#pragma once

#include <functional>

#include "minecraft/java/api/game_entity.h"
#include "minecraft/utils/rotation/MovementFix.h"
#include "minecraft/utils/rotation/Rotation.h"

namespace nova::rotation {

enum class RotationPriority { Medium, Highest };

struct RotationManagerState {
    Rotation offset;
    bool hasRotations = false;
    Rotation rotations;
    bool hasLastRotations = false;
    Rotation lastRotations;
    bool hasTargetRotations = false;
    Rotation targetRotations;
    bool hasAnimation = false;
    Rotation animationRotation;
    bool hasLastAnimation = false;
    Rotation lastAnimationRotation;
    bool hasRaycast = false;
    std::function<bool(const Rotation&)> raycast;
    bool active = false;
    bool smoothed = false;
    bool linear = false;
    bool linearReturning = false;
    bool direct = false;
    bool moveInputFix = true;
    bool randomize = true;
    double rotationSpeed = 0.0;
    double linearHorizontalSpeed = 0.0;
    double linearVerticalSpeed = 0.0;
    Rotation linearRemainder;
    RotationPriority priority = RotationPriority::Medium;
    float randomAngle = 0.0f;
};

RotationManagerState& State();
// Game-thread emergency interaction lease; only its owner can update/release it.
bool SetExclusiveRotations(const void* owner,const Rotation& rotations);
bool OwnsExclusiveRotations(const void* owner);
void ReleaseExclusiveRotations(const void* owner);
// Visible scripted camera motion must preserve the user's pre-camera heading.
bool SetExclusiveMovementYaw(const void* owner, float yaw);

// Yaw remains unbounded/continuous: never normalize the published angle to +/-180.
// Accepted requests default to preserving the physical movement direction.
// A caller needing raw movement must set its policy after submitting a request.
void SetRotations(const Rotation& rotations, double rotationSpeed);

// Independent degrees/second per axis; zero holds that axis while tracking.
bool SetLinearRotations(const Rotation& rotations, double horizontalSpeed, double verticalSpeed);
// Continue on game ticks until the silent rotation reaches the live camera.
void ReturnLinearRotations(double degreesPerSecond);
// Releases only a linear request; ordinary requests from other modules survive.
// Rebase the camera by whole turns at release, preserving its view and interpolation.
void ReleaseLinearRotations();
// A module supplies its smoothed angle; the manager applies the shared sensitivity grid.
bool SetDirectRotations(const Rotation& rotations);
void ReleaseDirectRotations();

void SetRotations(const Rotation& rotations, double rotationSpeed,
                  std::function<bool(const Rotation&)> raycast,
                  RotationPriority priority = RotationPriority::Medium,
                  bool randomize = true);

void Smooth();

Rotation GetRotation();

// Modern game-thread transaction: prediction, item-use and movement must use
// one exact unbounded angle, even if a module releases its rotation after use.
void BeginInteractionTick();
bool CanSubmitItemUse();
bool SubmitItemUse();
// Optional exclusive transaction: refuses a prior attack and blocks later
// native attacks until the owner releases its attack lease on the next tick.
bool SubmitItemUse(const void* owner);
Rotation CommitInteractionRotation();
void PublishInteractionMotion(float& yaw, float& pitch);
bool AutomatedItemUseSubmitted();
bool BlocksManualItemUse();
// Exclusive left-click transaction; native controller guard also covers manual
// clicks. Call EndAttackSubmission after the synchronous native call, even on failure.
bool AcquireAttack(const void* owner);
void ReleaseAttack(const void* owner);
bool CanSubmitAttack(const void* owner);
bool BeginAttackSubmission(const void* owner);
bool EndAttackSubmission();
bool CanCallNativeAttack();
bool EnterNativeAttack();
bool AttackReserved();

void SetActive(bool value);
bool IsActive();
void SetSmoothed(bool value);
bool IsSmoothed();
void SetMoveInputFix(bool value);
bool MoveInputFix();

float ClampPitch(float pitch);

void ClearRaycast();

void OnRespawn();

void UpdateGlobalYaw();

bool OnAnimation(float& yaw, float& lastYaw, float& pitch, float& lastPitch);

void OnMotionPre(float& yaw, float& pitch);

void OnMoveInput(MoveInput& input);

void OnRayTrace(const java::EntityHandle& entity, float& yaw, float& pitch);

void OnItemRayTrace(float& yaw, float& pitch);

void OnStrafe(float& yaw);

void OnJump(float& yaw);

void OnPositionItem(float& yaw, float& pitch);

void OnFallFlying(float& pitch);

void OnAttack(float& yaw);

void CorrectDisabledRotations();

}
