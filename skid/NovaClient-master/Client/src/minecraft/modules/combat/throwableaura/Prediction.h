#pragma once
#include "minecraft/modules/combat/throwableaura/Trajectory.h"
#include "minecraft/utils/rotation/Rotation.h"

namespace nova::module::throwableaura {
inline double Length(Vec3 v) { return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z); }
inline Vec3 Plus(Vec3 a,Vec3 b) { return a.Add(b.x,b.y,b.z); }
inline Vec3 ShiftAt(Vec3 velocity, bool grounded, double ticks) {
    if (grounded) return {velocity.x*ticks,0,velocity.z*ticks};
    const double n=std::floor(ticks), part=ticks-n;
    const double sum=(1-std::pow(.98,n))/.02+part*std::pow(.98,n);
    return {velocity.x*ticks,velocity.y*sum-(.08*.98/.02)*(ticks-sum),velocity.z*ticks};
}
struct TargetMotion {
    Bounds bounds;
    Vec3 velocity;
    bool grounded=true;
};
struct InterceptSolution {
    rotation::Rotation aim;
    double ticks=0;
    bool valid=false;
};

// Motion is sampled once per game tick. Server-side teleports and direction
// reversals invalidate the short observation window rather than extending a
// stale lead indefinitely. This is a stability heuristic, not a probability.
struct MotionHistory {
    int id=-1,lastTick=-1,stable=0;
    Vec3 position,velocity;
    double uncertainty=0;
    bool valid=false;
    void Reset() { *this={}; }
    void Sample(int entityId,int tick,Vec3 at,Vec3 observed,bool grounded) {
        if (!Finite(at)||!Finite(observed)) { Reset();return; }
        if (id==entityId && lastTick==tick) return;
        const int elapsed=tick-lastTick;
        if (!valid || id!=entityId || elapsed<=0 || elapsed>3) {
            id=entityId;lastTick=tick;position=at;
            velocity=grounded?Vec3{observed.x,0,observed.z}:observed;
            valid=Length(velocity)<3;stable=valid?1:0;uncertainty=0;return;
        }
        Vec3 measured=at.Subtract(position).Scale(1.0/elapsed);
        if (grounded) measured.y=0;
        const double change=std::hypot(measured.x-velocity.x,measured.z-velocity.z);
        uncertainty=std::max(change,uncertainty*.55);
        const bool plausible=Length(measured)<3;
        stable=plausible && change<.16 ? std::min(stable+elapsed,20) : 0;
        // Recent positions are more reliable than remote Entity.motion fields.
        velocity=measured;position=at;lastTick=tick;valid=plausible;
    }
    bool Ready() const { return valid && stable>=2 && uncertainty<.16; }
};

inline Vec3 Launch(Vec3 eye,float yaw) {
    const double y=yaw*rotation::kToRadians;
    return eye.Add(-std::cos(y)*.16,-.1,-std::sin(y)*.16);
}
inline Vec3 Direction(float yaw,float pitch) {
    const double y=yaw*rotation::kToRadians,p=pitch*rotation::kToRadians;
    return {-std::sin(y)*std::cos(p),-std::sin(p),std::cos(y)*std::cos(p)};
}
inline double Travel(double ticks) {
    const double n=std::floor(ticks);
    return (1-std::pow(.99,n))/.01+(ticks-n)*std::pow(.99,n);
}
inline InterceptSolution SolveIntercept(Vec3 eye,const TargetMotion& target) {
    InterceptSolution result;
    if (!Finite(eye)||!Finite(target.velocity)||!Finite(target.bounds.min)||!Finite(target.bounds.max) ||
        target.bounds.max.x<=target.bounds.min.x || target.bounds.max.y<=target.bounds.min.y ||
        target.bounds.max.z<=target.bounds.min.z || Length(target.velocity)>=3) return result;
    const Vec3 center=Plus(target.bounds.min,target.bounds.max).Scale(.5);
    Vec3 start=eye.Add(0,-.1,0);
    // Solve the earliest low arc, iterating the yaw-dependent launch offset.
    for(int pass=0;pass<3;++pass) {
        auto required=[&](double ticks) {
            Vec3 delta=Plus(center,ShiftAt(target.velocity,target.grounded,ticks)).Subtract(start);
            delta.y+=.03/.01*(ticks-Travel(ticks));
            return delta;
        };
        double lo=0,hi=0;
        for(double t=.25;t<=24;t+=.25) {
            if (Length(required(t))<=1.5*Travel(t)) {hi=t;break;}
            lo=t;
        }
        if (hi==0) return {};
        for(int i=0;i<16;++i) {
            const double middle=(lo+hi)*.5;
            if(Length(required(middle))>1.5*Travel(middle))lo=middle;else hi=middle;
        }
        const Vec3 delta=required(hi);
        result.aim={float(std::atan2(delta.z,delta.x)*rotation::kToDegrees-90),
            float(-std::atan2(delta.y,std::hypot(delta.x,delta.z))*rotation::kToDegrees)};
        result.ticks=hi;
        start=Launch(eye,result.aim.yaw);
    }
    result.valid=std::isfinite(result.aim.yaw)&&std::isfinite(result.aim.pitch);
    return result;
}

// Moving-box sweep in relative space, using the same discrete projectile
// gravity/drag as the preview. Trace returns the first world collision fraction.
template<class Trace>
bool PredictedHit(Vec3 eye,rotation::Rotation aim,const TargetMotion& target,Trace&& trace) {
    if (!Finite(eye)||!Finite(target.velocity)||!std::isfinite(aim.yaw)||!std::isfinite(aim.pitch))return false;
    Vec3 at=Launch(eye,aim.yaw),motion=Direction(aim.yaw,aim.pitch).Scale(1.5);
    const Bounds box{target.bounds.min.Add(-.3,-.3,-.3),target.bounds.max.Add(.3,.3,.3)};
    for(int tick=0;tick<24;++tick) {
        const Vec3 end=Plus(at,motion);
        double entityFraction=0,blockFraction=1;
        const bool hit=Intersect(box,at.Subtract(ShiftAt(target.velocity,target.grounded,tick)),
            end.Subtract(ShiftAt(target.velocity,target.grounded,tick+1)),entityFraction);
        bool blocked=false;
        if(!trace(at,end,blocked,blockFraction) || (blocked && !std::isfinite(blockFraction)))return false;
        if(blocked && (!hit || blockFraction<=entityFraction))return false;
        if(hit)return true;
        at=end;motion=motion.Scale(.99);motion.y-=.03;
    }
    return false;
}
inline bool RobustHit(Vec3 eye,rotation::Rotation aim,const TargetMotion& target,double uncertainty) {
    int hits=0;
    for(int yaw=-1;yaw<=1;++yaw)for(int pitch=-1;pitch<=1;++pitch) {
        TargetMotion variant=target;
        // Probe both launch spread and small errors in extrapolated movement.
        variant.velocity.x+=yaw*std::min(.06,uncertainty);
        variant.velocity.z+=pitch*std::min(.06,uncertainty);
        if(PredictedHit(eye,{aim.yaw+yaw*.75f,aim.pitch+pitch*.75f},variant,
            [](Vec3,Vec3,bool& block,double& fraction){block=false;fraction=1;return true;}))++hits;
    }
    return hits>=8; // Eight of nine deterministic probes, NOT an 89% hit chance.
}
inline int AdaptiveDelay(int configured,double flightTicks,double uncertainty) {
    return std::max(configured,std::clamp(int(std::ceil(flightTicks*.35+uncertainty*12)),1,12));
}
}
