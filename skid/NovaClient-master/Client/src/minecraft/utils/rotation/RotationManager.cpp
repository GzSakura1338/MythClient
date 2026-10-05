#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/AttackTransaction.h"

#include <cmath>
#include <utility>

#include "minecraft/java/api/game_rotation.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/utils/rotation/MathConst.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include "minecraft/utils/rotation/Mth.h"
#include "minecraft/utils/rotation/Random.h"
#include "minecraft/utils/rotation/RotationUtils.h"

namespace nova::rotation {
namespace {

RotationManagerState g_state;
AttackTransaction attack;
const void* exclusiveOwner=nullptr;
bool exclusiveMovementYawValid=false;
float exclusiveMovementYaw=0;
struct InteractionTick {
    bool enabled=false, locked=false, submitted=false, motion=false;
    Rotation angle;
} interaction;

bool PlayerRotation(float& yaw, float& pitch) {
    return java::PlayerRotation(yaw, pitch);
}

// Rise applies its sensitivity patch after smoothing. Extend that contract to
// Nova's linear/direct requests, using the last published silent angle as origin.
bool QuantizeRequest(const Rotation& target, Rotation& result) {
    double sensitivity = 0;
    if (!g_state.hasLastRotations || !java::PlayerSensitivity(sensitivity) ||
        !std::isfinite(sensitivity) || sensitivity < 0 || sensitivity > 1) return false;
    result = ApplySensitivityPatch(RelateToPlayerRotation(target, g_state.lastRotations),
        g_state.lastRotations, sensitivity, false);
    return std::isfinite(result.yaw) && std::isfinite(result.pitch);
}

}

RotationManagerState& State() {
    return g_state;
}

bool OwnsExclusiveRotations(const void* owner){return owner && exclusiveOwner==owner;}
bool SetExclusiveMovementYaw(const void* owner,float yaw){
    if(!OwnsExclusiveRotations(owner)||!std::isfinite(yaw))return false;
    exclusiveMovementYaw=yaw;exclusiveMovementYawValid=true;return true;
}
bool SetExclusiveRotations(const void* owner,const Rotation& target) {
    if(java::FreeCamActive())return false;
    if(!owner || (exclusiveOwner && exclusiveOwner!=owner) || !std::isfinite(target.yaw) || !std::isfinite(target.pitch))return false;
    if(!g_state.hasRotations || !g_state.hasLastRotations)UpdateGlobalYaw();
    if(!g_state.hasRotations)return false;
    Rotation applied;
    if(!QuantizeRequest(target,applied))return false;
    if(exclusiveOwner!=owner)exclusiveMovementYawValid=false;
    exclusiveOwner=owner;
    g_state.rotations=g_state.targetRotations=applied;
    g_state.hasTargetRotations=true;g_state.active=g_state.direct=g_state.smoothed=true;
    g_state.linear=g_state.linearReturning=false;g_state.linearRemainder={};g_state.moveInputFix=true;
    g_state.raycast=nullptr;g_state.hasRaycast=false;
    java::PickCursor(1.f);
    return true;
}
void ReleaseExclusiveRotations(const void* owner) {
    if(!OwnsExclusiveRotations(owner))return;
    exclusiveOwner=nullptr;exclusiveMovementYawValid=false;g_state.direct=false;g_state.linear=true;
    ReturnLinearRotations(720);
}

float ClampPitch(float pitch) {
    return mth::Clamp(pitch, -90.0f, 90.0f);
}

void ClearRaycast() {
    if(exclusiveOwner)return;
    g_state.raycast = nullptr;
    g_state.hasRaycast = false;
}

void SetRotations(const Rotation& rotations, double rotationSpeed) {
    SetRotations(rotations, rotationSpeed, nullptr);
}

void SetRotations(const Rotation& rotations, double rotationSpeed,
                  std::function<bool(const Rotation&)> raycast, RotationPriority priority, bool randomize) {
    if(java::FreeCamActive())return;
    if(exclusiveOwner)return;
    if (g_state.active && priority < g_state.priority) return;
    if (!std::isfinite(rotations.yaw) || !std::isfinite(rotations.pitch) ||
        !std::isfinite(rotationSpeed) || rotationSpeed < 0.0) return;
    // Movement policy belongs to the accepted request, not its previous owner.
    g_state.moveInputFix = true;
    g_state.randomize = randomize;
    if (!randomize) { g_state.offset = {}; g_state.randomAngle = 0; }
    g_state.linear = false;
    g_state.linearReturning = false;
    g_state.linearRemainder = {};
    g_state.priority = priority;
    g_state.direct = false;
    g_state.targetRotations = Rotation{rotations.yaw, ClampPitch(rotations.pitch)};
    g_state.hasTargetRotations = true;
    g_state.rotationSpeed = rotationSpeed;
    g_state.raycast = std::move(raycast);
    g_state.hasRaycast = g_state.raycast != nullptr;
    g_state.active = true;

    Smooth();
}

bool SetLinearRotations(const Rotation& rotations, double horizontalSpeed, double verticalSpeed) {
    if(java::FreeCamActive())return false;
    if(exclusiveOwner)return false;
    if (g_state.active && g_state.priority > RotationPriority::Medium) return false;
    if (!std::isfinite(rotations.yaw) || !std::isfinite(rotations.pitch) ||
        !std::isfinite(horizontalSpeed) || horizontalSpeed < 0.0 ||
        !std::isfinite(verticalSpeed) || verticalSpeed < 0.0) return false;
    if (!g_state.active || !g_state.hasRotations || !g_state.hasLastRotations) {
        float yaw = 0.0f, pitch = 0.0f;
        if (!PlayerRotation(yaw, pitch)) return false;
        g_state.rotations = g_state.lastRotations = {yaw, pitch};
        g_state.hasRotations = g_state.hasLastRotations = true;
        g_state.smoothed = false;
    }
    if (!g_state.linear || g_state.linearReturning) g_state.linearRemainder = {};
    g_state.moveInputFix = true;
    g_state.linear = true;
    g_state.linearReturning = false;
    g_state.direct = false;
    g_state.priority = RotationPriority::Medium;
    g_state.targetRotations = {rotations.yaw, ClampPitch(rotations.pitch)};
    g_state.hasTargetRotations = true;
    g_state.linearHorizontalSpeed = horizontalSpeed;
    g_state.linearVerticalSpeed = verticalSpeed;
    g_state.active = true;
    ClearRaycast();
    Smooth();
    return true;
}

void ReturnLinearRotations(double degreesPerSecond) {
    if(exclusiveOwner)return;
    if (!g_state.linear) return;
    float yaw = 0, pitch = 0;
    if (!g_state.active || !g_state.hasRotations || !PlayerRotation(yaw, pitch) ||
        !std::isfinite(yaw) || !std::isfinite(pitch) ||
        !std::isfinite(degreesPerSecond) || degreesPerSecond <= 0) {
        ReleaseLinearRotations();
        return;
    }
    g_state.linearReturning = true;
    g_state.linearRemainder = {};
    g_state.rotationSpeed = degreesPerSecond;
    g_state.targetRotations = {yaw, ClampPitch(pitch)};
    g_state.hasTargetRotations = true;
    ClearRaycast();
    // Preserve this tick's budget; the next game tick advances the return.
}

void ReleaseLinearRotations() {
    if(exclusiveOwner)return;
    if (!g_state.linear) return;
    SetActive(false);
    g_state.linear = false;
    g_state.linearReturning = false;
    g_state.linearRemainder = {};
    g_state.smoothed = false;
    g_state.hasRotations = g_state.hasLastRotations = g_state.hasTargetRotations = false;
    g_state.moveInputFix = true;
    ClearRaycast();
}

bool SetDirectRotations(const Rotation& rotations) {
    if(java::FreeCamActive())return false;
    if(exclusiveOwner)return false;
    if (g_state.active && g_state.priority > RotationPriority::Medium) return false;
    if (!std::isfinite(rotations.yaw) || !std::isfinite(rotations.pitch)) return false;
    if (!g_state.hasLastRotations) {
        float yaw = 0, pitch = 0;
        if (!PlayerRotation(yaw, pitch)) return false;
        g_state.lastRotations = {yaw, pitch};
        g_state.hasLastRotations = true;
    }
    Rotation applied;
    if (!QuantizeRequest(rotations, applied)) return false;
    g_state.moveInputFix = true;
    g_state.direct = true;
    g_state.linear = false;
    g_state.linearReturning = false;
    g_state.linearRemainder = {};
    g_state.priority = RotationPriority::Medium;
    g_state.rotations = g_state.targetRotations = applied;
    g_state.hasRotations = g_state.hasTargetRotations = true;
    g_state.active = g_state.smoothed = true;
    ClearRaycast();
    java::PickCursor(1.0f);
    return true;
}

void ReleaseDirectRotations() {
    if(exclusiveOwner)return;
    if (!g_state.direct) return;
    SetActive(false);
    g_state.direct = false;
    g_state.smoothed = false;
    g_state.hasRotations = g_state.hasLastRotations = g_state.hasTargetRotations = false;
    g_state.moveInputFix = true;
    ClearRaycast();
}

void Smooth() {
    if (g_state.linear && g_state.linearReturning) {
        float yaw = 0, pitch = 0;
        if (!PlayerRotation(yaw, pitch) || !std::isfinite(yaw) || !std::isfinite(pitch)) {
            ReleaseLinearRotations();
            return;
        }
        g_state.targetRotations = {yaw, ClampPitch(pitch)};
    }
    if (g_state.direct) { java::PickCursor(1.0f); return; }
    if (g_state.smoothed) {
        java::PickCursor(1.0f);
        return;
    }

    if (!g_state.hasTargetRotations || !g_state.hasRotations)
        return;

    // Keep target and current angles on the same revolution, including raycast offsets.
    float targetYaw = RelateToPlayerRotation(g_state.targetRotations, g_state.lastRotations).yaw;
    float targetPitch = ClampPitch(g_state.targetRotations.pitch);

    if (g_state.linear) {
        Rotation step = g_state.linearReturning
            ? LinearStep(g_state.lastRotations, {targetYaw, targetPitch}, g_state.rotationSpeed / 20.0)
            : AxisLinearStep(g_state.lastRotations, {targetYaw, targetPitch},
                             g_state.linearHorizontalSpeed / 20.0, g_state.linearVerticalSpeed / 20.0);
        if (!g_state.linearReturning) {
            // Carry sub-mouse-step movement so low speeds cannot stall forever.
            if (std::fabs(step.yaw - targetYaw) < .0001f) g_state.linearRemainder.yaw = 0;
            if (std::fabs(step.pitch - targetPitch) < .0001f) g_state.linearRemainder.pitch = 0;
            if (g_state.linearHorizontalSpeed > 0) step.yaw += g_state.linearRemainder.yaw;
            if (g_state.linearVerticalSpeed > 0) step.pitch += g_state.linearRemainder.pitch;
        }
        Rotation applied;
        if (QuantizeRequest(step, applied)) {
            if (!g_state.linearReturning) {
                if (g_state.linearHorizontalSpeed == 0) applied.yaw = g_state.lastRotations.yaw;
                if (g_state.linearVerticalSpeed == 0) applied.pitch = g_state.lastRotations.pitch;
                g_state.linearRemainder = {
                    g_state.linearHorizontalSpeed > 0 ? step.yaw - applied.yaw : 0.f,
                    g_state.linearVerticalSpeed > 0 ? ClampPitch(step.pitch) - applied.pitch : 0.f};
            }
            g_state.rotations = applied;
        }
        g_state.hasRotations = true;
        g_state.smoothed = true;
        java::PickCursor(1.0f);
        return;
    }

    if (g_state.randomize && g_state.hasRaycast &&
        (std::fabs(mth::WrapDegrees(targetYaw - g_state.rotations.yaw)) > 5.0f ||
         std::fabs(targetPitch - g_state.rotations.pitch) > 5.0f)) {
        const Rotation trueTargetRotations{targetYaw, targetPitch};

        const double speed = (Random() * Random() * Random()) * 20.0;
        const int tickCount = java::PlayerTickCount();

        g_state.randomAngle += static_cast<float>(
            (20.0 + (Random() - 0.5) * (Random() * Random() * Random() * 360.0)) *
            ((tickCount / 10 % 2 == 0) ? -1 : 1));

        g_state.offset.yaw = static_cast<float>(
            g_state.offset.yaw +
            static_cast<double>(-mth::Sin(ToRadians(g_state.randomAngle))) * speed);
        g_state.offset.pitch = static_cast<float>(
            g_state.offset.pitch +
            static_cast<double>(mth::Cos(ToRadians(g_state.randomAngle))) * speed);

        targetYaw += g_state.offset.yaw;
        targetPitch = ClampPitch(targetPitch + g_state.offset.pitch);

        if (!g_state.raycast(Rotation{targetYaw, targetPitch})) {
            g_state.randomAngle =
                static_cast<float>(ToDegrees(std::atan2(
                    static_cast<double>(trueTargetRotations.yaw - targetYaw),
                    static_cast<double>(targetPitch - trueTargetRotations.pitch)))) -
                180.0f;

            targetYaw -= g_state.offset.yaw;
            targetPitch -= g_state.offset.pitch;

            g_state.offset.yaw = static_cast<float>(
                g_state.offset.yaw +
                static_cast<double>(-mth::Sin(ToRadians(g_state.randomAngle))) * speed);
            g_state.offset.pitch = static_cast<float>(
                g_state.offset.pitch +
                static_cast<double>(mth::Cos(ToRadians(g_state.randomAngle))) * speed);

            targetYaw = targetYaw + g_state.offset.yaw;
            targetPitch = ClampPitch(targetPitch + g_state.offset.pitch);
        }

        if (!g_state.raycast(Rotation{targetYaw, targetPitch})) {
            g_state.offset.yaw = 0.0f;
            g_state.offset.pitch = 0.0f;

            targetYaw = static_cast<float>(trueTargetRotations.yaw + Random() * 2.0);
            targetPitch =
                ClampPitch(static_cast<float>(g_state.targetRotations.pitch + Random() * 2.0));
        }
    }

    float previousYaw = 0.0f;
    float previousPitch = 0.0f;
    const bool hasPreviousRotation = java::PlayerPreviousRotation(previousYaw, previousPitch);

    double sensitivity = 0.0;
    const bool hasSensitivity = java::PlayerSensitivity(sensitivity);

    const int fps = java::ClientFps();

    g_state.rotations = Smooth(g_state.lastRotations, Rotation{targetYaw, targetPitch},
                               g_state.rotationSpeed + (g_state.randomize ? Random() : 0.0),
                               Rotation{previousYaw, previousPitch}, sensitivity, fps,
                               hasPreviousRotation && hasSensitivity, g_state.randomize);
    Rotation applied;
    // Missing JNI sensitivity must not silently publish an unquantized angle.
    g_state.rotations = QuantizeRequest(g_state.rotations, applied) ? applied : g_state.lastRotations;

    g_state.hasRotations = true;
    g_state.smoothed = true;

    java::PickCursor(1.0f);
}

Rotation GetRotation() {
    if (interaction.locked) return interaction.angle;
    if (g_state.active)
        return g_state.rotations;

    float yaw = 0.0f;
    float pitch = 0.0f;
    if (!PlayerRotation(yaw, pitch))
        return Rotation{};
    return Rotation{yaw, pitch};
}

void BeginInteractionTick() { interaction = {}; interaction.enabled = true; attack.BeginTick(); }
bool CanSubmitItemUse() { return !interaction.enabled || (!attack.Reserved() && !interaction.locked && !interaction.motion); }
bool AcquireAttack(const void* owner) {
    return interaction.enabled&&!interaction.locked&&!interaction.motion&&attack.Acquire(owner);
}
void ReleaseAttack(const void* owner) {attack.Release(owner);}
bool CanSubmitAttack(const void* owner) {
    return interaction.enabled&&!interaction.locked&&!interaction.motion&&attack.Ready(owner);
}
bool BeginAttackSubmission(const void* owner) {
    if(!CanSubmitAttack(owner))return false;
    const auto angle=GetRotation();
    if(!std::isfinite(angle.yaw)||!std::isfinite(angle.pitch)||!attack.Begin(owner))return false;
    CommitInteractionRotation();return true;
}
bool EndAttackSubmission(){return attack.End();}
bool CanCallNativeAttack(){return attack.CanCall();}
bool EnterNativeAttack(){return attack.EnterNative();}
bool AttackReserved(){return attack.Reserved();}
Rotation CommitInteractionRotation() {
    const auto angle = interaction.motion ? interaction.angle : GetRotation();
    if (interaction.enabled && !interaction.locked) {
        interaction.angle = angle;
        interaction.locked = true;
    }
    return angle;
}
bool SubmitItemUse() {
    if (!CanSubmitItemUse()) return false;
    const auto angle = GetRotation();
    if (!std::isfinite(angle.yaw) || !std::isfinite(angle.pitch)) return false;
    if (interaction.enabled) { CommitInteractionRotation(); interaction.submitted = true; }
    return true;
}
bool SubmitItemUse(const void* owner) {
    if(!owner||!interaction.enabled||!CanSubmitItemUse())return false;
    const auto angle=GetRotation();
    if(!std::isfinite(angle.yaw)||!std::isfinite(angle.pitch)||!attack.Acquire(owner))return false;
    CommitInteractionRotation();interaction.submitted=true;return true;
}
void PublishInteractionMotion(float& yaw, float& pitch) {
    if (!interaction.enabled) return;
    if (interaction.locked) { yaw=interaction.angle.yaw; pitch=interaction.angle.pitch; }
    interaction.angle = {yaw,pitch}; interaction.motion = true;
}
bool AutomatedItemUseSubmitted() { return interaction.enabled && interaction.submitted; }
bool BlocksManualItemUse() { return interaction.enabled && (interaction.submitted || exclusiveOwner || attack.Reserved()); }

void SetActive(bool value) {
    if(exclusiveOwner)return;
    if (!value && g_state.active) CorrectDisabledRotations();
    g_state.active = value;
    if (!value) g_state.priority = RotationPriority::Medium;
}

bool IsActive() {
    return g_state.active || interaction.locked;
}

void SetSmoothed(bool value) {
    g_state.smoothed = value;
}

bool IsSmoothed() {
    return g_state.smoothed;
}

void SetMoveInputFix(bool value) {
    if(exclusiveOwner)return;
    g_state.moveInputFix = value;
}

bool MoveInputFix() {
    return g_state.moveInputFix;
}

void OnRespawn() {
    attack = {};
    const bool modern=interaction.enabled; interaction = {}; interaction.enabled=modern;
    // A new player/world establishes a new yaw origin; never rebase it to the old body.
    exclusiveOwner = nullptr;
    exclusiveMovementYawValid=false;
    g_state = {};
}

void UpdateGlobalYaw() {
    if(java::FreeCamActive()){exclusiveOwner=nullptr;exclusiveMovementYawValid=false;g_state={};return;}
    float yaw = 0.0f;
    float pitch = 0.0f;
    if (!PlayerRotation(yaw, pitch) || !std::isfinite(yaw) || !std::isfinite(pitch))
        return;

    if (!g_state.active || !g_state.hasRotations || !g_state.hasLastRotations ||
        !g_state.hasTargetRotations) {
        const Rotation current{yaw, pitch};
        g_state.rotations = current;
        g_state.lastRotations = current;
        g_state.targetRotations = current;
        g_state.hasRotations = true;
        g_state.hasLastRotations = true;
        g_state.hasTargetRotations = true;
    }

    if (g_state.active)
        Smooth();
}

bool OnAnimation(float& yaw, float& lastYaw, float& pitch, float& lastPitch) {
    if (g_state.active && g_state.hasAnimation && g_state.hasLastAnimation) {
        yaw = g_state.animationRotation.yaw;
        lastYaw = g_state.lastAnimationRotation.yaw;
        pitch = g_state.animationRotation.pitch;
        lastPitch = g_state.lastAnimationRotation.pitch;
        return true;
    }
    return false;
}

void OnMotionPre(float& yaw, float& pitch) {
    if(java::FreeCamActive()){exclusiveOwner=nullptr;exclusiveMovementYawValid=false;g_state={};return;}
    if (interaction.locked) {
        // Completion/return requests cannot change this tick's submitted use.
        yaw = interaction.angle.yaw; pitch = interaction.angle.pitch;
        g_state.lastRotations = interaction.angle; g_state.hasLastRotations = true;
        g_state.lastAnimationRotation = g_state.animationRotation;
        g_state.hasLastAnimation = g_state.hasAnimation;
        g_state.animationRotation = interaction.angle; g_state.hasAnimation = true;
        g_state.smoothed = false;
        g_state.priority = RotationPriority::Medium;
        return;
    }
    bool returned = false;
    bool completed = false;
    if (g_state.active && g_state.hasRotations) {
        const float outYaw = g_state.rotations.yaw;
        const float outPitch = ClampPitch(g_state.rotations.pitch);
        if (std::isfinite(outYaw) && std::isfinite(outPitch)) {
            yaw = outYaw;
            pitch = outPitch;
        } else {
            exclusiveOwner = nullptr;
            exclusiveMovementYawValid=false;
            g_state = {};
            return;
        }

        float playerYaw = 0.0f;
        float playerPitch = 0.0f;
        if (PlayerRotation(playerYaw, playerPitch)) {
            if (g_state.linear) {
                Rotation returnTarget;
                returned = g_state.linearReturning &&
                    QuantizeRequest({playerYaw,playerPitch},returnTarget) &&
                    std::fabs(g_state.rotations.yaw - returnTarget.yaw) < 0.0001f &&
                    std::fabs(g_state.rotations.pitch - returnTarget.pitch) < 0.0001f;
            } else if (!exclusiveOwner && std::fabs(mth::WrapDegrees(g_state.rotations.yaw - playerYaw)) < 1.0f &&
                std::fabs(g_state.rotations.pitch - playerPitch) < 1.0f) {
                completed = true;
            }
        }

        g_state.lastRotations = g_state.rotations;
        g_state.hasLastRotations = true;
        // Rebase against this motion's output, not the preceding tick's angle.
        if (completed) SetActive(false);
    } else {
        float playerYaw = 0.0f;
        float playerPitch = 0.0f;
        if (PlayerRotation(playerYaw, playerPitch)) {
            g_state.lastRotations = Rotation{playerYaw, playerPitch};
            g_state.hasLastRotations = true;
        }
    }

    g_state.lastAnimationRotation = g_state.animationRotation;
    g_state.hasLastAnimation = g_state.hasAnimation;
    g_state.animationRotation = Rotation{yaw, pitch};
    g_state.hasAnimation = true;

    float playerYaw = 0.0f;
    float playerPitch = 0.0f;
    if (!exclusiveOwner && (!g_state.linear || g_state.linearReturning) && PlayerRotation(playerYaw, playerPitch)) {
        g_state.targetRotations = Rotation{playerYaw, playerPitch};
        g_state.hasTargetRotations = true;
    }

    ClearRaycast();
    g_state.smoothed = false;
    g_state.priority = RotationPriority::Medium;
    if (returned) ReleaseLinearRotations();
}

void OnMoveInput(MoveInput& input) {
    if ((!interaction.locked && (!g_state.active || !g_state.hasRotations)) || !g_state.moveInputFix)
        return;

    float playerYaw = 0.0f;
    float playerPitch = 0.0f;
    if (!PlayerRotation(playerYaw, playerPitch))
        return;

    FixMovement(input, GetRotation().yaw,
        exclusiveOwner && exclusiveMovementYawValid ? exclusiveMovementYaw : playerYaw);
}

void OnRayTrace(const java::EntityHandle& entity, float& yaw, float& pitch) {
    if (!g_state.hasRotations || !g_state.active)
        return;
    if (!entity.Valid())
        return;

    java::EntityHandle player = java::PlayerEntity();
    if (!player.Valid() || !entity.IsSame(player))
        return;

    const auto angle=GetRotation(); yaw=angle.yaw; pitch=angle.pitch;
}

void OnItemRayTrace(float& yaw, float& pitch) {
    if (interaction.locked) { yaw=interaction.angle.yaw; pitch=interaction.angle.pitch; return; }
    if (!g_state.hasRotations || !g_state.active)
        return;

    yaw = g_state.rotations.yaw;
    pitch = g_state.rotations.pitch;
}

void OnStrafe(float& yaw) {
    if (interaction.locked) { yaw=interaction.angle.yaw; return; }
    if (!g_state.active || !g_state.hasRotations)
        return;

    yaw = g_state.rotations.yaw;
}

void OnJump(float& yaw) {
    if (interaction.locked) { yaw=interaction.angle.yaw; return; }
    if (!g_state.active || !g_state.hasRotations)
        return;

    yaw = g_state.rotations.yaw;
}

void OnPositionItem(float& yaw, float& pitch) {
    if (interaction.locked) { yaw=interaction.angle.yaw; pitch=interaction.angle.pitch; return; }
    if (!g_state.active || !g_state.hasRotations)
        return;

    yaw = g_state.rotations.yaw;
    pitch = ClampPitch(g_state.rotations.pitch);
}

void OnFallFlying(float& pitch) {
    if (interaction.locked) { pitch=interaction.angle.pitch; return; }
    if (!g_state.hasRotations)
        return;

    pitch = g_state.rotations.pitch;
}

void OnAttack(float& yaw) {
    if (interaction.locked) { yaw=interaction.angle.yaw; return; }
    if (!g_state.hasRotations)
        return;

    yaw = g_state.rotations.yaw;
}

void CorrectDisabledRotations() {
    if(exclusiveOwner || java::FreeCamActive())return;
    float playerYaw = 0.0f;
    float playerPitch = 0.0f;
    if (!PlayerRotation(playerYaw, playerPitch))
        return;
    if (!g_state.hasLastRotations || !std::isfinite(g_state.lastRotations.yaw) ||
        !std::isfinite(playerYaw) || !std::isfinite(playerPitch))
        return;

    // Preserve the live view exactly; only change its whole-turn representation.
    // Otherwise e.g. silent 361 -> camera 1 produces a spurious -360 packet delta.
    const double turns = std::round((static_cast<double>(g_state.lastRotations.yaw) - playerYaw) / 360.0);
    if (turns == 0.0) return;
    const float yaw = static_cast<float>(static_cast<double>(playerYaw) + turns * 360.0);
    if (!std::isfinite(yaw)) return;
    // Shift both interpolation endpoints so render frames never spin through a full turn.
    if (!java::SetPlayerCameraRotation(yaw, playerPitch, 100.0))
        java::SetPlayerRotation(yaw, playerPitch);
}

}
