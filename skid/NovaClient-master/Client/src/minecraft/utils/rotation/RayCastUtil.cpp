#include "minecraft/utils/rotation/RayCastUtil.h"

#include <cmath>
#include <utility>
#include <vector>

#include "minecraft/java/api/game_rotation.h"
#include "minecraft/utils/rotation/Mth.h"
#include "minecraft/utils/rotation/Random.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/RotationUtils.h"

namespace nova::rotation {
namespace {

struct Candidate {
    java::EntityHandle entity;
    java::Box box;
};

constexpr double kBlockReach = 4.5;

}

bool OverBlock(const Rotation& rotation, BlockFace facing, const java::BlockPos& pos, bool strict) {
    java::EntityHandle player = java::PlayerEntity();
    if (!player.Valid())
        return false;

    java::EntityHandle level = java::LevelEntity();
    if (!level.Valid())
        return false;

    Vec3 lookVec;
    if (!java::Vec3DirectionFromRotation(rotation.pitch, rotation.yaw, lookVec))
        return false;

    Vec3 eyePos;
    if (!player.EyePosition(1.0f, eyePos))
        return false;

    const Vec3 scaledLook = lookVec.Scale(kBlockReach);
    const Vec3 endVec = eyePos.Add(scaledLook.x, scaledLook.y, scaledLook.z);

    java::BlockHitResultInfo hit;
    if (!java::WorldClip(eyePos, endVec, player, hit))
        return false;

    if (hit.kind != java::HitKind::Block || !hit.hasBlockPos)
        return false;

    if (hit.blockPos.x != pos.x || hit.blockPos.y != pos.y || hit.blockPos.z != pos.z)
        return false;

    if (!strict)
        return true;

    return hit.direction == static_cast<int>(facing);
}

bool OverBlock(BlockFace, const java::BlockPos& pos, bool) {
    return OverBlock(GetRotation(), BlockFace::Up, pos, false);
}

bool OverBlock(const Rotation& rotation, const java::BlockPos& pos) {
    return OverBlock(rotation, BlockFace::Up, pos, false);
}

bool OverBlock(const Rotation& rotation, const java::BlockPos& pos, BlockFace facing) {
    return OverBlock(rotation, facing, pos, true);
}

Rotation CalculateEntity(const java::EntityHandle& entity) {
    if (!entity.Valid())
        return Rotation{};

    java::EntityHandle player = java::PlayerEntity();
    if (!player.Valid())
        return Rotation{};

    Vec3 eye;
    Vec3 basePos;
    double playerY = 0.0;
    double playerEyeHeight = 0.0;
    java::Box box;

    if (!player.EyePosition(1.0f, eye) || !entity.Position(basePos) ||
        !player.GetY(playerY) || !player.EyeHeight(playerEyeHeight) || !entity.BoundingBox(box))
        return Rotation{};

    return CalculateEntityAim(eye, basePos, box.maxY - box.minY, playerY, playerEyeHeight);
}

Rotation CalculateEntity(const java::EntityHandle& entity, bool adaptive, double range) {
    const Rotation normal = CalculateEntity(entity);
    if (!entity.Valid())
        return normal;

    if (!adaptive || RayCast(normal, range).type == HitResultType::Entity)
        return normal;

    java::EntityHandle player = java::PlayerEntity();
    if (!player.Valid())
        return normal;

    Vec3 eye;
    Vec3 basePos;
    java::Box box;
    if (!player.EyePosition(1.0f, eye) || !entity.Position(basePos) || !entity.BoundingBox(box))
        return normal;

    const double width = box.maxX - box.minX;
    const double height = box.maxY - box.minY;
    const double depth = box.maxZ - box.minZ;

    for (double yPercent = 1.0; yPercent >= 0.0; yPercent -= 0.25 + Random() * 0.1) {
        for (double xPercent = 1.0; xPercent >= -0.5; xPercent -= 0.5) {
            for (double zPercent = 1.0; zPercent >= -0.5; zPercent -= 0.5) {
                const Vec3 targetPoint =
                    basePos.Add(width * xPercent, height * yPercent, depth * zPercent);
                const Rotation adaptiveRotations = Calculate(eye, targetPoint);
                if (RayCast(adaptiveRotations, range).type == HitResultType::Entity)
                    return adaptiveRotations;
            }
        }
    }

    return normal;
}

double EntityDistance(const Vec3& eye, const java::EntityHandle& entity) {
    java::Box box;
    if (!entity.Valid() || !entity.BoundingBox(box))
        return 1.7976931348623157E308;

    return GetDistance(eye, Vec3{box.minX, box.minY, box.minZ},
                       Vec3{box.maxX, box.maxY, box.maxZ});
}

static HitResult RayCastWithRules(const Rotation&, double, float, const java::EntityHandle&, bool, bool, bool = false);

HitResult InteractionRayCast(const Rotation& rotation, double range) {
    return RayCastWithRules(rotation, range, 0, java::PlayerEntity(), false, false, true);
}

HitResult RiseRayCast(const Rotation& rotation, double range, float expand) {
    return RayCastWithRules(rotation, range, expand, java::PlayerEntity(), false, true);
}

HitResult LeaderRayCast(const Rotation& rotation, double range) {
    return RayCastWithRules(rotation, range, 0.0f, java::PlayerEntity(), true, false);
}

HitResult RayCast(const Rotation& rotation, double range) {
    return RayCast(rotation, range, 0.0f, java::PlayerEntity());
}

HitResult RayCast(const Rotation& rotation, double range, float expand,
                  const java::EntityHandle& entity) {
    return RayCastWithRules(rotation, range, expand, entity, false, false);
}

static HitResult RayCastWithRules(const Rotation& rotation, double range, float expand,
                                 const java::EntityHandle& entity, bool leader, bool rise, bool tickEyes) {
    HitResult result;
    result.location = Vec3{0.0, 0.0, 0.0};

    if (!entity.Valid())
        return result;

    java::EntityHandle level = java::LevelEntity();
    if (!level.Valid())
        return result;

    const float partialTicks = (leader || tickEyes) ? 1.0f : java::FrameTime();

    Vec3 eyePos;
    if (!entity.EyePosition(partialTicks, eyePos))
        return result;

    Vec3 lookVec;
    if (!java::Vec3DirectionFromRotation(rotation.pitch, rotation.yaw, lookVec))
        return result;

    const Vec3 scaledLook = lookVec.Scale(range);
    const Vec3 endVec = eyePos.Add(scaledLook.x, scaledLook.y, scaledLook.z);

    java::BlockHitResultInfo blockHit;
    if (!java::WorldClip(eyePos, endVec, entity, blockHit))
        return result;

    double distToBlock = range;
    if (blockHit.kind != java::HitKind::Miss)
        distToBlock = blockHit.location.DistanceTo(eyePos);

    const Vec3 entitySearchEndVec = endVec;

    java::Box selfBox;
    if (!entity.BoundingBox(selfBox))
        return result;

    const java::Box searchBox = java::BoxInflate(java::BoxExpandTowards(selfBox, scaledLook), 1.0);

    std::vector<java::EntityHandle> list;
    if (!java::WorldEntities(entity, searchBox, list))
        return result;

    java::EntityHandle pointedEntity;
    Vec3 hitVec;
    double currentDist = distToBlock;

    for (const java::EntityHandle& candidate : list) {
        if ((!leader && candidate.IsSpectator()) || !candidate.IsPickable())
            continue;

        double pickRadius = 0.0;
        if (!candidate.PickRadius(pickRadius))
            continue;

        java::Box candidateBox;
        if (!candidate.BoundingBox(candidateBox))
            continue;

        const double collisionSize = rise ? static_cast<float>(pickRadius) + expand
                                          : pickRadius + static_cast<double>(expand);
        const java::Box entityBox = java::BoxInflate(candidateBox, collisionSize);

        bool interceptHit = false;
        Vec3 interceptVec;
        if (!java::BoxClip(entityBox, eyePos, entitySearchEndVec, interceptHit, interceptVec))
            continue;

        if (java::BoxContains(entityBox, eyePos)) {
            if (currentDist >= 0.0) {
                pointedEntity = java::AdoptEntity(candidate.raw());
                hitVec = interceptHit ? interceptVec : eyePos;
                currentDist = 0.0;
            }
        } else if (interceptHit) {
            const double d3 = eyePos.DistanceTo(interceptVec);

            if (d3 < currentDist || currentDist == 0.0) {
                if (!leader && !rise && candidate.IsSameRootVehicle(entity)) {
                    if (currentDist == 0.0) {
                        pointedEntity = java::AdoptEntity(candidate.raw());
                        hitVec = interceptVec;
                    }
                } else {
                    pointedEntity = java::AdoptEntity(candidate.raw());
                    hitVec = interceptVec;
                    currentDist = d3;
                }
            }
        }
    }

    if (pointedEntity.Valid() &&
        (currentDist < distToBlock || blockHit.kind == java::HitKind::Miss)) {
        result.type = HitResultType::Entity;
        result.location = hitVec;
        result.entity = std::move(pointedEntity);
        return result;
    }

    result.type = blockHit.kind == java::HitKind::Block ? HitResultType::Block : HitResultType::Miss;
    result.location = blockHit.location;
    return result;
}

bool InView(const java::EntityHandle& entity) {
    if (!entity.Valid())
        return false;

    java::EntityHandle player = java::PlayerEntity();
    if (!player.Valid())
        return false;

    int renderDistance = 0;
    int fov = 0;
    if (!java::RenderDistance(renderDistance) || !java::Fov(fov))
        return false;

    const double renderRange = static_cast<double>(renderDistance) * 16.0;

    const Rotation rotations = CalculateEntity(entity);

    float playerYaw = 0.0f;
    float playerPitch = 0.0f;
    if (!java::PlayerRotation(playerYaw, playerPitch))
        return false;

    if (std::fabs(mth::WrapDegrees(playerYaw - rotations.yaw)) > static_cast<float>(fov))
        return false;

    double dist = 0.0;
    if (!player.DistanceTo(entity, dist))
        return false;

    if (dist > 100.0 || !entity.IsPlayer()) {
        const HitResult result = RayCast(rotations, renderRange, 0.2f, player);
        return result.type == HitResultType::Entity;
    }

    java::Box box;
    Vec3 basePos;
    if (!entity.BoundingBox(box) || !entity.Position(basePos))
        return false;

    const double width = box.maxX - box.minX;
    const double height = box.maxY - box.minY;
    const double depth = box.maxZ - box.minZ;

    Vec3 eye;
    if (!player.EyePosition(1.0f, eye))
        return false;

    for (double yPercent = 1.0; yPercent >= -1.0; yPercent -= 0.5) {
        for (double xPercent = 1.0; xPercent >= -1.0; xPercent -= 1.0) {
            for (double zPercent = 1.0; zPercent >= -1.0; zPercent -= 1.0) {
                const Vec3 targetPoint =
                    basePos.Add(width * xPercent, height * yPercent, depth * zPercent);

                const Rotation subRotations = Calculate(eye, targetPoint);

                const HitResult result = RayCast(subRotations, renderRange, 0.2f, player);
                if (result.type == HitResultType::Entity)
                    return true;
            }
        }
    }

    return false;
}

}
