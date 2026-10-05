#pragma once
#include "minecraft/modules/player/autocharge/Trajectory.h"
#include <cstdint>

namespace nova::module::autocharge {
// The first projectile inherits upward player motion. A later ground launch
// cannot reproduce that speed; wait through ascent before spending the pearl.
// Ground launches zero inherited Y in MovementHistory::Launch.
inline bool ReadyToLaunch(Vec3 inherited){return Finite(inherited)&&inherited.y<=0.;}
inline rotation::Rotation PrimeAngle(Vec3 inherited,float cameraYaw){
    // Pearl yaw is immaterial at -90 degrees. Prepare the rearward heading
    // needed to compensate drag so a slow wind turn need not start 180 deg away.
    const double horizontal=inherited.x*inherited.x+inherited.z*inherited.z;
    const float yaw=Finite(inherited)&&horizontal>.0004?
        rotation::Calculate({},Vec3{-inherited.x,0,-inherited.z}).yaw:cameraYaw;
    return {yaw,-90};
}
// Reproduce the observed movement stream, including rotation-only and empty
// ticks. Render interpolation endpoints are not network movement history.
struct MovementHistory {
    Vec3 position{},movement{},previousMovement{};
    bool initialized=false,ready=false,moved=false,ground=false;
    void Seed(Vec3 feet,bool onGround){if(!initialized&&Finite(feet)){position=feet;ground=onGround;initialized=true;}}
    void Move(bool hasPosition,Vec3 next,bool onGround){
        previousMovement=movement;
        ground=onGround;moved=true;
        if(!hasPosition){movement={};ready=initialized;return;}
        if(!Finite(next)){*this={};return;}
        movement=initialized?next.Subtract(position):Vec3{};
        ready=initialized&&movement.DistanceToSqr({})<16.;
        initialized=true;position=next;
    }
    void EndTick(){if(!moved){previousMovement=movement;movement={};ready=initialized;}moved=false;}
    bool Launch(double eyeHeight,Vec3& eye,Vec3& inherited) const {
        if(!ready||!std::isfinite(eyeHeight))return false;
        eye=position.Add(0,eyeHeight,0);inherited=movement;
        if(ground)inherited.y=0;
        return true;
    }
    bool CanForecast() const {
        return ready&&ground&&movement.Subtract(previousMovement).DistanceToSqr({})<.03*.03;
    }
};

// A received pearl is behind the player's local tick clock. Fit its physical
// flight age to our recorded launch origin, then evaluate it at the interval
// between the two use requests. Network arrival age is never projectile age.
inline bool AlignPearl(Pearl observed,Vec3 launch,int elapsed,Pearl& aligned,double& residual){
    if(!Finite(observed.position)||!Finite(observed.velocity)||elapsed<1||elapsed>kPearlMaxAge)return false;
    Pearl rewind=observed,best{};double error=1e9;int flightAge=-1;
    for(int age=0;age<=kPearlMaxAge;++age){
        const double distance=rewind.position.DistanceToSqr(launch);
        if(distance<error){error=distance;best=rewind;flightAge=age;}
        // Inverse of gravity -> drag -> movement, using the current velocity
        // for the position step before undoing drag and gravity.
        rewind.position=rewind.position.Subtract(rewind.velocity);
        rewind.velocity=rewind.velocity.Scale(1./kPearlDrag).Add(0,kPearlGravity,0);
    }
    residual=std::sqrt(error);
    if(flightAge<0||flightAge>elapsed+2||residual>.30)return false;
    // Use the fitted spawn position (not the nominal direction) to retain
    // the pearl's actual random spread observed from the server.
    aligned={best.position,best.velocity,0};
    for(int t=0;t<elapsed;++t)Step(aligned);
    return Finite(aligned.position)&&Finite(aligned.velocity);
}

// Height is the player's FEET destination relative to feet at pearl submission.
// Vanilla ThrownEnderpearl.onHit uses Entity.oldPosition(), not hit location.
// ServerLevel ticks setOldPosAndRot and increment tickCount before projectile
// collision; the margin is therefore (completedSteps+1-2)/20, clamped as float.
inline constexpr double kHeightTolerance=.75;
inline constexpr int kHeightWaitDeadline=24;
inline bool HeightMatches(const HeightImpact& hit,double targetY){
    return std::isfinite(targetY)&&std::abs(hit.destination.y-targetY)<=kHeightTolerance;
}
// Independent triangular samples of vanilla's (uniform-uniform)*deviation.
// Fixed probes keep planning deterministic. This is a model confidence test,
// not knowledge of server RNG or a guarantee about network tick phase.
inline const std::array<Vec3,128>& WindSpreadProbes(){
    static const auto probes=[] {
        std::array<Vec3,128> values{};std::uint32_t state=0x4e4f5641u;
        auto uniform=[&]{state=1664525u*state+1013904223u;return double(state>>8)/16777216.;};
        auto triangle=[&]{const double a=uniform(),b=uniform();return (a-b)*kLaunchSpread;};
        for(auto& v:values)v={triangle(),triangle(),triangle()};
        return values;
    }();
    return probes;
}
struct InterceptQuality { int misses=0,edgeMisses=0,phaseMisses=0,phaseSpreadMisses=0,worstPhaseMisses=0; double margin=0; };
inline bool StableIntercept(Pearl pearl,Vec3 eye,Vec3 inherited,rotation::Rotation angle,
                            InterceptQuality* quality=nullptr){
    InterceptQuality result;
    const PearlPath path(pearl);
    for(const auto& spread:WindSpreadProbes()){
        HeightImpact hit;
        if(!Impact(path,eye,inherited,angle,spread,[](auto,auto){return true;},hit))
            {if(++result.misses>2)return false;}
        else result.margin+=hit.clearance;
    }
    if(quality){
        result.margin/=WindSpreadProbes().size();
        // Random probes rarely sample the tails. Deterministic face/edge/corner
        // probes expose marginal solutions without becoming a new hard veto.
        for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y)for(int z=-1;z<=1;++z){
            if(x==0&&y==0&&z==0)continue;
            const auto spread=Vec3{double(x),double(y),double(z)}.Scale(kLaunchSpread);
            HeightImpact hit;
            if(!Impact(path,eye,inherited,angle,spread,[](auto,auto){return true;},hit))++result.edgeMisses;
        }
        // Packet delivery/local clocks need not put both uses at the same
        // server tick phase. Prefer angles that tolerate one pearl tick of
        // alignment error. Reliable() applies the final phase-spread limit.
        for(int phase:{-1,1}){
            auto shifted=pearl;
            if(phase>0)Step(shifted);
            else {shifted.position=shifted.position.Subtract(shifted.velocity);
                shifted.velocity=shifted.velocity.Scale(1./kPearlDrag).Add(0,kPearlGravity,0);--shifted.age;}
            HeightImpact hit;
            const PearlPath shiftedPath(shifted);
            if(!Impact(shiftedPath,eye,inherited,angle,{},[](auto,auto){return true;},hit))++result.phaseMisses;
            int misses=0;
            for(const auto& spread:WindSpreadProbes())
                if(!Impact(shiftedPath,eye,inherited,angle,spread,[](auto,auto){return true;},hit))++misses;
            result.phaseSpreadMisses+=misses;
            result.worstPhaseMisses=std::max(result.worstPhaseMisses,misses);
        }
        *quality=result;
    }
    return true;
}
inline double HeightCost(const HeightImpact& hit,double targetY){
    // All destinations in the height tolerance band are equally useful. Do
    // not trade reliable contact for a few centimetres of nominal precision.
    return std::max(0.,std::abs(hit.destination.y-targetY)-kHeightTolerance);
}
inline double InterceptCost(const HeightImpact& hit,double targetY,const InterceptQuality& quality){
    return HeightCost(hit,targetY)+quality.misses*.6+quality.edgeMisses*.4+quality.phaseMisses*3.+
        quality.worstPhaseMisses*3.+quality.phaseSpreadMisses*.2+
        std::max(0.,.2-quality.margin)*2.+hit.ticks*.01;
}
inline bool Reliable(const InterceptQuality& quality){
    return quality.misses<=1&&quality.worstPhaseMisses<=2;
}
// Horizontal destination with a bounded horizontal-motion
// estimate. Flight time is relative to the wind submission being evaluated.
struct InterceptTarget {
    Vec3 point{},velocity{};
    Vec3 At(double ticks) const{return Add(point,velocity.Scale(ticks));}
    InterceptTarget Advanced(double ticks) const{return {At(ticks),velocity};}
    double HorizontalError(const HeightImpact& hit) const {
        const auto d=hit.destination.Subtract(At(hit.ticks-1));return std::hypot(d.x,d.z);
    }
    bool Accepts(const HeightImpact& hit) const {
        return Finite(point)&&Finite(velocity)&&HorizontalError(hit)<=2.;
    }
};
template<class Clear>
bool InterceptReady(Pearl pearl,Vec3 eye,Vec3 inherited,rotation::Rotation angle,
                 Clear&& clear,HeightImpact& hit){
    InterceptQuality quality;
    return Impact(pearl,eye,inherited,angle,{},clear,hit)&&hit.contact.y>eye.y+1&&
        hit.clearance>=.035 && StableIntercept(pearl,eye,inherited,angle,&quality)&&Reliable(quality);
}
// Height ranks reliable interceptions; it must never veto all wind throws.
// Tick-discrete collisions and random spread cannot satisfy an arbitrary exact
// height. Always retain the closest safe attainable candidate.
struct UnobstructedPath {bool operator()(Vec3,Vec3) const {return true;}};
template<class Quantize,class Clear=UnobstructedPath>
bool SolveHeight(Pearl pearl,Vec3 eye,Vec3 inherited,double targetY,Quantize&& quantize,
                 rotation::Rotation& angle,HeightImpact& out,const InterceptTarget* target=nullptr,bool requireTarget=true,Clear clear={}){
    if(!std::isfinite(targetY))return false;
    bool found=false;double best=1e9;
    const PearlPath path(pearl);
    std::unordered_set<std::uint64_t> visited;visited.reserve(256);
    auto visit=[&](auto candidate){
      // A reachable box is not a point. Small quantized neighbours can move
      // the first collision to the desired discrete pearl tick without using
      // the later centre crossing as a fictitious teleport destination.
      constexpr rotation::Rotation offsets[]{{0,0},{0,-.25f},{0,.25f},{0,-.5f},{0,.5f},
          {-.25f,0},{.25f,0},{-.5f,0},{.5f,0},
          {0,-1},{0,1},{-1,0},{1,0},{0,-2},{0,2},{-2,0},{2,0}};
      for(const auto offset:offsets){
        const auto adjusted=quantize(rotation::Rotation{candidate.yaw+offset.yaw,candidate.pitch+offset.pitch});
        const auto key=(std::uint64_t(std::bit_cast<std::uint32_t>(adjusted.yaw))<<32)|std::bit_cast<std::uint32_t>(adjusted.pitch);
        if(!visited.insert(key).second)continue;
        HeightImpact hit;
        if(!Impact(path,eye,inherited,adjusted,{},[](auto,auto){return true;},hit)||
           hit.contact.y<=eye.y+1||hit.clearance<.035)continue;
        if(target&&requireTarget&&!target->Accepts(hit))continue;
        const double horizontalCost=target?target->HorizontalError(hit)*3.:0.;
        if((target?0.:HeightCost(hit,targetY))+horizontalCost+hit.ticks*.01>=best)continue;
        InterceptQuality quality;
        if(!StableIntercept(pearl,eye,inherited,adjusted,&quality)||!Reliable(quality))continue;
        const double score=InterceptCost(hit,target?hit.destination.y:targetY,quality)+horizontalCost;
        if(score>=best)continue;
        // A geometrically optimal but obstructed trajectory must not suppress
        // other clear candidates for the entire remaining flight.
        HeightImpact visible;
        if(!Impact(path,eye,inherited,adjusted,{},clear,visible))continue;
        best=score;angle=adjusted;out=hit;found=true;
      }
    };
    const auto raw=[](rotation::Rotation a){return a;};
    Candidates(pearl,eye,inherited,raw,visit);
    auto shifted=pearl;Step(shifted);Candidates(shifted,eye,inherited,raw,visit);
    shifted=pearl;shifted.position=shifted.position.Subtract(shifted.velocity);
    shifted.velocity=shifted.velocity.Scale(1./kPearlDrag).Add(0,kPearlGravity,0);--shifted.age;
    Candidates(shifted,eye,inherited,raw,visit);
    return found;
}
// Short rolling lookahead is only a reason to delay a currently valid shot.
// Future movement is an estimate, never used directly to authorize a throw.
// A hard deadline on height waiting prevents a receding target from starving
// the second use. After it, collision/smoothing checks still remain mandatory.
template<class Quantize>
bool WaitForHeight(Pearl pearl,Vec3 eye,Vec3 inherited,double targetY,int elapsed,
                  const HeightImpact& current,rotation::Rotation currentAngle,Quantize&& quantize){
    // Constant-Y extrapolation is not a forecast of an airborne player. Do not
    // discard a verified shot for a fictional future launch position.
    if(elapsed>=kHeightWaitDeadline||std::abs(inherited.y)>.001)return false;
    InterceptQuality quality;
    // Use the same reliability cost as angle selection. Height-only waiting
    // used to discard a good current window in favour of a fragile apex hit.
    if(!StableIntercept(pearl,eye,inherited,currentAngle,&quality))return false;
    // Being at the requested height is not sufficient when a one-tick phase
    // error makes the same angle unreliable. Compare later robust windows too.
    if(HeightMatches(current,targetY)&&quality.misses==0&&quality.worstPhaseMisses<=2)return false;
    const double cost=InterceptCost(current,targetY,quality);
    const int horizon=std::min(8,kHeightWaitDeadline-elapsed);
    for(int delay=1;delay<=horizon;++delay){
        Step(pearl);eye=Add(eye,inherited);
        rotation::Rotation angle;HeightImpact future;
        if(SolveHeight(pearl,eye,inherited,targetY,quantize,angle,future)&&
           StableIntercept(pearl,eye,inherited,angle,&quality)&&
           InterceptCost(future,targetY,quality)+.15<cost)return true;
    }
    return false;
}
inline double PearlApex(Pearl pearl){
    double top=pearl.position.y;
    for(int tick=0;tick<kPearlMaxAge && pearl.velocity.y>0;++tick){Step(pearl);top=std::max(top,pearl.position.y);}
    return top;
}
}
