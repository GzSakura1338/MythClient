#pragma once
#include "minecraft/java/api/game_entity.h"
#include <algorithm>
#include <cmath>

namespace nova::module::crystal {
// Virtual obsidian only affects ray tests; never mutate the live world.
inline bool CrossesSolid(rotation::Vec3 from,rotation::Vec3 to,const java::Box& box) {
    const auto d=to.Subtract(from);
    const double p[]{from.x,from.y,from.z},v[]{d.x,d.y,d.z};
    const double lo[]{box.minX,box.minY,box.minZ},hi[]{box.maxX,box.maxY,box.maxZ};
    double enter=0,leave=1;
    for(int i=0;i<3;++i) {
        if(std::abs(v[i])<1e-12){if(p[i]<=lo[i]||p[i]>=hi[i])return false;continue;}
        double a=(lo[i]-p[i])/v[i],b=(hi[i]-p[i])/v[i];
        if(a>b)std::swap(a,b);
        enter=std::max(enter,a);leave=std::min(leave,b);
        if(leave-enter<1e-7)return false;
    }
    return enter<1.-1e-7&&leave>1e-7;
}
// Vanilla/Mio bounding-box sample spacing. Clip returns false on lookup failure.
template<class Clip> bool SampleExposure(const java::Box& b,rotation::Vec3 explosion,
                                        const java::Box* solid,Clip clip,double& out) {
    out=0;
    const double width=b.maxX-b.minX,height=b.maxY-b.minY,depth=b.maxZ-b.minZ;
    if(!std::isfinite(width+height+depth)||width<=0||height<=0||depth<=0||width>4||height>4||depth>4)return false;
    const double sx=1/(width*2+1),sy=1/(height*2+1),sz=1/(depth*2+1);
    const double ox=(1-std::floor(1/sx)*sx)/2,oz=(1-std::floor(1/sz)*sz)/2;
    int visible=0,total=0;
    for(double x=0;x<=1;x+=sx)for(double y=0;y<=1;y+=sy)for(double z=0;z<=1;z+=sz) {
        const rotation::Vec3 from{b.minX+width*x+ox,b.minY+height*y,b.minZ+depth*z+oz};
        ++total;
        if(solid&&CrossesSolid(from,explosion,*solid))continue;
        bool clear=false;if(!clip(from,explosion,clear))return false;
        if(clear)++visible;
    }
    out=double(visible)/total;return true;
}
}
