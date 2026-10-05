#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <compare>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <tuple>
#include <vector>
#include "minecraft/utils/rotation/Rotation.h"
#include "minecraft/utils/rotation/Mth.h"
#include "minecraft/utils/rotation/Vec3.h"

namespace nova::module::blockin {
using rotation::Rotation;
using rotation::Vec3;
struct Pos {
    int x=0,y=0,z=0;
    auto operator<=>(const Pos&) const = default;
};
inline constexpr std::array<Pos,6> steps{{{0,-1,0},{0,1,0},{0,0,-1},{0,0,1},{-1,0,0},{1,0,0}}};
inline Pos Offset(Pos p,int side) { auto d=steps[side]; return {p.x+d.x,p.y+d.y,p.z+d.z}; }
inline Pos Floor(Vec3 p) { return {int(std::floor(p.x)),int(std::floor(p.y)),int(std::floor(p.z))}; }
struct Cell { bool replaceable=false,air=false,roofExcluded=false; };
struct Hit { Pos block; int side=-1; Vec3 point; };
struct Aim { Hit hit; Rotation rotation; };
struct Stack { bool valid=false; float strength=0; };
struct Selection { Aim aim; int slot=-1; bool adjacent=false; };
inline bool Adjacent(Pos p,Pos feet) {
    int dx=p.x-feet.x,dy=p.y-feet.y,dz=p.z-feet.z;
    return (dx==0 && dz==0 && dy==2) ||
        ((dy==0 || dy==1) && ((std::abs(dx)==1 && dz==0)||(std::abs(dz)==1 && dx==0)));
}
inline Rotation Toward(Vec3 eye,Vec3 target) {
    const double dx=target.x-eye.x,dy=target.y-eye.y,dz=target.z-eye.z;
    constexpr double deg=57.29577951308232;
    return {float(std::atan2(dz,dx)*deg)-90.f,float(-std::atan2(dy,std::sqrt(dx*dx+dz*dz))*deg)};
}
template<class Random> Rotation Smooth(Rotation base,Rotation target,int speed,float randomization,Random&& random) {
    target.pitch=std::clamp(target.pitch,-90.f,90.f);
    if(speed<=0) return {base.yaw,std::clamp(base.pitch,-90.f,90.f)};
    if(speed>=30) return target;
    float dy=rotation::mth::WrapDegrees(target.yaw-base.yaw),dp=target.pitch-base.pitch;
    float magnitude=float(std::sqrt(double(dy*dy+dp*dp)));
    if(magnitude<.001f) return target;
    float t=speed/30.f,step=t*t*180.f,range=.6f*float(randomization/100.0);
    step*=range<=.001f?1.f:1.f-range/2.f+float(random()*range);
    float proximity=float(std::pow(double(std::min(1.f,magnitude/180.f)),.7));
    step*=std::max(.8f,1.f-float(randomization/100.0)*(1.f-proximity));
    float scale=std::min(step,magnitude)/magnitude;
    return {base.yaw+dy*scale,std::clamp(base.pitch+dp*scale,-90.f,90.f)};
}
inline Rotation Fix(Rotation target,Rotation previous,double sensitivity) {
    float yaw=previous.yaw+(std::fmod(std::fmod(target.yaw-previous.yaw+180.f,360.f)+360.f,360.f)-180.f);
    float factor=float(sensitivity)*.6f+.2f;
    double gcd=double(factor*factor*factor)*1.2;
    if(!std::isfinite(gcd)||gcd<=0) return previous;
    return {previous.yaw+float(std::floor(double(yaw-previous.yaw)/gcd+.5)*gcd),
        std::clamp(previous.pitch+float(std::floor(double(target.pitch-previous.pitch)/gcd+.5)*gcd),-90.f,90.f)};
}
inline bool QueuePlacement(const Aim& target,Rotation smooth,Rotation previous,double tolerance,const std::optional<Hit>& hit) {
    return hit && hit->block==target.hit.block && hit->side==target.hit.side &&
        std::abs(smooth.yaw-previous.yaw)<=tolerance && std::abs(smooth.pitch-previous.pitch)<=tolerance;
}

// Raven BlockIn's planner. IO owns a per-tick snapshot/cache and native JNI locals.
template<class IO> class Planner {
    IO& io;
    Vec3 eye;
    Pos feet;
    Rotation previous;
    int slot=-1;
    std::map<Pos,Cell> cells;
    std::map<std::tuple<int,int,int,int>,bool> placeable;
    static constexpr double reach=4.5,inset=.05,step=.2;
    Cell Read(Pos p) {
        auto [it,newCell]=cells.try_emplace(p);
        if(newCell) it->second=io.Read(p);
        return it->second;
    }
    bool CanPlace(Pos p,int face) {
        auto [it,fresh]=placeable.try_emplace(std::tuple{p.x,p.y,p.z,face});
        if(fresh) it->second=io.CanPlace(slot,p,face);
        return it->second;
    }
    double Jitter() { return (io.Random()*2-1)*(step*.1); }
    static double Clamp(double v) { return std::clamp(v,0.,1.); }
    double Cost(Rotation r) { return std::abs(rotation::mth::WrapDegrees(r.yaw-previous.yaw))+std::abs(r.pitch-previous.pitch); }
    std::optional<Aim> Try(Rotation r,Pos support,int side,Pos goal) {
        auto hit=io.Trace(r);
        if(!hit || hit->block!=support || hit->side!=side || Offset(hit->block,hit->side)!=goal) return {};
        return Aim{*hit,r};
    }
    bool AirNeighbor(Pos p) {
        for(int f=0;f<6;++f) { auto n=Offset(p,f); if(n!=feet && n!=Offset(feet,1) && Read(n).air) return true; }
        return false;
    }
    std::optional<Aim> RoofBlock(Pos p,int minY) {
        bool up=std::abs(eye.y-(p.y+1))<std::abs(eye.y-p.y);
        bool south=std::abs(eye.z-(p.z+1))<std::abs(eye.z-p.z);
        bool east=std::abs(eye.x-(p.x+1))<std::abs(eye.x-p.x);
        struct Candidate { double cost; Rotation r; };
        std::vector<Candidate> candidates{{0,previous}};
        for(int row=0;row<=5;++row) {
            double v=Clamp(row*step+Jitter());
            for(int col=0;col<=5;++col) {
                double u=Clamp(col*step+Jitter());
                for(Vec3 point : {Vec3{p.x+u,up?p.y+1-inset:p.y+inset,p.z+v},
                    Vec3{p.x+u,p.y+v,south?p.z+1-inset:p.z+inset},
                    Vec3{east?p.x+1-inset:p.x+inset,p.y+v,p.z+u}}) {
                    auto r=Toward(eye,point); candidates.push_back({Cost(r),r});
                }
            }
        }
        std::stable_sort(candidates.begin(),candidates.end(),[](auto& a,auto& b){return a.cost<b.cost;});
        for(auto& c:candidates) {
            auto hit=io.Trace(c.r);
            if(hit && hit->block==p && hit->block.y>=minY && !(hit->side==0 && p.y==minY) && CanPlace(p,hit->side))
                return Aim{*hit,c.r};
        }
        return {};
    }
    std::optional<Aim> Roof() {
        if(!Read({feet.x,feet.y+2,feet.z}).replaceable) return {};
        struct Candidate { double distance; Pos p; };
        std::vector<Candidate> candidates;
        int minY=int(std::floor(eye.y))+1;
        for(int y=minY;y<=int(std::floor(eye.y+reach));++y)
            for(int x=int(std::floor(eye.x-reach));x<=int(std::floor(eye.x+reach));++x)
                for(int z=int(std::floor(eye.z-reach));z<=int(std::floor(eye.z+reach));++z) {
                    Vec3 center{x+.5,y+.5,z+.5};
                    if(eye.DistanceToSqr(center)>(reach+1)*(reach+1)) continue;
                    Pos p{x,y,z}; auto c=Read(p);
                    if(c.replaceable || c.roofExcluded) continue;
                    double distance=eye.DistanceToSqr({std::clamp(eye.x,double(x),double(x+1)),
                        std::clamp(eye.y,double(y),double(y+1)),std::clamp(eye.z,double(z),double(z+1))});
                    if(distance<=reach*reach) candidates.push_back({distance,p});
                }
        std::stable_sort(candidates.begin(),candidates.end(),[](auto& a,auto& b){return a.distance<b.distance;});
        for(auto& c:candidates) if(auto aim=RoofBlock(c.p,minY)) return aim;
        return {};
    }
    std::optional<Aim> Goals(const std::vector<Pos>& goals) {
        if(goals.empty()) return {};
        if(auto hit=io.Trace(previous);hit && !Read(hit->block).replaceable && CanPlace(hit->block,hit->side))
            for(auto g:goals) if(auto aim=Try(previous,hit->block,hit->side,g)) return aim;
        // Keep Raven's exact support order and horizontal face assignments.
        struct Support { Pos d; int face; };
        constexpr Support supports[]{{{0,1,0},0},{{0,-1,0},1},{{0,0,-1},2},{{0,0,1},3},{{1,0,0},5},{{-1,0,0},4}};
        struct Candidate { double cost; Rotation r; Pos support,goal; int side; };
        std::vector<Candidate> candidates;
        for(auto g:goals) for(auto s:supports) {
            Pos p{g.x+s.d.x,g.y+s.d.y,g.z+s.d.z};
            if(Read(p).replaceable || !CanPlace(p,s.face)) continue;
            for(int row=0;row<=5;++row) {
                double v=Clamp(row*step+Jitter());
                for(int col=0;col<=5;++col) {
                    double cu=Clamp(col*step+Jitter()),u=(row&1)?1-cu:cu;
                    Vec3 point;
                    if(s.d.y) point={p.x+u,p.y+(s.d.y<0?1-inset-.001:inset+.001),p.z+v};
                    else if(s.d.z) point={p.x+u,p.y+v,p.z+(s.d.z<0?1-inset-.001:inset+.001)};
                    else point={p.x+(s.d.x<0?1-inset-.001:inset+.001),p.y+v,p.z+u};
                    auto r=Toward(eye,point); double cost=Cost(r);
                    if(std::abs(rotation::mth::WrapDegrees(r.yaw-previous.yaw))<.1f && std::abs(r.pitch-previous.pitch)<.1f) continue;
                    candidates.push_back({cost,r,p,g,s.face});
                }
            }
        }
        std::stable_sort(candidates.begin(),candidates.end(),[](auto& a,auto& b){return a.cost<b.cost;});
        for(auto& c:candidates) if(auto aim=Try(c.r,c.support,c.side,c.goal)) return aim;
        return {};
    }
    std::optional<Aim> Sides() {
        std::vector<Pos> baseline,goals;
        for(int side:{5,3,4,2}) { baseline.push_back(Offset(feet,side)); baseline.push_back(Offset(Offset(feet,1),side)); }
        for(auto p:baseline) if(Read(p).replaceable && AirNeighbor(p)) goals.push_back(p);
        if(goals.empty()) return {};
        if(auto enemy=io.ClosestPlayer()) {
            auto distance=[&](Pos p){return enemy->DistanceToSqr({p.x+.5,p.y+.5,p.z+.5});};
            std::stable_sort(baseline.begin(),baseline.end(),[&](auto a,auto b){return distance(a)<distance(b);});
            int picked=0;
            for(auto p:baseline) {
                if(picked>=3) break;
                if(!Read(p).replaceable || !AirNeighbor(p)) continue;
                if(auto aim=Goals({p})) return aim;
                ++picked;
            }
        }
        if(auto aim=Goals(goals)) return aim;
        std::vector<Pos> frontier=goals;
        std::set<Pos> seen(frontier.begin(),frontier.end());
        for(int iter=0;iter<5 && !frontier.empty();++iter) {
            std::vector<Pos> layer;
            for(auto g:frontier) for(int side=0;side<6;++side) {
                auto p=Offset(g,side);
                if(Read(p).replaceable && seen.insert(p).second) layer.push_back(p);
            }
            if(auto aim=Goals(layer)) return aim;
            frontier=std::move(layer);
        }
        return {};
    }
public:
    Planner(IO& io,Vec3 position,Vec3 eye,Rotation previous):io(io),eye(eye),feet(Floor(position)),previous(previous) {}
    std::optional<Selection> Select() {
        int strong=-1,weak=-1; float hi=-1,lo=std::numeric_limits<float>::max();
        for(int i=8;i>=0;--i) {
            auto s=io.Item(i); if(!s.valid) continue;
            if(s.strength>hi) {hi=s.strength;strong=i;}
            if(s.strength<lo) {lo=s.strength;weak=i;}
        }
        if(strong<0 && weak<0) return {};
        slot=strong>=0?strong:weak;
        auto aim=Roof(); if(!aim) aim=Sides();
        if(!aim) return {};
        bool adjacent=Adjacent(Offset(aim->hit.block,aim->hit.side),feet);
        return Selection{*aim,adjacent?(strong>=0?strong:weak):(weak>=0?weak:strong),adjacent};
    }
    int Filled() {
        int count=!Read({feet.x,feet.y+2,feet.z}).replaceable;
        for(int side:{5,3,4,2}) {
            count+=!Read(Offset(feet,side)).replaceable;
            count+=!Read(Offset(Offset(feet,1),side)).replaceable;
        }
        return count;
    }
};
}
