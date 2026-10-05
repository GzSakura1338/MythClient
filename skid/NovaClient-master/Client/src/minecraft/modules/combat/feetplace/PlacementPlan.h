#pragma once
#include "minecraft/java/api/game_entity.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace nova::module::feetplace {
using Pos=java::BlockPos;
inline bool Same(Pos a,Pos b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
inline bool Contains(const std::vector<Pos>& list,Pos p){return std::any_of(list.begin(),list.end(),[&](Pos b){return Same(p,b);});}
inline bool Add(std::vector<Pos>& list,Pos p){if(list.size()>=32||Contains(list,p))return false;list.push_back(p);return true;}
inline constexpr Pos Sides[]={{-1,0,0},{1,0,0},{0,0,1},{0,0,-1}};
inline constexpr Pos Supports[]={{0,-1,0},{0,0,-1},{1,0,0},{0,0,1},{-1,0,0},{0,1,0}};
inline Pos Offset(Pos a,Pos b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline std::vector<Pos> Footprint(const java::Box& box){
    if(!std::isfinite(box.minX)||!std::isfinite(box.maxX)||!std::isfinite(box.minY)||
       !std::isfinite(box.minZ)||!std::isfinite(box.maxZ)||box.maxX-box.minX>4||box.maxZ-box.minZ>4)return {};
    std::vector<Pos> out;const int y=int(std::floor(box.minY+.5));
    for(int x=int(std::floor(box.minX+1e-5));x<=int(std::floor(box.maxX-1e-5));++x)
    for(int z=int(std::floor(box.minZ+1e-5));z<=int(std::floor(box.maxZ-1e-5));++z)Add(out,{x,y,z});
    return out;
}
inline std::vector<Pos> Ring(const std::vector<Pos>& footprint){
    std::vector<Pos> out;
    for(auto p:footprint)for(auto d:Sides){auto next=Offset(p,d);if(!Contains(footprint,next))Add(out,next);}
    for(auto p:footprint)Add(out,Offset(p,{0,-1,0}));
    return out;
}
inline bool SameFootprint(const std::vector<Pos>& a,const std::vector<Pos>& b){
    return a.size()==b.size()&&std::all_of(a.begin(),a.end(),[&](Pos p){return Contains(b,p);});
}
// Search a connected chain, returning only its first currently actionable cell.
// Never treat a hypothetical support as a block already present in the world.
template<class Buildable,class Aim> bool FirstStep(Pos goal,int depth,std::vector<Pos>& visited,
                                                  Buildable buildable,Aim aim,Pos& out){
    if(!Add(visited,goal)||!buildable(goal))return false;
    if(aim(goal)){out=goal;return true;}
    if(depth<=0)return false;
    for(auto d:Supports)if(FirstStep(Offset(goal,d),depth-1,visited,buildable,aim,out))return true;
    return false;
}
}
