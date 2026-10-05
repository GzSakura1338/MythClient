#pragma once
#include "minecraft/java/api/game_entity.h"
#include <algorithm>
#include <cmath>
namespace nova::module::automlg {
struct Landing {java::BlockHitResultInfo hit;int ticks=0;};
inline bool SameBlock(const java::BlockPos& a,const java::BlockPos& b){return a.x==b.x && a.y==b.y && a.z==b.z;}
// Bounded swept foot samples: finds the first blocking surface, including slab
// heights, instead of treating every block cell as a full cube.
template<class Clip> bool Predict(java::Box box,rotation::Vec3 velocity,Clip&& clip,Landing& result) {
    if(!std::isfinite(velocity.x+velocity.y+velocity.z+box.minX+box.minY+box.minZ+box.maxX+box.maxY+box.maxZ) ||
       box.maxX<=box.minX || box.maxZ<=box.minZ || box.maxX-box.minX>2 || box.maxZ-box.minZ>2 || velocity.y>=0)return false;
    const double xs[]{box.minX+.02,(box.minX+box.maxX)*.5,box.maxX-.02};
    const double zs[]{box.minZ+.02,(box.minZ+box.maxZ)*.5,box.maxZ-.02};
    rotation::Vec3 offset{};
    for(int tick=1;tick<=10;++tick) {
        double best=2;java::BlockHitResultInfo first;
        for(double x:xs)for(double z:zs) {
            const rotation::Vec3 from{x+offset.x,box.minY+offset.y+.01,z+offset.z};
            const auto to=from.Add(velocity.x,velocity.y,velocity.z);
            java::BlockHitResultInfo hit;
            if(!clip(from,to,hit))return false;
            if(hit.kind!=java::HitKind::Block || !hit.hasBlockPos)continue;
            const double length=velocity.x*velocity.x+velocity.y*velocity.y+velocity.z*velocity.z;
            const double fraction=((hit.location.x-from.x)*velocity.x+(hit.location.y-from.y)*velocity.y+(hit.location.z-from.z)*velocity.z)/length;
            if(fraction>=0 && fraction<best){best=fraction;first=hit;}
        }
        if(best<=1) {
            if(first.direction!=1)return false; // A wall is not a landing surface.
            result={first,tick};return true;
        }
        offset=offset.Add(velocity.x,velocity.y,velocity.z);
        velocity.x*=.91;velocity.z*=.91;velocity.y=(velocity.y-.08)*.98;
    }
    return false;
}
inline bool SafeToRecover(bool enteredWater,bool inWater,bool ground,float fallDistance,double verticalVelocity) {
    return enteredWater && (inWater||ground) && fallDistance<.1f && verticalVelocity>-.12;
}
inline bool SwitchReady(int ticks,double distance,double verticalVelocity) {
    return ticks>0 && ticks<=3 && std::isfinite(distance) && distance<=4.5+std::min(2.,std::abs(verticalVelocity));
}
}
