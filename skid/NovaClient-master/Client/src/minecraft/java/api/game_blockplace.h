#pragma once
#include "minecraft/java/api/game_entity.h"
namespace nova::java {
// Full-cube placement checks, without crystal headroom or explosion rules.
bool ObsidianBuildable(const BlockPos&);
bool ObsidianContextReady();
bool ReadObsidian(const BlockPos&,bool&);
bool PlaceObsidian(const BlockPos&,const BlockHitResultInfo&,int,const void*,bool&);
bool ReadGlowstone(const BlockPos&,bool&);
bool GlowstoneSupport(const BlockPos&);
bool PlaceGlowstone(const BlockPos&,const BlockHitResultInfo&,int,const void*,bool&);
}
