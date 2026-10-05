#include "minecraft/utils/rotation/FallingPlayer.h"

#include <cmath>

#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/utils/rotation/MathConst.h"
#include "minecraft/utils/rotation/Mth.h"

namespace nova::rotation {
namespace {

// The two drag constants the game applies to a falling entity each tick.
constexpr double kVerticalDrag = 0.9800000190734863;
constexpr float kInputDrag = 0.9800000190734863f;
constexpr double kHorizontalDrag = 0.91;
constexpr double kGravity = 0.08;

// Below this the input vector is treated as zero and the acceleration step is
// skipped entirely, which is the case Scaffold always hits -- it passes no input.
constexpr float kInputEpsilon = 1.0E-4f;

}

void FallingPlayer::CalculateForTick() {
    float sr = strafe * kInputDrag;
    float fw = forward * kInputDrag;
    float movement = sr * sr + fw * fw;

    if (movement >= kInputEpsilon) {
        movement = std::sqrt(movement);
        if (movement < 1.0f)
            movement = 1.0f;

        float factor = jumpMovementFactor;
        bool sprinting = false;
        if (java::PlayerSprinting(sprinting) && sprinting)
            factor *= 1.3f;

        movement = factor / movement;
        sr *= movement;
        fw *= movement;

        const float radians = yaw * kToRadians;
        const float sin = mth::Sin(radians);
        const float cos = mth::Cos(radians);
        motionX += static_cast<double>(sr * cos - fw * sin);
        motionZ += static_cast<double>(fw * cos + sr * sin);
    }

    motionY -= kGravity;
    motionY *= kVerticalDrag;
    x += motionX;
    y += motionY;
    z += motionZ;
    motionX *= kHorizontalDrag;
    motionZ *= kHorizontalDrag;
}

void FallingPlayer::Calculate(int ticks) {
    for (int i = 0; i < ticks; ++i)
        CalculateForTick();
}

bool MakeFallingPlayer(FallingPlayer& out) {
    java::EntityHandle player = java::PlayerEntity();
    if (!player.Valid())
        return false;

    Vec3 position;
    if (!player.Position(position))
        return false;

    Vec3 motion;
    if (!java::PlayerDeltaMovement(motion))
        return false;

    float yaw = 0.0f;
    float pitch = 0.0f;
    if (!java::PlayerRotation(yaw, pitch))
        return false;

    float speed = 0.0f;
    if (!java::PlayerSpeed(speed))
        return false;

    out.x = position.x;
    out.y = position.y;
    out.z = position.z;
    out.motionX = motion.x;
    out.motionY = motion.y;
    out.motionZ = motion.z;
    out.yaw = yaw;
    out.strafe = 0.0f;
    out.forward = 0.0f;
    out.jumpMovementFactor = speed;
    return true;
}

}
