#pragma once
#include "minecraft/java/api/game_entity.h"
namespace nova::java {
enum class MlgBucket { None, Water, Empty };
struct MlgEnvironment {bool water=false,lava=false,passenger=false,flying=false;};
bool ReadMlgEnvironment(MlgEnvironment& out);
bool ReadMlgHazards(bool& burning,bool& inWeb,BlockPos& webPos);
MlgBucket MlgBucketAt(int slot);
bool MlgWaterAt(const BlockPos& pos,bool& water,bool& source);
bool MlgAllowsWater(const BlockPos& pos);
bool UseMlgBucket(MlgBucket expected, float yaw, float pitch);
}
