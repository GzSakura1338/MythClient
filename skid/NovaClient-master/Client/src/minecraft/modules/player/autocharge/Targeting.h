#pragma once
#include "minecraft/modules/player/autocharge/WindPlanner.h"

namespace nova::module::autocharge {
inline double Setting(double value,double low,double high,double fallback){
    return std::isfinite(value)?std::clamp(value,low,high):fallback;
}
inline bool TargetInView(Vec3 self,Vec3 feet,float cameraYaw,double fov,double range){
    if(!Finite(self)||!Finite(feet)||!std::isfinite(cameraYaw))return false;
    const auto d=feet.Subtract(self);
    if(d.DistanceToSqr({})>range*range)return false;
    if(d.x*d.x+d.z*d.z<1e-8)return true;
    const auto yaw=rotation::Calculate(self,feet).yaw;
    return std::abs(std::remainder(double(yaw)-cameraYaw,360.))<=fov*.5;
}
struct TargetMotion {
    Vec3 head{},velocity{};int tick=-1;
    bool Update(Vec3 next,int now){
        if(!Finite(next))return false;
        if(tick>=0&&now>tick){
            const auto delta=next.Subtract(head).Scale(1./double(now-tick));
            if(delta.DistanceToSqr({})>4.)return false; // Teleport / stale entity.
            velocity={delta.x,0,delta.z};
            const double speed=std::hypot(velocity.x,velocity.z);
            if(speed>.8)velocity=velocity.Scale(.8/speed);
        }
        head=next;tick=now;return true;
    }
    InterceptTarget Goal()const{return {head,velocity};}
};
// Invert the horizontal drag recurrence; Y is free and uses the remaining
// launch speed. Final acceptance uses the full discrete collision model.
inline bool TargetPearlAngle(Vec3 eye,Vec3 inherited,const InterceptTarget& goal,rotation::Rotation& angle,double time=20.){
    if(!Finite(eye)||!Finite(inherited)||!Finite(goal.point)||!Finite(goal.velocity))return false;
    // XZ fixes the horizontal launch velocity. The remaining launch energy
    // points upward; neither the Height setting nor target Y chooses the arc.
    if(!std::isfinite(time)||time<2||time>80)return false;
    const double sum=kPearlDrag*(1.-std::pow(kPearlDrag,time))/(1.-kPearlDrag);
    const auto displacement=goal.At(time).Subtract(eye);
    const double x=displacement.x/sum-inherited.x,z=displacement.z/sum-inherited.z;
    const double vertical2=kLaunchSpeed*kLaunchSpeed-x*x-z*z;
    if(vertical2<=0)return false;
    angle=rotation::Calculate({},Vec3{x,std::sqrt(vertical2),z});
    return angle.pitch<=kWindMaxPitch;
}
template<class Clear>
bool TargetLaunchReady(Vec3 eye,Vec3 inherited,const InterceptTarget& goal,rotation::Rotation angle,
                       double speed,double sensitivity,bool sensitivityValid,Clear&& clear){
    if(!ReadyToLaunch(inherited))return false;
    Pearl pearl{eye.Add(0,-kPearlEyeOffset,0),Add(Look(angle).Scale(kLaunchSpeed),inherited),0};
    // Cheap reach test prevents expensive interception searches while still
    // smoothing, or after moving away from the chosen ballistic direction.
    auto probe=pearl;bool reachable=false;
    for(int t=1;t<=100;++t){Step(probe);const auto error=probe.position.Subtract(goal.At(t));
        if(std::hypot(error.x,error.z)<=1.5){reachable=true;break;}}
    if(!reachable)return false;
    for(int delay=1;delay<=80;++delay){
        const auto old=pearl.position;Step(pearl);if(!clear(old,pearl.position))return false;
        if(delay<2||delay%2)continue;
        const auto windEye=Add(eye,inherited.Scale(delay));const auto future=goal.Advanced(delay);
        // Reject windows outside the destination band before searching angles.
        auto path=pearl;bool nearTarget=false;
        for(int t=1;t<=kFlightHorizon;++t){
            HeightImpact possible;possible.destination=path.position;possible.ticks=t;
            if(future.Accepts(possible)){nearTarget=true;break;}Step(path);
        }
        if(!nearTarget)continue;
        const AimStep step(angle,Setting(speed,1,180,90)*delay,sensitivity,sensitivityValid);
        rotation::Rotation wind;HeightImpact hit;
        if(SolveHeight(pearl,windEye,inherited,future.point.y,step,wind,hit,&future,true,clear)&&
           InterceptReady(pearl,windEye,inherited,wind,clear,hit)&&future.Accepts(hit))return true;
    }
    return false;
}
}
