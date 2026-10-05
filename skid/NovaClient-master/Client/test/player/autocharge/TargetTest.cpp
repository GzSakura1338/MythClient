#include "minecraft/modules/player/autocharge/Targeting.h"
#include <cassert>
#include <cstdio>
#include <chrono>
#include <random>
using namespace nova::module::autocharge;
namespace nova::rotation {double Random(){return .5;}}
// Independent swept collision oracle: no production Impact/SegmentBox calls.
bool ActualDestination(Pearl pearl,Vec3 eye,Vec3 inherited,nova::rotation::Rotation angle,Vec3 spread,Vec3& destination,int windLead=0){
    constexpr double rad=3.14159265358979323846/180.;
    Vec3 windVelocity{-std::sin(angle.yaw*rad)*std::cos(angle.pitch*rad)*1.5,
        -std::sin(angle.pitch*rad)*1.5,std::cos(angle.yaw*rad)*std::cos(angle.pitch*rad)*1.5};
    windVelocity=Add(Add(windVelocity,inherited),spread);
    if(windLead)eye=Add(eye,windVelocity);
    for(int t=0;t<32;++t){
        const auto start=pearl.position;
        pearl.velocity={pearl.velocity.x*double(.99f),(pearl.velocity.y-.03)*double(.99f),pearl.velocity.z*double(.99f)};
        pearl.position=Add(start,pearl.velocity);++pearl.age;
        const double radius=.15625+double(std::clamp(float(pearl.age-2)/20.f,0.f,.3f));
        const double from[]{start.x,start.y,start.z},delta[]{pearl.velocity.x,pearl.velocity.y,pearl.velocity.z};
        const double center[]{eye.x,eye.y+.15625-double(.15f),eye.z};
        double enter=0,leave=1;bool valid=true;
        for(int axis=0;axis<3;++axis){
            if(std::abs(delta[axis])<1e-12){if(std::abs(from[axis]-center[axis])>radius)valid=false;}
            else{double a=(center[axis]-radius-from[axis])/delta[axis],b=(center[axis]+radius-from[axis])/delta[axis];
                if(a>b)std::swap(a,b);enter=std::max(enter,a);leave=std::min(leave,b);}
        }
        if(valid&&enter<=leave){destination=start;return true;}eye=Add(eye,windVelocity);
    }
    return false;
}
int main(){
    std::setvbuf(stdout,nullptr,_IONBF,0);
    const auto clear=[](auto,auto){return true;};
    assert(TargetInView({},{0,0,8},0,90,10));
    assert(!TargetInView({},{0,0,-8},0,90,10));
    assert(TargetInView({},{0,0,-8},0,360,10));
    assert(!TargetInView({},{0,0,11},0,360,10));
    TargetMotion motion;assert(motion.Update({0,1.8,10},1));assert(motion.Update({.3,1.8,10},2));
    assert(std::abs(motion.velocity.x-.3)<1e-9);assert(!motion.Update({30,1.8,10},3));
    int cases=0,launches=0,fired=0,randomFired=0,randomCases=0,randomHits=0,randomTrials=0;double maxMs=0;
    std::mt19937 rng(0x294030);std::uniform_real_distribution<double> unit(0,1);
    auto noise=[&]{auto axis=[&]{const double a=unit(rng),b=unit(rng);return (a-b)*.02584125;};return Vec3{axis(),axis(),axis()};};
    for(double range:{4.,8.,16.})for(double height:{5.,10.,20.,25.})for(double movement:{0.,.28})for(double targetSpeed:{0.,.12}){
        ++cases;const Vec3 eye{0,1.62,0},inherited{movement,0,0};
        InterceptTarget goal{{0,height+1.8,range},{targetSpeed,0,0}};
        nova::rotation::Rotation angle;
        if(!TargetPearlAngle(eye,inherited,goal,angle)){std::printf("UNREACHABLE %.0f %.0f %.2f\n",range,height,movement);continue;}
        angle=AimStep({0,0},180,.5).Quantize(angle);
        const auto start=std::chrono::steady_clock::now();
        if(!TargetLaunchReady(eye,inherited,goal,angle,90,.5,true,clear)){
            const bool aimed=TargetPearlAngle(eye,inherited,goal,angle,40.);
            angle=AimStep({0,0},180,.5).Quantize(angle);
            if(!aimed||!TargetLaunchReady(eye,inherited,goal,angle,90,.5,true,clear)){
                assert(range>8); // Nearby Height=5 cases previously failed here.
                std::printf("NO PREFLIGHT %.0f %.0f %.2f\n",range,height,movement);continue;
            }
        }
        maxMs=std::max(maxMs,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());++launches;
        const auto launchAngle=angle;
        Pearl pearl{eye.Add(0,-kPearlEyeOffset,0),Add(Look(angle).Scale(kLaunchSpeed),inherited),0};
        bool shot=false;
        for(int age=1;age<=100;++age){
            Step(pearl);if(age<2)continue;
            const auto now=goal.Advanced(age);
            const auto plan=PlanWind(pearl,Add(eye,inherited.Scale(age)),inherited,now.point.y,age,angle,90,.5,true,clear,false,&now);
            angle=plan.angle;
            if(plan.decision==WindDecision::Fire){assert(now.Accepts(plan.impact));++fired;shot=true;break;}
        }
        if(!shot)std::printf("FAILED AFTER PREFLIGHT %.0f %.0f %.2f\n",range,height,movement);
        assert(shot);
        for(int sample=0;sample<4;++sample){
            ++randomCases;
            Pearl observed{eye.Add(0,-kPearlEyeOffset,0),Add(Add(Look(launchAngle).Scale(kLaunchSpeed),inherited),noise()),0};
            auto actualAngle=launchAngle;
            for(int age=1;age<=100;++age){
                Step(observed);if(age<2)continue;
                const auto now=goal.Advanced(age);const auto windEye=Add(eye,inherited.Scale(age));
                const auto plan=PlanWind(observed,windEye,inherited,now.point.y,age,actualAngle,90,.5,true,clear,false,&now);
                actualAngle=plan.angle;if(plan.decision!=WindDecision::Fire)continue;
                ++randomFired;assert(now.Accepts(plan.impact));
                for(int wind=0;wind<256;++wind){Vec3 destination;++randomTrials;randomHits+=ActualDestination(observed,windEye,inherited,actualAngle,noise(),destination);}
                break;
            }
        }
    }
    std::printf("Target launch scenarios=%d preflight=%d fired=%d maxPreflight=%.3fms\n",cases,launches,fired,maxMs);
    assert(launches>=32&&fired==launches);
    std::printf("Random pearl spread: %d/%d wind submissions; independent wind collision samples %d/%d\n",randomFired,randomCases,randomHits,randomTrials);
    assert(randomFired>=randomCases*.9&&randomHits>=randomTrials*.95);
    nova::rotation::Rotation low,high;
    assert(TargetPearlAngle({0,1.62,0},{},{{0,5,8},{}},low));
    assert(TargetPearlAngle({0,1.62,0},{},{{0,60,8},{}},high));
    assert(low.yaw==high.yaw&&low.pitch==high.pitch);
    assert(!TargetLaunchReady({0,1.62,0},{},{{0,21.8,8},{}},{0,-75},90,.5,true,[](auto,auto){return false;}));
    // A post-launch target turn used to veto the wind until the pearl expired.
    // Compare the old hard destination gate with committed catch priority.
    int committedShots=0,strictShots=0,fallbackShots=0,trials=0,hits=0,windFirstHits=0;
    for(int seed=0;seed<24;++seed){
        const Vec3 eye{0,1.62,0},move{seed%2?.28:0,0,0};
        InterceptTarget initial{{0,1.8,8},{}};
        nova::rotation::Rotation launchAngle;
        assert(TargetPearlAngle(eye,move,initial,launchAngle));
        launchAngle=AimStep({0,0},180,.5).Quantize(launchAngle);
        const auto pearlSpread=noise();
        for(bool committed:{false,true}){
            auto angle=launchAngle;
            Pearl actual{eye.Add(0,-kPearlEyeOffset,0),Add(Add(Look(angle).Scale(kLaunchSpeed),move),pearlSpread),0};
            bool fired=false;
            for(int age=1;age<=80;++age){
                Step(actual);if(age<2+seed%4)continue;
                // Includes a suddenly moving, turning, or disappearing target
                // (the latter retains its last XZ with zero extrapolation).
                const double side=(seed%3==0?0:seed%3==1?.4:-.6);
                InterceptTarget now{{side*age,1.8,8},{side,0,0}};
                const auto windEye=Add(eye,move.Scale(age));
                const auto plan=PlanWind(actual,windEye,move,now.point.y,age,angle,90,.5,true,clear,false,&now,committed);
                angle=plan.angle;if(plan.decision!=WindDecision::Fire)continue;
                if(committed){
                    ++committedShots;fallbackShots+=plan.targetFallback;
                    for(int i=0;i<512;++i){Vec3 destination;++trials;const auto spread=noise();
                        hits+=ActualDestination(actual,windEye,move,angle,spread,destination);
                        windFirstHits+=ActualDestination(actual,windEye,move,angle,spread,destination,1);
                    }
                }else{++strictShots;}
                fired=true;break;
            }
            if(committed)assert(fired); // Count failures to throw, not just accuracy of submitted shots.
        }
    }
    std::printf("Post-launch target changes: strict=%d/24 committed=%d/24 fallback=%d collisions=%d/%d windFirst=%d/%d\n",strictShots,committedShots,fallbackShots,hits,trials,windFirstHits,trials);
    assert(committedShots==24&&fallbackShots>0&&hits>=trials*.95);
}
