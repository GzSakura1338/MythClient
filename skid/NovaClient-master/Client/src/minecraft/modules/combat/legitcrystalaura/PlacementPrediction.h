#pragma once
#include "minecraft/java/api/game_entity.h"
#include "minecraft/utils/rotation/Rotation.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace nova::module::crystal {
using PredictionVec=rotation::Vec3;
inline bool Finite(PredictionVec v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
inline java::Box ShiftBox(java::Box b,PredictionVec d) {
    return {b.minX+d.x,b.minY+d.y,b.minZ+d.z,b.maxX+d.x,b.maxY+d.y,b.maxZ+d.z};
}
inline bool Overlaps(const java::Box& a,const java::Box& b) {
    return a.maxX>b.minX+1e-6&&a.minX<b.maxX-1e-6&&a.maxY>b.minY+1e-6&&
        a.minY<b.maxY-1e-6&&a.maxZ>b.minZ+1e-6&&a.minZ<b.maxZ-1e-6;
}
struct MotionSample {
    PredictionVec position,velocity;
    int tick=-1;
    void Reset(){*this={};}
    void Update(int now,PredictionVec at,PredictionVec observed,bool grounded) {
        if(!Finite(at)||!Finite(observed)){Reset();return;}
        const int elapsed=now-tick;
        auto measured=tick>=0&&elapsed>0&&elapsed<=2?at.Subtract(position).Scale(1./elapsed):observed;
        // Teleports, pauses and reversals never inherit the previous direction.
        velocity=Finite(measured)&&measured.DistanceTo({})<=1.5?measured:PredictionVec{};
        if(grounded)velocity.y=0;
        position=at;tick=now;
    }
};
struct MotionForecast {
    static constexpr int MaxTicks=8;
    PredictionVec origin;
    java::Box bounds;
    bool grounded=false;
    std::array<PredictionVec,MaxTicks+1> positions{};
    PredictionVec At(double ticks)const {
        ticks=std::clamp(ticks,0.,double(MaxTicks));
        const int i=int(ticks);if(i==MaxTicks)return positions[i];
        const auto delta=positions[i+1].Subtract(positions[i]);
        return positions[i].Add(delta.x*(ticks-i),delta.y*(ticks-i),delta.z*(ticks-i));
    }
    java::Box BoxAt(double ticks)const{return ShiftBox(bounds,At(ticks).Subtract(origin));}
    double NearestDistance(PredictionVec point)const {
        double distance=point.DistanceTo(positions[0]);
        for(int i=0;i<MaxTicks;++i) {
            const auto segment=positions[i+1].Subtract(positions[i]),offset=point.Subtract(positions[i]);
            const double length=segment.x*segment.x+segment.y*segment.y+segment.z*segment.z;
            const double t=length>1e-12?std::clamp((offset.x*segment.x+offset.y*segment.y+offset.z*segment.z)/length,0.,1.):0.;
            distance=std::min(distance,point.DistanceTo(positions[i].Add(segment.x*t,segment.y*t,segment.z*t)));
        }
        return distance;
    }
    bool Crosses(const java::Box& volume,double ticks)const {
        // Half-tick swept boxes also reject walking through a cell between endpoints.
        for(double t=0;t<ticks;t+=.5) {
            auto a=BoxAt(t),b=BoxAt(std::min(t+.5,ticks));
            java::Box swept{std::min(a.minX,b.minX),std::min(a.minY,b.minY),std::min(a.minZ,b.minZ),
                std::max(a.maxX,b.maxX),std::max(a.maxY,b.maxY),std::max(a.maxZ,b.maxZ)};
            if(Overlaps(swept,volume))return true;
        }
        return false;
    }
};
// Short vanilla-like extrapolation. World collisions stop individual axes;
// actual placement and damage are still rechecked against live game state.
template<class Clear> MotionForecast Forecast(const MotionSample& sample,java::Box bounds,bool grounded,Clear clear) {
    MotionForecast out;out.origin=sample.position;out.bounds=bounds;out.grounded=grounded;out.positions[0]=sample.position;
    auto p=sample.position,v=sample.velocity;
    for(int tick=1;tick<=MotionForecast::MaxTicks;++tick) {
        const int steps=std::max(1,int(std::ceil(std::max({std::abs(v.x),std::abs(v.y),std::abs(v.z)})/.25)));
        const std::array<int,3> axes=std::abs(v.x)<std::abs(v.z)?std::array<int,3>{1,2,0}:std::array<int,3>{1,0,2};
        for(int step=0;step<steps;++step)for(int axis:axes) {
            auto next=p;double& value=axis==0?next.x:axis==1?next.y:next.z;
            double& speed=axis==0?v.x:axis==1?v.y:v.z;
            if(std::abs(speed)<1e-7)continue;
            value+=speed/steps;
            if(clear(ShiftBox(bounds,next.Subtract(sample.position))))p=next;
            else {
                // Keep the legal part of this movement instead of stopping a
                // full substep short of a wall/floor. Bounded native queries.
                double low=0,high=1;
                for(int attempt=0;attempt<5;++attempt) {
                    const double mid=(low+high)*.5;auto partial=p;
                    (axis==0?partial.x:axis==1?partial.y:partial.z)+=speed/steps*mid;
                    if(clear(ShiftBox(bounds,partial.Subtract(sample.position))))low=mid;else high=mid;
                }
                (axis==0?p.x:axis==1?p.y:p.z)+=speed/steps*low;
                if(axis==1&&speed<0)grounded=true;speed=0;
            }
        }
        out.positions[tick]=p;
        if(!grounded)v.y=(v.y-.08)*.98;
    }
    return out;
}
inline double RotationTicks(rotation::Rotation from,rotation::Rotation to,double speed) {
    double distance=std::hypot(std::remainder(double(to.yaw-from.yaw),360.),double(to.pitch-from.pitch));
    int ticks=0;
    // Match the module's half-distance approach, rather than a constant-speed ETA.
    while(distance>1&&ticks<MotionForecast::MaxTicks){distance-=std::min(std::max(1.,speed),distance*.5);++ticks;}
    return ticks+(distance>.001?1:0);
}
template<class Clear> MotionForecast ForecastWithBase(const MotionSample& sample,const MotionForecast& world,
                                                       const java::Box& base,Clear clear) {
    if(!world.Crosses(base,MotionForecast::MaxTicks))return world;
    // The same planned solid used for exposure must stop predicted movement.
    // Live occupancy and the actual support hit are checked separately before use.
    return Forecast(sample,world.bounds,world.grounded,[&](const java::Box& box){
        return !Overlaps(box,base)&&clear(box);
    });
}
// Mirrors the rotation history relevant to DuplicateRotPlace. Unchanged motion
// does not erase the last look delta; the post-motion callback finalizes a use.
class PlacementTurnGate {
public:
    void Reset(){*this={};}
    void Seed(rotation::Rotation angle){if(!known_){motion_=angle;known_=true;}}
    bool Ready(rotation::Rotation next)const {
        if(!known_||!std::isfinite(next.yaw)||!std::isfinite(next.pitch))return false;
        const bool changed=next.yaw!=motion_.yaw||next.pitch!=motion_.pitch;
        if(!changed&&!rotated_)return true;
        const double delta=changed?std::abs(double(next.yaw-motion_.yaw)):lookDelta_;
        return !placed_ || delta<=2. || std::abs(delta-placedDelta_)>=.0001;
    }
    void Submitted(){pending_=true;}
    void Observe(rotation::Rotation next) {
        if(!std::isfinite(next.yaw)||!std::isfinite(next.pitch)){Reset();return;}
        if(known_&&(next.yaw!=motion_.yaw||next.pitch!=motion_.pitch)) {
            lookDelta_=std::abs(double(next.yaw-motion_.yaw));rotated_=true;
        }
        motion_=next;known_=true;
        if(pending_) {
            if(rotated_){placedDelta_=lookDelta_;placed_=true;rotated_=false;}
            pending_=false;
        }
    }
private:
    rotation::Rotation motion_{};
    double lookDelta_=0,placedDelta_=0;
    bool known_=false,rotated_=false,placed_=false,pending_=false;
};
}
