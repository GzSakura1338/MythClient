#pragma once
#include "minecraft/java/api/game_entity.h"

namespace nova::java {
struct CrystalDefense {
    double armor=0, toughness=0, health=0, absorption=0;
    int resistance=-1, protection=0, difficulty=2;
};
bool CrystalSupported();
bool CrystalContextReady();
bool IsEndCrystal(const EntityHandle& entity);
bool CrystalHotbarSlot(int slot);
bool CrystalOffhand();
bool CrystalCreative(const EntityHandle& entity,bool& out);
bool PlaceOffhandEndCrystal(const BlockHitResultInfo& hit,const void* owner,bool& entered);
bool SwingCrystalHand(bool offhand,bool animate);
bool CrystalBlockReach(double& out);
bool CrystalBase(const BlockPos& pos);
// Checks the future crystal space without requiring an already-built base.
bool CrystalSpaceClear(const BlockPos& pos);
bool CrystalPlaceable(const BlockPos& pos);
bool CrystalObsidianSlot(int slot);
// Base placement occupies one block; crystal placement occupies the two above it.
inline Box CrystalPlacementVolume(const BlockPos& pos,bool building) {
    return {double(pos.x),double(pos.y+(building?0:1)),double(pos.z),
        double(pos.x+1),double(pos.y+(building?1:3)),double(pos.z+1)};
}
// Empty base block with air above; includes the local player in collision checks.
bool CrystalBaseBuildable(const BlockPos& pos);
bool CrystalBaseSupport(const BlockPos& pos);
bool CrystalMotionClear(const EntityHandle& entity,const Box& box);
bool ReadCrystalDefense(const EntityHandle& entity, CrystalDefense& out);
bool CrystalExposure(const EntityHandle& entity,const rotation::Vec3& explosion,double& out);
bool CrystalExposureAt(const EntityHandle& entity,const rotation::Vec3& explosion,
                       const Box& bounds,const BlockPos* virtualBase,double& out);
// AutoCrystal's Mio exposure policy; the legacy aura retains vanilla clipping.
bool MioCrystalExposureAt(const EntityHandle& entity,const rotation::Vec3& explosion,
                         const Box& bounds,const BlockPos* virtualBase,double& out);
// Enters the shared item-use transaction only after all native inputs resolve.
// entered distinguishes an uncertain native result from a pre-submit rejection.
bool PlaceEndCrystal(const BlockHitResultInfo& hit,int expectedSlot,const void* owner,bool& entered);
bool PlaceCrystalBase(const BlockPos& base,const BlockHitResultInfo& hit,int expectedSlot,const void* owner,bool& entered);
}
