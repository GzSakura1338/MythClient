#pragma once
#include "minecraft/modules/player/automlg/Prediction.h"
#include <vector>

namespace nova::module::automlg {
inline rotation::Vec3 WaterAimPoint(const java::BlockPos& pos, const rotation::Vec3& eye) {
    // Pick an interior point near the eye, including sources at the edge of reach.
    return {std::clamp(eye.x,pos.x+.15,pos.x+.85),
            std::clamp(eye.y,pos.y+.15,pos.y+.75),
            std::clamp(eye.z,pos.z+.15,pos.z+.85)};
}
inline double DistanceSquared(rotation::Vec3 a, rotation::Vec3 b) {
    const auto d=a.Subtract(b);return d.x*d.x+d.y*d.y+d.z*d.z;
}
// Bounded work on the game thread; nearby cells are examined before distant ones.
class WaterSearch {
public:
    void Reset(){cells_.clear();index_=0;}
    template<class IsSource,class Visible>
    bool Step(rotation::Vec3 eye,IsSource&& isSource,Visible&& visible,java::BlockPos& result) {
        if(!std::isfinite(eye.x+eye.y+eye.z))return false;
        if(cells_.empty() || DistanceSquared(eye,origin_)>.75*.75)Begin(eye);
        const auto end=std::min(index_+64,cells_.size());
        for(;index_<end;) {
            const auto pos=cells_[index_++].pos;
            const auto point=WaterAimPoint(pos,eye);
            if(DistanceSquared(point,eye)>4.5*4.5 || !isSource(pos) || !visible(pos,point))continue;
            result=pos;Reset();return true;
        }
        if(index_==cells_.size())Reset();
        return false;
    }
private:
    struct Cell{java::BlockPos pos;double distance;};
    void Begin(rotation::Vec3 eye) {
        Reset();origin_=eye;
        const int x=static_cast<int>(std::floor(eye.x)),y=static_cast<int>(std::floor(eye.y)),z=static_cast<int>(std::floor(eye.z));
        for(int dx=-5;dx<=5;++dx)for(int dy=-5;dy<=5;++dy)for(int dz=-5;dz<=5;++dz) {
            java::BlockPos pos{x+dx,y+dy,z+dz};const double d=DistanceSquared(WaterAimPoint(pos,eye),eye);
            if(d<=4.5*4.5)cells_.push_back({pos,d});
        }
        std::stable_sort(cells_.begin(),cells_.end(),[](const auto& a,const auto& b){return a.distance<b.distance;});
    }
    rotation::Vec3 origin_{};
    std::vector<Cell> cells_;
    size_t index_=0;
};
}
