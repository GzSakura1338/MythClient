#pragma once
#include "minecraft/java/api/game_crystal.h"
namespace nova::java {
enum class AnchorItem { Anchor, Glowstone, Detonator };
enum class AnchorAction { Place, Charge, Detonate, Shield };
struct AnchorState {bool anchor=false,replaceable=false;int charges=0;};
bool ReadAnchor(const BlockPos&,AnchorState&);
bool AnchorExplodes(const BlockPos&,bool&);
bool AnchorBuildable(const BlockPos&);
bool AnchorSlot(int,AnchorItem);
bool AnchorOffhand();
bool MioAnchorExposure(const EntityHandle&,const BlockPos&,const Box&,double&);
bool UseOffhandAnchor(const BlockPos&,const BlockHitResultInfo&,int,const void*,bool&);
bool AnchorExposure(const EntityHandle&,const BlockPos&,const Box&,double&);
bool AnchorExposureWithShield(const EntityHandle&,const BlockPos&,const Box&,const BlockPos*,double&);
int AnchorGlowstoneCount();
bool AnchorTargetYaw(const EntityHandle&,float&);
bool UseAnchor(const BlockPos&,const BlockHitResultInfo&,int,AnchorAction,const void*,bool&);
}
