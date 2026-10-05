#pragma once
#include "minecraft/modules/player/autocharge/Prediction.h"
#include "minecraft/utils/rotation/LinearAim.h"

namespace nova::module::autocharge {
// One immutable sensitivity lattice per tick. Calling the generic patch for
// each candidate used a fresh random perturbation and then quantized twice.
class AimStep {
public:
    AimStep(rotation::Rotation from,double speed,double sensitivity,bool valid=true):from_(from),speed_(std::isfinite(speed)?std::clamp(speed,1.,180.):90.){
        if(valid&&std::isfinite(sensitivity)){
            const float f=static_cast<float>(std::clamp(sensitivity,0.,1.)*.6f+.2f);
            unit_=double(f*f*f*8.f)*.15;
        }
    }
    rotation::Rotation Quantize(rotation::Rotation value) const {
        value.yaw=from_.yaw+float(std::remainder(double(value.yaw)-from_.yaw,360.));
        if(unit_>0){
            value.yaw=from_.yaw+float(std::floor((value.yaw-from_.yaw)/unit_+.5)*unit_);
            value.pitch=from_.pitch+float(std::floor((value.pitch-from_.pitch)/unit_+.5)*unit_);
        }
        value.pitch=std::clamp(value.pitch,-90.f,90.f);return value;
    }
    rotation::Rotation operator()(rotation::Rotation target) const {
        return Quantize(rotation::LinearStep(from_,target,speed_));
    }
private:
    rotation::Rotation from_;double speed_,unit_=0;
};
enum class WindDecision { NoSolution, Tracking, Waiting, Fire };
struct WindPlan {
    WindDecision decision=WindDecision::NoSolution;
    rotation::Rotation angle{};
    HeightImpact impact{};
    InterceptQuality quality{};
    bool targetFallback=false;
};
template<class Clear>
WindPlan PlanWind(Pearl pearl,Vec3 eye,Vec3 inherited,double targetY,int elapsed,
                  rotation::Rotation current,double speed,double sensitivity,bool sensitivityValid,Clear&& clear,bool allowWait=true,
                  const InterceptTarget* target=nullptr,bool committed=false){
    WindPlan plan;plan.angle=current;
    if(!Finite(pearl.position)||!Finite(pearl.velocity)||!Finite(eye)||!Finite(inherited)||
       !std::isfinite(targetY)||!std::isfinite(current.yaw)||!std::isfinite(current.pitch))return plan;
    const AimStep step(current,speed,sensitivity,sensitivityValid);
    // Rank angles the player can publish THIS tick, not an ideal angle outside
    // the smoothing budget. The same returned angle is submitted unchanged.
    bool solved=SolveHeight(pearl,eye,inherited,targetY,step,plan.angle,plan.impact,target,true,clear);
    // Once the pearl is in flight its XZ trajectory cannot follow a target's
    // new direction. Prefer the target band, then the nearest reliable catch;
    // never relax collision/spread checks to preserve an impossible XZ goal.
    if(!solved&&target&&committed){
        solved=SolveHeight(pearl,eye,inherited,targetY,step,plan.angle,plan.impact,target,false,clear);
        plan.targetFallback=solved;
    }
    if(!solved){
        Solution tracking;
        if(Solve(pearl,eye,inherited,[&](auto a){return step.Quantize(a);},[](auto,auto){return true;},tracking)){
            plan.angle=step(tracking.angle);plan.decision=WindDecision::Tracking;
        }
        return plan;
    }
    plan.decision=WindDecision::Tracking;
    if(!InterceptReady(pearl,eye,inherited,plan.angle,clear,plan.impact))return plan;
    if(!target&&allowWait&&WaitForHeight(pearl,eye,inherited,targetY,elapsed,plan.impact,plan.angle,
        [&](auto a){return step.Quantize(a);})){plan.decision=WindDecision::Waiting;return plan;}
    StableIntercept(pearl,eye,inherited,plan.angle,&plan.quality);
    plan.decision=WindDecision::Fire;return plan;
}
}
