#include <cassert>
#include <iostream>
#include "minecraft/modules/combat/throwableaura/Prediction.h"
using namespace nova::module::throwableaura;
using nova::rotation::Vec3;
const auto clear=[](Vec3,Vec3,bool& blocked,double& fraction){blocked=false;fraction=1;return true;};

// Independent, finer time-step collision check. The production gate sweeps
// relative segments; here we sample the projectile/target overlap directly.
bool SampleHit(Vec3 eye,nova::rotation::Rotation aim,TargetMotion target) {
    Vec3 pos=Launch(eye,aim.yaw),v=Direction(aim.yaw,aim.pitch).Scale(1.5);
    Vec3 targetOffset{},targetVelocity=target.velocity;
    for(int tick=0;tick<24;++tick) {
        for(int s=0;s<=100;++s) {
            const double f=s/100.;
            const auto p=pos.Add(v.x*f,v.y*f,v.z*f).Subtract(
                targetOffset.Add(targetVelocity.x*f,target.grounded?0:targetVelocity.y*f,targetVelocity.z*f));
            if(p.x>=target.bounds.min.x-.3 && p.x<=target.bounds.max.x+.3 &&
               p.y>=target.bounds.min.y-.3 && p.y<=target.bounds.max.y+.3 &&
               p.z>=target.bounds.min.z-.3 && p.z<=target.bounds.max.z+.3)return true;
        }
        pos=Plus(pos,v);v=v.Scale(.99);v.y-=.03;
        targetOffset=targetOffset.Add(targetVelocity.x,target.grounded?0:targetVelocity.y,targetVelocity.z);
        if(!target.grounded)targetVelocity.y=(targetVelocity.y-.08)*.98;
    }
    return false;
}
int main() {
    const Vec3 eye{0,1.62,0};int scenarios=0;
    for(double distance : {2.,4.,8.,12.})for(double sideways : {-.28,0.,.28})
    for(double away : {-.2,0.,.2})for(double vertical : {-.25,0.,.42}) {
        TargetMotion target{{{-.3,0,distance-.3},{.3,1.8,distance+.3}},
            {sideways,vertical,away},vertical==0};
        auto solution=SolveIntercept(eye,target);
        assert(solution.valid && solution.ticks>0 && solution.ticks<=24);
        assert(PredictedHit(eye,solution.aim,target,clear));
        assert(SampleHit(eye,solution.aim,target));
        ++scenarios;
    }
    TargetMotion target{{{-.3,0,7.7},{.3,1.8,8.3}},{.28,0,0},true};
    auto solution=SolveIntercept(eye,target);
    assert(solution.valid && solution.aim.yaw< -5 && RobustHit(eye,solution.aim,target,0));
    assert(!PredictedHit(eye,{0,0},target,clear));
    assert(!RobustHit(eye,{solution.aim.yaw+15,solution.aim.pitch},target,0));
    assert(!PredictedHit(eye,solution.aim,target,
        [](Vec3,Vec3,bool& blocked,double& fraction){blocked=true;fraction=.1;return true;}));
    assert(!PredictedHit(eye,solution.aim,target,
        [](Vec3,Vec3,bool&,double&){return false;}));
    target.velocity={4,0,0};assert(!SolveIntercept(eye,target).valid);
    target.velocity={NAN,0,0};assert(!SolveIntercept(eye,target).valid);
    target.velocity={0,0,0};assert(!SolveIntercept({NAN,0,0},target).valid);
    MotionHistory history;
    history.Sample(1,1,{0,0,0},{.2,0,0},true);assert(!history.Ready());
    history.Sample(1,2,{.2,0,0},{.2,0,0},true);assert(history.Ready());
    history.Sample(1,2,{.2,0,0},{.2,0,0},true);assert(history.stable==2);
    history.Sample(1,3,{0,0,0},{-.2,0,0},true);assert(!history.Ready());
    history.Sample(1,4,{-.2,0,0},{-.2,0,0},true);assert(!history.Ready());
    history.Sample(1,5,{-.4,0,0},{-.2,0,0},true);assert(history.Ready());
    history.Sample(1,6,{20,0,0},{0,0,0},true);assert(!history.Ready());
    history.Sample(2,7,{20,0,0},{0,0,0},true);assert(!history.Ready());
    history.Sample(2,8,{20,0,0},{0,0,0},true);assert(history.Ready());
    history.Sample(2,30,{20,0,0},{0,0,0},true);assert(!history.Ready());
    assert(AdaptiveDelay(5,2,0)==5);
    assert(AdaptiveDelay(0,10,0)>AdaptiveDelay(0,2,0));
    assert(AdaptiveDelay(0,10,.15)>=AdaptiveDelay(0,10,0));
    std::cout<<"PASS: "<<scenarios<<" simulated intercept scenarios checked by independent flight sampling; walls, spread margin, motion reversals/teleports and adaptive delay\n";
}
