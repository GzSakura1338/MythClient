#pragma once

#include "minecraft/utils/rotation/Rotation.h"
#include "minecraft/utils/rotation/Vec3.h"

namespace nova::rotation {

Rotation Calculate(const Vec3& from, const Vec3& to);

Rotation CalculateEntityAim(const Vec3& eye, const Vec3& entityPos, double boxHeight,
                            double playerY, double playerEyeHeight);

Rotation ApplySensitivityPatch(const Rotation& rotation, const Rotation& previousRotation,
                               double sensitivity, bool randomize = true);

Rotation RelateToPlayerRotation(const Rotation& rotation, const Rotation& previousRotation);

Rotation ResetRotation(const Rotation& rotation, float playerYaw, float playerPitch);

Rotation Move(const Rotation& lastRotation, const Rotation& targetRotation, double speed);

Rotation Smooth(const Rotation& lastRotation, const Rotation& targetRotation, double speed,
                const Rotation& previousRotation, double sensitivity, int fps,
                bool sensitivityAvailable, bool randomize = true);

double GetDistance(const Vec3& eyes, const Vec3& boxMin, const Vec3& boxMax);

}
