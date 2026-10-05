#pragma once
#include "minecraft/java/api/game_entity.h"
#include <algorithm>
#include <cmath>

namespace nova::module::autocrystal {
// Mio HoleSnapHelper.holeSnapHelper2 ignores blocks below resistance 600.
// Skip a whole ignored voxel without changing client/server block state.
template<class Clip,class Resistance>
bool ResistantRay(rotation::Vec3 from,rotation::Vec3 end,Clip clip,Resistance resistance,bool& clear) {
    clear=false;
    const auto delta=end.Subtract(from);const double length=delta.DistanceTo({});
    if(!std::isfinite(length))return false;
    if(length<1e-8){clear=true;return true;}
    const auto direction=delta.Scale(1./length);
    for(int count=0;count<96;++count) {
        java::BlockHitResultInfo hit;
        if(!clip(from,end,hit))return false;
        if(hit.kind==java::HitKind::Miss){clear=true;return true;}
        if(hit.kind!=java::HitKind::Block||!hit.hasBlockPos)return false;
        double value=0;if(!resistance(hit.blockPos,value)||!std::isfinite(value))return false;
        if(value>=600)return true;
        const double p[]{from.x,from.y,from.z},d[]{direction.x,direction.y,direction.z};
        const int b[]{hit.blockPos.x,hit.blockPos.y,hit.blockPos.z};
        double exit=length+1;
        for(int axis=0;axis<3;++axis)if(std::abs(d[axis])>1e-10)
            exit=std::min(exit,(double(b[axis])+(d[axis]>0?1.:0.)-p[axis])/d[axis]);
        if(!std::isfinite(exit)||exit<0)return false;
        if(exit>=end.DistanceTo(from)-1e-7){clear=true;return true;}
        from=from.Add(direction.x*(exit+1e-6),direction.y*(exit+1e-6),direction.z*(exit+1e-6));
    }
    return false;
}
}
