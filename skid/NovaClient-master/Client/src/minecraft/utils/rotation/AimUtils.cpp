#include "minecraft/utils/rotation/AimUtils.h"

#include <algorithm>
#include <cmath>

#include "minecraft/utils/rotation/Mth.h"
#include "minecraft/utils/rotation/Random.h"

namespace nova::rotation {
namespace {

constexpr double kToDegrees = 57.295780181884766;
constexpr double kBackupFaceInset = 0.05;
constexpr int kBackupTargetTotal = 30;
constexpr float kFarThreshold = 180.0f;

void AddFaceGrid(std::vector<Vec3>& out, int fixedAxis, double fixedValue, double uMin, double uMax,
                 double vMin, double vMax, int targetPoints, double dimU, double dimV) {
    if (dimU < 1e-4 || dimV < 1e-4) {
        const double uMid = (uMin + uMax) / 2.0;
        const double vMid = (vMin + vMax) / 2.0;
        switch (fixedAxis) {
            case 0: out.push_back(Vec3{fixedValue, uMid, vMid}); break;
            case 1: out.push_back(Vec3{uMid, fixedValue, vMid}); break;
            case 2: out.push_back(Vec3{uMid, vMid, fixedValue}); break;
            default: break;
        }
        return;
    }

    const double ratio = dimU / dimV;
    const int gridU = std::max(2, static_cast<int>(std::lround(std::sqrt(targetPoints * ratio))));
    const int gridV = std::max(2, static_cast<int>(std::lround(std::sqrt(targetPoints / ratio))));

    for (int i = 0; i < gridU; ++i) {
        const double u = uMin + (uMax - uMin) * static_cast<double>(i) / (gridU - 1);
        for (int j = 0; j < gridV; ++j) {
            const double v = vMin + (vMax - vMin) * static_cast<double>(j) / (gridV - 1);
            switch (fixedAxis) {
                case 0: out.push_back(Vec3{fixedValue, u, v}); break;
                case 1: out.push_back(Vec3{u, fixedValue, v}); break;
                case 2: out.push_back(Vec3{u, v, fixedValue}); break;
                default: break;
            }
        }
    }
}

bool ExpandedBox(const java::EntityHandle& entity, java::Box& out) {
    double border = 0.0;
    if (!entity.PickRadius(border))
        border = 0.0;
    java::Box box;
    if (!entity.BoundingBox(box))
        return false;
    out = java::BoxInflate(box, border);
    return true;
}

bool MainRayHitsTargetAabb(const Vec3& eye, const Vec3& point,
                           const java::EntityHandle& target, double range) {
    const double dx = point.x - eye.x;
    const double dy = point.y - eye.y;
    const double dz = point.z - eye.z;
    const double len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 1e-6)
        return false;
    const double scale = range / len;
    const Vec3 end = Vec3{eye.x + dx * scale, eye.y + dy * scale, eye.z + dz * scale};

    java::Box box;
    if (!ExpandedBox(target, box))
        return false;

    bool hit = false;
    Vec3 hitVec;
    if (!java::BoxClip(box, eye, end, hit, hitVec))
        return false;
    return hit;
}

bool HasEntityBlockingPath(const Vec3& eye, const Vec3& end, const java::EntityHandle& target,
                           double targetDistSq) {
    java::EntityHandle player = java::PlayerEntity();
    if (!player.Valid())
        return false;

    java::Box playerBox;
    if (!player.BoundingBox(playerBox))
        return false;

    const Vec3 delta = end.Subtract(eye);
    const java::Box searchBox = java::BoxInflate(java::BoxExpandTowards(playerBox, delta), 1.0);

    std::vector<java::EntityHandle> entities;
    if (!java::WorldEntities(player, searchBox, entities))
        return false;

    for (const java::EntityHandle& entity : entities) {
        if (!entity.Valid() || entity.IsSame(target))
            continue;

        if (entity.IsSpectator())
            continue;

        java::Box box;
        if (!ExpandedBox(entity, box))
            continue;

        if (java::BoxContains(box, eye))
            return true;

        bool hit = false;
        Vec3 hitVec;
        if (!java::BoxClip(box, eye, end, hit, hitVec))
            continue;
        if (!hit)
            continue;

        const double entityDistSq = eye.DistanceToSqr(hitVec);
        if (entityDistSq < targetDistSq - 1.0E-7)
            return true;
    }

    return false;
}

}

AimFrame ReadAimFrame() {
    AimFrame frame;
    java::EntityHandle player = java::PlayerEntity();
    if (!player.Valid())
        return frame;

    if (!player.Position(frame.feet) || !player.EyePosition(1.0f, frame.eye) ||
        !player.EyeHeight(frame.eyeHeight))
        return frame;

    frame.valid = true;
    return frame;
}

float ClampAimPitch(float pitch) {
    return mth::Clamp(pitch, -90.0f, 90.0f);
}

float AimAngle(double x, double z) {
    java::EntityHandle player = java::PlayerEntity();
    if (!player.Valid())
        return 0.0f;
    Vec3 pos;
    if (!player.Position(pos))
        return 0.0f;
    return static_cast<float>(std::atan2(x - pos.x, z - pos.z) * kToDegrees * -1.0);
}

float AimYawTo(const Vec3& from, const Vec3& to) {
    const double x = to.x - from.x;
    const double z = to.z - from.z;
    return static_cast<float>(std::atan2(x, z) * 57.29577951308232 * -1.0);
}

float AimPitchTo(const Vec3& from, double fromEyeHeight, const Vec3& to, double toEyeHeight) {
    const double x = to.x - from.x;
    const double z = to.z - from.z;
    const double y = to.y + toEyeHeight / 2.0 - (from.y + fromEyeHeight);
    return static_cast<float>(std::atan2(y, std::sqrt(x * x + z * z)) * 57.29577951308232 * -1.0);
}

bool InFov(float viewYaw, float fov, float targetYaw) {
    const float half = fov * 0.5f;
    const double wrapped =
        mth::WrapDegrees(static_cast<double>(std::fmod(viewYaw - targetYaw, 360.0f)));
    if (wrapped > 0.0)
        return wrapped < half;
    return wrapped > -half;
}

double AimYawDifference(float viewYaw, const Vec3& from, const Vec3& to) {
    const double raw = static_cast<double>(viewYaw - AimYawTo(from, to));
    return std::fmod(std::fmod(raw, 360.0) + 540.0, 360.0) - 180.0;
}

double AimPitchDifference(float viewPitch, const Vec3& from, double fromEyeHeight, const Vec3& to,
                          double toEyeHeight) {
    const double raw = static_cast<double>(
        viewPitch - AimPitchTo(from, fromEyeHeight, to, toEyeHeight));
    return std::fmod(std::fmod(raw, 360.0) + 540.0, 360.0) - 180.0;
}

Vec3 ClosestPointOnAabb(const java::Box& box, const Vec3& point) {
    const double x = std::max(box.minX, std::min(box.maxX, point.x));
    const double y = std::max(box.minY, std::min(box.maxY, point.y));
    const double z = std::max(box.minZ, std::min(box.maxZ, point.z));
    return Vec3{x, y, z};
}

double DistanceSqFromEyeToClosestOnAABB(const java::EntityHandle& entity) {
    if (!entity.Valid())
        return 1.7976931348623157E308;

    java::EntityHandle player = java::PlayerEntity();
    if (!player.Valid())
        return 1.7976931348623157E308;

    Vec3 eye;
    if (!player.EyePosition(1.0f, eye))
        return 1.7976931348623157E308;

    java::Box box;
    if (!ExpandedBox(entity, box))
        return 1.7976931348623157E308;

    return eye.DistanceToSqr(ClosestPointOnAabb(box, eye));
}

std::vector<Vec3> BuildBackupPoints(const java::EntityHandle& entity, const Vec3& eye) {
    std::vector<Vec3> points;
    if (!entity.Valid())
        return points;

    java::Box bb;
    if (!ExpandedBox(entity, bb))
        return points;

    const double sizeX = bb.maxX - bb.minX;
    const double sizeY = bb.maxY - bb.minY;
    const double sizeZ = bb.maxZ - bb.minZ;

    const bool xPos = eye.x > bb.maxX;
    const bool xNeg = eye.x < bb.minX;
    const bool yPos = eye.y > bb.maxY;
    const bool yNeg = eye.y < bb.minY;
    const bool zPos = eye.z > bb.maxZ;
    const bool zNeg = eye.z < bb.minZ;

    const int visibleFaceCount = (xPos || xNeg ? 1 : 0) + (yPos || yNeg ? 1 : 0) +
                                 (zPos || zNeg ? 1 : 0);
    if (visibleFaceCount == 0)
        return points;

    const int pointsPerFace = kBackupTargetTotal / visibleFaceCount;
    points.reserve(static_cast<std::size_t>(kBackupTargetTotal + 6));

    if (xPos || xNeg) {
        const double fixedX = xPos ? bb.maxX - kBackupFaceInset : bb.minX + kBackupFaceInset;
        AddFaceGrid(points, 0, fixedX, bb.minY + kBackupFaceInset, bb.maxY - kBackupFaceInset,
                    bb.minZ + kBackupFaceInset, bb.maxZ - kBackupFaceInset, pointsPerFace, sizeY,
                    sizeZ);
    }

    if (yPos || yNeg) {
        const double fixedY = yPos ? bb.maxY - kBackupFaceInset : bb.minY + kBackupFaceInset;
        AddFaceGrid(points, 1, fixedY, bb.minX + kBackupFaceInset, bb.maxX - kBackupFaceInset,
                    bb.minZ + kBackupFaceInset, bb.maxZ - kBackupFaceInset, pointsPerFace, sizeX,
                    sizeZ);
    }

    if (zPos || zNeg) {
        const double fixedZ = zPos ? bb.maxZ - kBackupFaceInset : bb.minZ + kBackupFaceInset;
        AddFaceGrid(points, 2, fixedZ, bb.minX + kBackupFaceInset, bb.maxX - kBackupFaceInset,
                    bb.minY + kBackupFaceInset, bb.maxY - kBackupFaceInset, pointsPerFace, sizeX,
                    sizeY);
    }

    return points;
}

bool GetAimPoint(const java::EntityHandle& entity, double horizontalMultipoint,
                 double verticalMultipoint, const Vec3& eye, Vec3& out) {
    if (!entity.Valid())
        return false;

    java::Box bb;
    if (!ExpandedBox(entity, bb))
        return false;

    const double centerX = (bb.minX + bb.maxX) / 2.0;
    const double centerZ = (bb.minZ + bb.maxZ) / 2.0;

    Vec3 basePos;
    double eyeHeight = 0.0;
    if (!entity.Position(basePos) || !entity.EyeHeight(eyeHeight))
        return false;
    const double centerY = basePos.y + eyeHeight;

    if (java::BoxContains(bb, eye)) {
        out = Vec3{centerX, eye.y, centerZ};
        return true;
    }

    const Vec3 closest = ClosestPointOnAabb(bb, eye);
    const double tH = std::max(0.0, std::min(1.0, horizontalMultipoint / 100.0));
    const double tV = std::max(0.0, std::min(1.0, verticalMultipoint / 100.0));

    out = Vec3{centerX + (closest.x - centerX) * tH, centerY + (closest.y - centerY) * tV,
               centerZ + (closest.z - centerZ) * tH};
    return true;
}

AimRotations RotationsToPoint(const AimFrame& frame, const Vec3& point, float baseYaw,
                              float basePitch) {
    const double deltaX = point.x - frame.feet.x;
    const double deltaZ = point.z - frame.feet.z;
    const double deltaY = point.y - (frame.feet.y + frame.eyeHeight);
    const double horizDistSq = deltaX * deltaX + deltaZ * deltaZ;

    float yaw = 0.0f;
    float targetPitch = 0.0f;
    if (horizDistSq < 1.0E-12) {
        yaw = baseYaw;
        targetPitch = static_cast<float>(-(std::atan2(deltaY, 0.0) * kToDegrees));
    } else {
        const float targetYaw =
            static_cast<float>(std::atan2(deltaZ, deltaX) * kToDegrees) - 90.0f;
        yaw = baseYaw + mth::WrapDegrees(targetYaw - baseYaw);
        const double horizDist = std::sqrt(horizDistSq);
        targetPitch = static_cast<float>(-(std::atan2(deltaY, horizDist) * kToDegrees));
    }

    const float pitch = basePitch + mth::WrapDegrees(targetPitch - basePitch) + 3.0f;
    return AimRotations{yaw, ClampAimPitch(pitch)};
}

bool CanAimAtPoint(const java::EntityHandle& target, const Vec3& eye, const Vec3& point,
                   double range, bool allowThroughBlocks, bool allowThroughEntities) {
    if (!target.Valid())
        return false;

    const double dx = point.x - eye.x;
    const double dy = point.y - eye.y;
    const double dz = point.z - eye.z;
    const double len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 1e-6)
        return false;
    const double scale = range / len;
    const Vec3 end = Vec3{eye.x + dx * scale, eye.y + dy * scale, eye.z + dz * scale};

    java::Box box;
    if (!ExpandedBox(target, box))
        return false;

    bool hit = false;
    Vec3 hitVec;
    if (!java::BoxClip(box, eye, end, hit, hitVec))
        return false;
    if (!hit)
        return false;

    const double entityDistSq = eye.DistanceToSqr(hitVec);

    if (!allowThroughBlocks) {
        java::BlockHitResultInfo blockHit;
        if (java::WorldClip(eye, end, java::PlayerEntity(), blockHit) &&
            blockHit.kind == java::HitKind::Block) {
            const double blockDistSq = eye.DistanceToSqr(blockHit.location);
            if (blockDistSq < entityDistSq)
                return false;
        }
    }

    if (!allowThroughEntities && HasEntityBlockingPath(eye, end, target, entityDistSq))
        return false;

    return true;
}

bool HasValidAimPoint(const java::EntityHandle& entity, double horizontalMultipoint,
                      double verticalMultipoint, double range, bool allowThroughBlocks,
                      bool allowThroughEntities) {
    if (!entity.Valid())
        return false;

    java::EntityHandle player = java::PlayerEntity();
    if (!player.Valid())
        return false;

    Vec3 eye;
    if (!player.EyePosition(1.0f, eye))
        return false;

    Vec3 mainPoint;
    if (!GetAimPoint(entity, horizontalMultipoint, verticalMultipoint, eye, mainPoint))
        return false;

    if (eye.DistanceToSqr(mainPoint) < 1e-6)
        return true;

    if (!MainRayHitsTargetAabb(eye, mainPoint, entity, range))
        return false;

    if (CanAimAtPoint(entity, eye, mainPoint, range, allowThroughBlocks, allowThroughEntities))
        return true;

    std::vector<Vec3> backups = BuildBackupPoints(entity, eye);
    std::stable_sort(backups.begin(), backups.end(), [&eye](const Vec3& a, const Vec3& b) {
        return eye.DistanceToSqr(a) < eye.DistanceToSqr(b);
    });

    for (const Vec3& point : backups) {
        if (CanAimAtPoint(entity, eye, point, range, allowThroughBlocks, allowThroughEntities))
            return true;
    }

    return false;
}

bool GetRotationsWithBackup(const java::EntityHandle& entity, double horizontalMultipoint,
                            double verticalMultipoint, float baseYaw, float basePitch, double range,
                            bool allowThroughBlocks, bool allowThroughEntities,
                            AimRotations& out) {
    if (!entity.Valid())
        return false;

    const AimFrame frame = ReadAimFrame();
    if (!frame.valid)
        return false;

    java::Box bb;
    if (!ExpandedBox(entity, bb))
        return false;

    if (java::BoxContains(bb, frame.eye)) {
        out = RotationsToPoint(frame, Vec3{(bb.minX + bb.maxX) / 2.0, frame.eye.y,
                                           (bb.minZ + bb.maxZ) / 2.0},
                               baseYaw, basePitch);
        return true;
    }

    Vec3 mainPoint;
    if (!GetAimPoint(entity, horizontalMultipoint, verticalMultipoint, frame.eye, mainPoint))
        return false;

    if (frame.eye.DistanceToSqr(mainPoint) < 1e-6)
        return false;

    if (!MainRayHitsTargetAabb(frame.eye, mainPoint, entity, range)) {
        out = RotationsToPoint(frame, mainPoint, baseYaw, basePitch);
        return true;
    }

    if (CanAimAtPoint(entity, frame.eye, mainPoint, range, allowThroughBlocks, allowThroughEntities)) {
        out = RotationsToPoint(frame, mainPoint, baseYaw, basePitch);
        return true;
    }

    std::vector<Vec3> backups = BuildBackupPoints(entity, frame.eye);
    std::stable_sort(backups.begin(), backups.end(), [&frame](const Vec3& a, const Vec3& b) {
        return frame.eye.DistanceToSqr(a) < frame.eye.DistanceToSqr(b);
    });

    for (const Vec3& point : backups) {
        if (CanAimAtPoint(entity, frame.eye, point, range, allowThroughBlocks, allowThroughEntities)) {
            out = RotationsToPoint(frame, point, baseYaw, basePitch);
            return true;
        }
    }

    return false;
}

AimRotations SmoothRotation(float baseYaw, float basePitch, float targetYaw, float targetPitch,
                            int speed, float randomizationPercent) {
    if (speed <= 0)
        return AimRotations{baseYaw, ClampAimPitch(basePitch)};
    if (speed >= 30)
        return AimRotations{targetYaw, ClampAimPitch(targetPitch)};

    const float deltaYaw = mth::WrapDegrees(targetYaw - baseYaw);
    const float deltaPitch = targetPitch - basePitch;
    const float magnitude =
        static_cast<float>(std::sqrt(static_cast<double>(deltaYaw) * deltaYaw +
                                     static_cast<double>(deltaPitch) * deltaPitch));
    if (magnitude < 0.001f)
        return AimRotations{targetYaw, ClampAimPitch(targetPitch)};

    const float t = static_cast<float>(speed) / 30.0f;
    float stepSize = t * t * 180.0f;

    const float range = 0.6f * static_cast<float>(randomizationPercent / 100.0);
    const float multiplier =
        (range <= 0.001f) ? 1.0f
                          : (1.0f - range / 2.0f + static_cast<float>(Random() * range));
    stepSize *= multiplier;

    float proximityFactor = std::min(1.0f, magnitude / kFarThreshold);
    proximityFactor = static_cast<float>(std::pow(proximityFactor, 0.7));

    const float maxSlowdown = static_cast<float>(randomizationPercent / 100.0);
    const float proximityMult =
        std::max(0.8f, 1.0f - maxSlowdown * (1.0f - proximityFactor));
    stepSize *= proximityMult;

    const float stepLength = std::min(stepSize, magnitude);
    const float scale = stepLength / magnitude;

    return AimRotations{baseYaw + deltaYaw * scale,
                        ClampAimPitch(basePitch + deltaPitch * scale)};
}

}
