#include "minecraft/utils/rotation/RotationUtils.h"

#include <cmath>

#include "minecraft/utils/rotation/MathConst.h"
#include "minecraft/utils/rotation/Mth.h"
#include "minecraft/utils/rotation/Random.h"

namespace nova::rotation {

Rotation Calculate(const Vec3& from, const Vec3& to) {
    const Vec3 diff = to.Subtract(from);
    const double distance = std::hypot(diff.x, diff.z);
    const float yaw =
        static_cast<float>(std::atan2(diff.z, diff.x) * static_cast<double>(kToDegrees)) - 90.0f;
    const float pitch =
        static_cast<float>(-(std::atan2(diff.y, distance) * static_cast<double>(kToDegrees)));
    return Rotation{yaw, pitch};
}

Rotation CalculateEntityAim(const Vec3& eye, const Vec3& entityPos, double boxHeight,
                            double playerY, double playerEyeHeight) {
    const double offsetY = std::fmax(
        0.0, std::fmin(playerY - entityPos.y + playerEyeHeight, boxHeight * 0.9));
    return Calculate(eye, entityPos.Add(0.0, offsetY, 0.0));
}

Rotation ApplySensitivityPatch(const Rotation& rotation, const Rotation& previousRotation,
                               double sensitivity, bool randomize) {
    const float mouseSensitivity =
        static_cast<float>(sensitivity * (1.0 + (randomize ? Random() / 1000000.0 : 0.0)) * 0.6f + 0.2f);
    const double multiplier =
        mouseSensitivity * mouseSensitivity * mouseSensitivity * 8.0f * 0.15;

    const float yaw =
        previousRotation.yaw +
        static_cast<float>(mth::JavaRound((rotation.yaw - previousRotation.yaw) / multiplier) *
                           multiplier);
    const float pitch =
        previousRotation.pitch +
        static_cast<float>(mth::JavaRound((rotation.pitch - previousRotation.pitch) / multiplier) *
                           multiplier);
    return Rotation{yaw, mth::Clamp(pitch, -90.0f, 90.0f)};
}

Rotation RelateToPlayerRotation(const Rotation& rotation, const Rotation& previousRotation) {
    const float yaw = static_cast<float>(static_cast<double>(previousRotation.yaw) +
        mth::WrapDegrees(static_cast<double>(rotation.yaw) - previousRotation.yaw));
    const float pitch = mth::Clamp(rotation.pitch, -90.0f, 90.0f);
    return Rotation{yaw, pitch};
}

Rotation ResetRotation(const Rotation& rotation, float playerYaw, float playerPitch) {
    const float yaw = rotation.yaw + mth::WrapDegrees(playerYaw - rotation.yaw);
    return Rotation{yaw, playerPitch};
}

Rotation Move(const Rotation& lastRotation, const Rotation& targetRotation, double speed) {
    if (speed != 0.0) {
        const double deltaYaw = static_cast<double>(
            mth::WrapDegrees(targetRotation.yaw - lastRotation.yaw));
        const double deltaPitch =
            static_cast<double>(targetRotation.pitch - lastRotation.pitch);

        const double distance = std::sqrt(deltaYaw * deltaYaw + deltaPitch * deltaPitch);
        // Retaining an already aligned angle must not divide zero by zero.
        if (distance == 0.0)
            return Rotation{0.0f, 0.0f};
        const double distributionYaw = std::fabs(deltaYaw / distance);
        const double distributionPitch = std::fabs(deltaPitch / distance);

        const double maxYaw = speed * distributionYaw;
        const double maxPitch = speed * distributionPitch;

        const float moveYaw =
            static_cast<float>(mth::JavaMax(mth::JavaMin(deltaYaw, maxYaw), -maxYaw));
        const float movePitch =
            static_cast<float>(mth::JavaMax(mth::JavaMin(deltaPitch, maxPitch), -maxPitch));

        return Rotation{moveYaw, movePitch};
    }

    return Rotation{0.0f, 0.0f};
}

Rotation Smooth(const Rotation& lastRotation, const Rotation& targetRotation, double speed,
                const Rotation& previousRotation, double sensitivity, int fps,
                bool sensitivityAvailable, bool randomize) {
    float yaw = RelateToPlayerRotation(targetRotation, lastRotation).yaw;
    float pitch = targetRotation.pitch;

    if (speed != 0.0) {
        const Rotation move = Move(lastRotation, targetRotation, speed);

        yaw = lastRotation.yaw + move.yaw;
        pitch = lastRotation.pitch + move.pitch;

        // Caller-provided noise is already part of the target. Quantize once,
        // without adding a second random walk or an FPS-dependent iteration count.
        if (!randomize) {
            const auto fixed = sensitivityAvailable
                ? ApplySensitivityPatch({yaw, pitch}, previousRotation, sensitivity, false)
                : Rotation{yaw, pitch};
            return {fixed.yaw, mth::Clamp(fixed.pitch, -90.0f, 90.0f)};
        }

        const int iterations =
            static_cast<int>(static_cast<float>(fps) / 20.0f + Random() * 10.0);
        for (int i = 1; i <= iterations; ++i) {
            if (std::fabs(move.yaw) + std::fabs(move.pitch) > 0.0001f) {
                yaw += static_cast<float>((Random() - 0.5) / 1000.0);
                pitch -= static_cast<float>(Random() / 200.0);
            }

            const Rotation fixed =
                sensitivityAvailable
                    ? ApplySensitivityPatch(Rotation{yaw, pitch}, previousRotation, sensitivity)
                    : Rotation{yaw, pitch};
            yaw = fixed.yaw;
            pitch = mth::Clamp(fixed.pitch, -90.0f, 90.0f);
        }
    }

    return Rotation{yaw, pitch};
}

double GetDistance(const Vec3& eyes, const Vec3& boxMin, const Vec3& boxMax) {
    const double x = mth::Clamp(eyes.x, boxMin.x, boxMax.x);
    const double y = mth::Clamp(eyes.y, boxMin.y, boxMax.y);
    const double z = mth::Clamp(eyes.z, boxMin.z, boxMax.z);

    return std::sqrt(eyes.DistanceToSqr(x, y, z));
}

}
