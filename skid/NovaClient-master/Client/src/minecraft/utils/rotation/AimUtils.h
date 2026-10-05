#pragma once

#include <vector>

#include "minecraft/java/api/game_entity.h"
#include "minecraft/utils/rotation/Rotation.h"
#include "minecraft/utils/rotation/Vec3.h"

namespace nova::rotation {

struct AimRotations {
    float yaw = 0.0f;
    float pitch = 0.0f;
};

struct AimFrame {
    Vec3 eye;
    Vec3 feet;
    double eyeHeight = 0.0;
    bool valid = false;
};

AimFrame ReadAimFrame();

float ClampAimPitch(float pitch);

float AimAngle(double x, double z);

float AimYawTo(const Vec3& from, const Vec3& to);

float AimPitchTo(const Vec3& from, double fromEyeHeight, const Vec3& to, double toEyeHeight);

bool InFov(float viewYaw, float fov, float targetYaw);

double AimYawDifference(float viewYaw, const Vec3& from, const Vec3& to);

double AimPitchDifference(float viewPitch, const Vec3& from, double fromEyeHeight, const Vec3& to,
                          double toEyeHeight);

Vec3 ClosestPointOnAabb(const java::Box& box, const Vec3& point);

double DistanceSqFromEyeToClosestOnAABB(const java::EntityHandle& entity);

std::vector<Vec3> BuildBackupPoints(const java::EntityHandle& entity, const Vec3& eye);

bool GetAimPoint(const java::EntityHandle& entity, double horizontalMultipoint,
                 double verticalMultipoint, const Vec3& eye, Vec3& out);

AimRotations RotationsToPoint(const AimFrame& frame, const Vec3& point, float baseYaw,
                              float basePitch);

bool CanAimAtPoint(const java::EntityHandle& target, const Vec3& eye, const Vec3& point,
                   double range, bool allowThroughBlocks, bool allowThroughEntities);

bool HasValidAimPoint(const java::EntityHandle& entity, double horizontalMultipoint,
                      double verticalMultipoint, double range, bool allowThroughBlocks,
                      bool allowThroughEntities);

bool GetRotationsWithBackup(const java::EntityHandle& entity, double horizontalMultipoint,
                            double verticalMultipoint, float baseYaw, float basePitch, double range,
                            bool allowThroughBlocks, bool allowThroughEntities, AimRotations& out);

AimRotations SmoothRotation(float baseYaw, float basePitch, float targetYaw, float targetPitch,
                            int speed, float randomizationPercent);

}
