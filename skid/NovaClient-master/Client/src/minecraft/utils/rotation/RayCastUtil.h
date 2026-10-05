#pragma once

#include "minecraft/java/api/game_entity.h"
#include "minecraft/utils/rotation/Rotation.h"
#include "minecraft/utils/rotation/Vec3.h"

namespace nova::rotation {

enum class HitResultType { Miss, Block, Entity };

enum class BlockFace { Down, Up, North, South, West, East };

struct HitResult {
    HitResultType type = HitResultType::Miss;
    Vec3 location;
    java::EntityHandle entity;
};

// Leader 1.8: tick-end eyes, canBeCollidedWith, no shared-vehicle exclusion.
HitResult RiseRayCast(const Rotation& rotation, double range, float expand = 0.0f);
HitResult LeaderRayCast(const Rotation& rotation, double range);
HitResult RayCast(const Rotation& rotation, double range);
// Game interaction: current tick eyes, ordinary entity/vehicle filters.
HitResult InteractionRayCast(const Rotation& rotation, double range);

HitResult RayCast(const Rotation& rotation, double range, float expand,
                  const java::EntityHandle& entity);

bool InView(const java::EntityHandle& entity);

Rotation CalculateEntity(const java::EntityHandle& entity);

Rotation CalculateEntity(const java::EntityHandle& entity, bool adaptive, double range);

double EntityDistance(const Vec3& eye, const java::EntityHandle& entity);

bool OverBlock(const Rotation& rotation, BlockFace facing, const java::BlockPos& pos, bool strict);

bool OverBlock(BlockFace facing, const java::BlockPos& pos, bool strict);

bool OverBlock(const Rotation& rotation, const java::BlockPos& pos);

bool OverBlock(const Rotation& rotation, const java::BlockPos& pos, BlockFace facing);

}
