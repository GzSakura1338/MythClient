#include "minecraft/utils/rotation/MovementFix.h"

#include <cmath>

#include "minecraft/utils/rotation/Mth.h"

namespace nova::rotation {

float Direction(float forward, float strafe, float playerYaw) {
    const float direction = playerYaw;
    const bool isMovingForward = forward > 0.0f;
    const bool isMovingBack = forward < 0.0f;
    const bool isMovingRight = strafe > 0.0f;
    const bool isMovingLeft = strafe < 0.0f;
    const bool isMovingSideways = isMovingRight || isMovingLeft;
    const bool isMovingStraight = isMovingForward || isMovingBack;

    if (forward != 0.0f || strafe != 0.0f) {
        if (isMovingBack && !isMovingSideways)
            return direction + 180.0f;

        if (isMovingForward && isMovingLeft)
            return direction + 45.0f;

        if (isMovingForward && isMovingRight)
            return direction - 45.0f;

        if (!isMovingStraight && isMovingLeft)
            return direction + 90.0f;

        if (!isMovingStraight && isMovingRight)
            return direction - 90.0f;

        if (isMovingBack && isMovingLeft)
            return direction + 135.0f;

        if (isMovingBack)
            return direction - 135.0f;
    }

    return direction;
}

void FixMovement(MoveInput& input, float yaw, float playerYaw) {
    const int angleUnit = 45;
    const float angleTolerance = 22.5f;

    const float directionFactor = std::fmax(std::fabs(input.forward), std::fabs(input.strafe));
    const double angleDifference =
        static_cast<double>(mth::WrapDegrees(Direction(input.forward, input.strafe, playerYaw) - yaw));
    const double angleDistance = std::fabs(angleDifference);

    float forward = 0.0f;
    float strafe = 0.0f;

    if (angleDistance <= static_cast<double>(static_cast<float>(angleUnit) + angleTolerance))
        forward += 1.0f;
    else if (angleDistance >= static_cast<double>(180.0f - static_cast<float>(angleUnit) - angleTolerance))
        forward -= 1.0f;

    if (angleDifference >= static_cast<double>(static_cast<float>(angleUnit) - angleTolerance) &&
        angleDifference <= static_cast<double>(180.0f - static_cast<float>(angleUnit) + angleTolerance))
        strafe -= 1.0f;
    else if (angleDifference <= static_cast<double>(static_cast<float>(-angleUnit) + angleTolerance) &&
             angleDifference >= static_cast<double>(-180.0f + static_cast<float>(angleUnit) - angleTolerance))
        strafe += 1.0f;

    forward *= directionFactor;
    strafe *= directionFactor;

    input.forward = forward;
    input.strafe = strafe;
}

}
