#pragma once
#include "minecraft/modules/player/automlg/WaterSearch.h"
namespace nova::module::automlg {
inline java::BlockPos Adjacent(const java::BlockHitResultInfo& hit) {
    auto p=hit.blockPos;
    switch(hit.direction){case 0:--p.y;break;case 1:++p.y;break;case 2:--p.z;break;
        case 3:++p.z;break;case 4:--p.x;break;case 5:++p.x;break;}
    return p;
}
inline bool CanFlowToWeb(const java::BlockPos& source,const java::BlockPos& web) {
    const int horizontal=std::abs(source.x-web.x)+std::abs(source.z-web.z);
    return (source.y==web.y || source.y==web.y+1) && horizontal<=1;
}
// Validate the actual ray result, including rays starting inside the web.
// Water below a head-level web cannot flow up to it, so it is never a fallback.
template<class Clip,class CanPlace>
bool FindWebPlacement(rotation::Vec3 eye,java::BlockPos web,Clip&& clip,CanPlace&& canPlace,Landing& out) {
    double best=1e9;bool found=false;
    const rotation::Vec3 faces[]{{.5,1.01,.5},{1.01,.5,.5},{-.01,.5,.5},
                                {.5,.5,1.01},{.5,.5,-.01},{.5,-.01,.5}};
    for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y)for(int z=-1;z<=1;++z)
    for(const auto& face:faces) {
        const rotation::Vec3 point{web.x+x+face.x,web.y+y+face.y,web.z+z+face.z};
        java::BlockHitResultInfo hit;
        if(!clip(eye,point,hit)||hit.kind!=java::HitKind::Block||!hit.hasBlockPos||hit.direction<0||hit.direction>5)continue;
        const auto source=Adjacent(hit);
        const double distance=DistanceSquared(eye,hit.location);
        if(distance>4.5*4.5||!CanFlowToWeb(source,web)||!canPlace(source))continue;
        // Prefer directly above the web, then the closest reachable side.
        const double score=distance+(source.x==web.x&&source.z==web.z?0:25);
        if(score>=best)continue;
        best=score;out.hit=hit;out.ticks=0;found=true;
    }
    return found;
}
}
