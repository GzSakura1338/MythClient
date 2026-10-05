#include "minecraft/modules/player/autocharge/Trajectory.h"
#include "minecraft/modules/player/autocharge/SlotHandoff.h"
#include "minecraft/modules/player/autocharge/ManualUseGate.h"
#include "minecraft/modules/player/autocharge/Prediction.h"
#include "minecraft/modules/player/autocharge/WindPlanner.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
using namespace nova::module::autocharge;
using nova::rotation::Rotation;
namespace nova::rotation {double Random(){return .5;}}
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
// Unreserved vanilla collision simulation, with independent wind spread. The
// planner sees the observed pearl velocity but cannot know the next RNG draw.
bool ActualHit(Pearl pearl,Vec3 eye,Vec3 inherited,Rotation angle,Vec3 spread,Vec3* destination=nullptr){
    Vec3 wind=eye,windVelocity=Add(Add(Look(angle).Scale(1.5),inherited),spread);
    for(int t=0;t<kFlightHorizon;++t){
        const Vec3 from=pearl.position;
        pearl.velocity={pearl.velocity.x*double(.99f),(pearl.velocity.y-.03)*double(.99f),pearl.velocity.z*double(.99f)};
        pearl.position=Add(from,pearl.velocity);
        ++pearl.age;
        const double r=.15625+double(std::clamp(float(pearl.age-2)/20.f,0.f,.3f));
        const Vec3 center=wind.Add(0,.15625-double(.15f),0);double fraction;
        if(SegmentBox(from,pearl.position,center.Add(-r,-r,-r),center.Add(r,r,r),fraction)&&
           Add(from,pearl.velocity.Scale(fraction)).y>eye.y+1){if(destination)*destination=from;return true;}
        wind=Add(wind,windVelocity);
    }
    return false;
}
// Reconstructed height-first objective for an A/B regression, not a frozen
// copy of the whole old module. Physics/candidates are shared; its old nominal
// 64-probe gate is kept while the new objective uses its stricter phase gate.
bool LegacyStable(Pearl p,Vec3 eye,Vec3 movement,Rotation a){
    int misses=0;HeightImpact hit;
    for(int i=0;i<64;++i)if(!Impact(p,eye,movement,a,WindSpreadProbes()[i],[](auto,auto){return true;},hit)&&++misses>2)return false;
    return true;
}
template<class Patch>
bool HeightOnlySolve(Pearl pearl,Vec3 eye,Vec3 inherited,double target,Patch patch,Rotation& angle,HeightImpact& out){
    double best=1e9;bool found=false;
    Candidates(pearl,eye,inherited,patch,[&](auto candidate){
        constexpr Rotation offsets[]{{0,0},{0,-.25f},{0,.25f},{0,-.5f},{0,.5f},{-.25f,0},{.25f,0},{-.5f,0},{.5f,0}};
        for(auto offset:offsets){
            auto adjusted=patch(Rotation{candidate.yaw+offset.yaw,candidate.pitch+offset.pitch});HeightImpact hit;
            if(!Impact(pearl,eye,inherited,adjusted,{},[](auto,auto){return true;},hit)||hit.contact.y<=eye.y+1||hit.clearance<.035)continue;
            const double score=std::abs(hit.destination.y-target)+.001*hit.ticks-.1*hit.clearance;
            if(score>=best||!LegacyStable(pearl,eye,inherited,adjusted))continue;
            best=score;angle=adjusted;out=hit;found=true;
        }
    });
    return found;
}
int main(){
    {
        CHECK(ReadyToLaunch({.3,0,0})&&ReadyToLaunch({0,-.1,0}));
        CHECK(!ReadyToLaunch({0,.01,0})&&!ReadyToLaunch({NAN,0,0}));
        const Rotation from{179,-80};const AimStep step(from,10,.5);
        const auto a=step({-170,-70}),repeat=step({-170,-70}),q=step.Quantize(a);
        CHECK(a.yaw==repeat.yaw&&a.pitch==repeat.pitch);
        CHECK(std::abs(a.yaw-q.yaw)<.0001&&std::abs(a.pitch-q.pitch)<.0001);
        CHECK(a.yaw>179&&a.yaw<190); // Continuous yaw across the seam.
        CHECK(std::hypot(a.yaw-from.yaw,a.pitch-from.pitch)<=10.2);
        const auto sane=AimStep({0,0},NAN,.5)({0,-90});CHECK(std::isfinite(sane.pitch));
        Pearl pearl{{0,1.52,0},{0,1.5,0}};for(int i=0;i<6;++i)Step(pearl);
        const auto blocked=PlanWind(pearl,{0,1.62,0},{},20,6,{0,-90},90,.5,true,[](auto,auto){return false;});
        CHECK(blocked.decision!=WindDecision::Fire);
        const auto invalid=PlanWind(pearl,{0,1.62,0},{},NAN,6,{0,-90},90,.5,true,[](auto,auto){return true;});
        CHECK(invalid.decision==WindDecision::NoSolution);
        MovementHistory h;h.Seed({},true);h.Move(true,{.3,0,0},true);h.EndTick();
        CHECK(!h.CanForecast());h.Move(true,{.6,0,0},true);h.EndTick();CHECK(h.CanForecast());
        h.EndTick();CHECK(!h.CanForecast());h.EndTick();CHECK(h.CanForecast());
        h.Move(true,{.6,.1,0},false);CHECK(!h.CanForecast());
        InterceptQuality quality;quality.misses=1;quality.worstPhaseMisses=2;CHECK(Reliable(quality));
        quality.worstPhaseMisses=3;CHECK(!Reliable(quality));quality.worstPhaseMisses=0;quality.misses=2;CHECK(!Reliable(quality));
        std::puts("PASS: deterministic bounded aim, ascent gate, forecast invalidation, obstruction and confidence gates");
    }
    {
        ManualUseGate gate;
        CHECK(!gate.Blocks());gate.Begin();CHECK(gate.Blocks());
        CHECK(!gate.BeginTick(true)&&gate.Blocks());
        // A failed use/Stop cannot unblock raw right-click until the next tick.
        bool working=false;
        CHECK(gate.Blocks());
        CHECK(gate.BeginTick(working)&&!gate.Blocks());
        CHECK(!gate.BeginTick(false));
        gate.Begin();CHECK(gate.Reset()&&!gate.Blocks());CHECK(!gate.Reset());
        std::puts("PASS: AutoCharge manual use is blocked through Stop until next tick; restore happens once");
    }
    MovementHistory history;Vec3 eye,motion;
    CHECK(!history.Launch(1.62,eye,motion));
    history.Seed({0,64,0},true);CHECK(!history.Launch(1.62,eye,motion));
    history.Move(true,{.3,64.42,0},false);history.EndTick();
    CHECK(history.Launch(1.62,eye,motion));CHECK(std::abs(motion.y-.42)<1e-9);
    CHECK(std::abs(eye.x-.3)<1e-9);
    history.Move(false,{},false);CHECK(history.Launch(1.62,eye,motion));CHECK(motion.DistanceToSqr({})==0);
    history.EndTick();history.Move(true,{.6,64.7532,0},false);history.EndTick();
    CHECK(history.Launch(1.62,eye,motion));CHECK(std::abs(motion.y-.3332)<1e-9);
    history.EndTick();CHECK(history.Launch(1.62,eye,motion));CHECK(motion.DistanceToSqr({})==0);
    history.Move(true,{100,90,0},false);CHECK(!history.Launch(1.62,eye,motion));
    std::puts("PASS: packet movement history handles jump, rotation-only, missing movement and corrections");
    for(bool cancel:{false,true}) {
        SlotHandoff slots;bool rightClicking=false;int held=0,releases=0;
        const auto release=[&]{CHECK(!rightClicking);held=0;++releases;};
        for(int attempt=0;attempt<2;++attempt){
            held=attempt+1;slots.BeforeUse();rightClicking=true;
            if(cancel)slots.Cleanup(release);
            CHECK(held==attempt+1&&releases==attempt);
            // Vanilla's tick-end packet clears Grim's per-tick action flags.
            rightClicking=false;slots.BeginTick(release);
            CHECK(held==0&&releases==attempt+1);
            slots.BeginTick(release);CHECK(releases==attempt+1);
        }
    }
    std::puts("PASS: pearl/wind slot restore follows tick boundary, including cancelled/failed submission");
    const auto clear=[](Vec3,Vec3){return true;};
    const auto quantize=[](Rotation a){return nova::rotation::ApplySensitivityPatch(a,{0,-90},.5);};
    std::mt19937 heightRandom(20260929);
    std::uniform_real_distribution<double> heightUnit(0,1);
    auto heightNoise=[&]{
        auto component=[&]{const double a=heightUnit(heightRandom),b=heightUnit(heightRandom);return (a-b)*.02584125;};
        return Vec3{component(),component(),component()};
    };
    int heightSamples=0,heightHits=0,heightInBand=0;
    auto checkHeightSamples=[&](Pearl pearl,Vec3 eye,Vec3 inherited,Rotation angle,double target){
        for(int sample=0;sample<256;++sample){
            ++heightSamples;Vec3 destination;
            if(ActualHit(pearl,eye,inherited,angle,heightNoise(),&destination)){
                ++heightHits;
                if(std::abs(destination.y-target)<=kHeightTolerance)++heightInBand;
            }
        }
    };
    // Height is relative to launch feet. EVERY unobstructed scenario must
    // complete the second throw, including unattainable target heights.
    int heightWindows=0,heightAttempts=0;double worstError=0;
    for(double base:{-40.,64.,200.})for(double speed:{0.,.28,.40})
    for(int yaw:{0,45,180})for(double height:{5.,10.,15.,20.,25.,28.,35.,45.}){
        ++heightAttempts;
        const auto movement=Look({float(yaw),0}).Scale(speed);
        Pearl pearl{{0,base+1.62-double(.1f),0},Add(Vec3{0,1.5,0},movement),0};
        Rotation actual=PrimeAngle(movement,float(yaw));
        bool thrown=false;
        for(int elapsed=1;elapsed<=kPearlMaxAge;++elapsed){
            Step(pearl);if(elapsed<2)continue;
            const auto launchEye=Add(Vec3{0,base+1.62,0},movement.Scale(elapsed));
            const auto plan=PlanWind(pearl,launchEye,movement,base+height,elapsed,actual,180,.5,true,clear);
            actual=plan.angle;auto impact=plan.impact;if(plan.decision!=WindDecision::Fire)continue;
            auto patch=[&](Rotation a){return AimStep(actual,180,.5).Quantize(a);};
            Vec3 destination;
            CHECK(ActualHit(pearl,launchEye,movement,actual,{},&destination));
            CHECK(destination.DistanceTo(impact.destination)<1e-8);
            checkHeightSamples(pearl,launchEye,movement,actual,base+height);
            const double error=std::abs(destination.y-base-height);
            worstError=std::max(worstError,error);
            CHECK(elapsed<=kHeightWaitDeadline);
            if(base==64 && speed==0 && yaw==0)
                std::printf("Height %.1f: automatic delay %d ticks, destination +%.3f, error %.3f\n",height,elapsed,destination.y-base,error);
            CHECK(!InterceptReady(pearl,launchEye,movement,actual,[](auto,auto){return false;},impact));
            CHECK(!WaitForHeight(pearl,launchEye,movement,base+height,kHeightWaitDeadline,impact,actual,patch));
            thrown=true;++heightWindows;break;
        }
        if(!thrown)std::printf("FAILED to throw: base %.0f speed %.2f yaw %d height %.1f\n",base,speed,yaw,height);
        CHECK(thrown);
    }
    std::printf("Height solver: %d/%d windows, max nominal error %.3f blocks\n",heightWindows,heightAttempts,worstError);
    CHECK(heightWindows==heightAttempts);
    CHECK(PearlApex({{0,1.52,0},{0,1.5,0},0})<30);
    // Closed-loop launch decisions under jumping, delayed observations and real
    // per-tick smoothing. Only the live quantized angle is allowed to fire.
    int jumpHeightWindows=0,jumpHeightAttempts=0;
    for(double speed:{0.,.30})for(int yaw:{0,45})for(int phase:{0,4,8})
    for(int lag:{0,3})for(double height:{20.,25.,30.,35.}){
        ++jumpHeightAttempts;
        double y=0,vy=.42,lastY=0;
        auto jumpStep=[&]{lastY=vy;y+=vy;vy=(vy-.08)*.98;if(y<=0){y=0;vy=0;lastY=0;}};
        for(int i=0;i<=phase;++i)jumpStep();
        while(!ReadyToLaunch({0,lastY,0}))jumpStep();
        const double base=y,target=base+height;
        const auto movement=Look({float(yaw),0}).Scale(speed);
        const Vec3 origin{0,y+1.62-double(.1f),0};
        Pearl truth{origin,Add(movement.Add(0,1.5+lastY,0),heightNoise()),0};
        std::array<Pearl,kPearlMaxAge+1> history{};history[0]=truth;
        Rotation actual=PrimeAngle(movement,float(yaw));
        for(int elapsed=1;elapsed<=kPearlMaxAge;++elapsed){
            Step(truth);history[elapsed]=truth;jumpStep();if(elapsed<2+lag)continue;
            Pearl aligned;double residual=0;
            CHECK(AlignPearl(history[elapsed-lag],origin,elapsed,aligned,residual));
            CHECK(aligned.position.DistanceTo(truth.position)<1e-7);
            const Vec3 eye=Add(Vec3{0,y+1.62,0},movement.Scale(elapsed));
            const Vec3 inherited=movement.Add(0,lastY,0);
            const auto plan=PlanWind(aligned,eye,inherited,target,elapsed,actual,30,.5,true,clear,y==0);
            actual=plan.angle;const auto impact=plan.impact;if(plan.decision!=WindDecision::Fire)continue;
            Vec3 destination;CHECK(ActualHit(truth,eye,inherited,actual,{},&destination));
            CHECK(destination.DistanceTo(impact.destination)<1e-7);
            checkHeightSamples(truth,eye,inherited,actual,target);
            ++jumpHeightWindows;break;
        }
        if(jumpHeightWindows!=jumpHeightAttempts)std::printf("FAILED jump: speed %.2f yaw %d phase %d lag %d height %.1f\n",speed,yaw,phase,lag,height);
        CHECK(jumpHeightWindows==jumpHeightAttempts);
    }
    std::printf("Height jump/sprint/smoothing/delayed observation: %d/%d safe windows\n",jumpHeightWindows,jumpHeightAttempts);
    CHECK(jumpHeightWindows==jumpHeightAttempts);
    std::printf("Independent height spread trials: %d samples, %d hits, %d within +/-%.2f blocks\n",heightSamples,heightHits,heightInBand,kHeightTolerance);
    CHECK(heightHits>heightSamples*.95);
    // User reproduction: Height 45 drives the old planner toward an
    // unattainable apex. Both planners see identical pearl spread, and are
    // evaluated with independent wind RNG plus a +/-1 tick alignment offset.
    int oldMisses=0,newMisses=0,oldPhaseMisses=0,newPhaseMisses=0,compareTrials=0;
    std::mt19937 compareRandom(0x290945);std::uniform_real_distribution<double> compareUnit(0.,1.);
    auto compareNoise=[&]{auto v=[&]{const double a=compareUnit(compareRandom),b=compareUnit(compareRandom);return (a-b)*kLaunchSpread;};return Vec3{v(),v(),v()};};
    for(double speed:{0.,.3})for(int yaw:{0,45})for(int sample=0;sample<12;++sample){
        const Vec3 movement=Look({float(yaw),0}).Scale(speed);
        const Pearl initial{{0,1.52,0},Add(movement.Add(0,1.5,0),compareNoise()),0};
        struct Shot {Pearl pearl;Vec3 eye;Rotation angle;bool sent=false;} shots[2];
        for(int mode=0;mode<2;++mode){
            Pearl pearl=initial;Rotation actual{float(yaw),-90};
            for(int elapsed=1;elapsed<=kPearlMaxAge;++elapsed){
                Step(pearl);if(elapsed<2)continue;
                const Vec3 eye=Add(Vec3{0,1.62,0},movement.Scale(elapsed));
                auto patch=[&](Rotation a){return nova::rotation::ApplySensitivityPatch(a,actual,.5);};
                Rotation desired;HeightImpact hit;
                const bool solved=mode?SolveHeight(pearl,eye,movement,45.,patch,desired,hit):HeightOnlySolve(pearl,eye,movement,45.,patch,desired,hit);
                if(!solved)continue;
                actual=patch(nova::rotation::LinearStep(actual,desired,180));
                if(mode){if(!InterceptReady(pearl,eye,movement,actual,clear,hit))continue;}
                else if(!Impact(pearl,eye,movement,actual,{},clear,hit)||hit.contact.y<=eye.y+1||hit.clearance<.035||!LegacyStable(pearl,eye,movement,actual))continue;
                bool wait=false;
                if(mode)wait=WaitForHeight(pearl,eye,movement,45.,elapsed,hit,actual,patch);
                else if(elapsed<kHeightWaitDeadline&&!HeightMatches(hit,45.)){
                    auto future=pearl;auto futureEye=eye;
                    for(int delay=1;delay<=std::min(8,kHeightWaitDeadline-elapsed);++delay){
                        Step(future);futureEye=Add(futureEye,movement);HeightImpact later;Rotation angle;
                        if(HeightOnlySolve(future,futureEye,movement,45.,patch,angle,later)&&
                           std::abs(later.destination.y-45.)+.15<std::abs(hit.destination.y-45.)){wait=true;break;}
                    }
                }
                if(wait)continue;
                shots[mode]={pearl,eye,actual,true};break;
            }
            CHECK(shots[mode].sent);
        }
        for(int trial=0;trial<1024;++trial){
            ++compareTrials;const auto noise=compareNoise();
            for(int mode=0;mode<2;++mode){
                const auto& shot=shots[mode];
                if(!ActualHit(shot.pearl,shot.eye,movement,shot.angle,noise))++(mode?newMisses:oldMisses);
                for(int phase:{-1,1}){
                    auto shifted=shot.pearl;
                    if(phase>0)Step(shifted);
                    else {shifted.position=shifted.position.Subtract(shifted.velocity);shifted.velocity=shifted.velocity.Scale(1./kPearlDrag).Add(0,kPearlGravity,0);--shifted.age;}
                    if(!ActualHit(shifted,shot.eye,movement,shot.angle,noise))++(mode?newPhaseMisses:oldPhaseMisses);
                }
            }
        }
    }
    std::printf("Height 45 A/B: %d independent samples, old/new misses %d/%d; +/-1 tick misses %d/%d of %d\n",compareTrials,oldMisses,newMisses,oldPhaseMisses,newPhaseMisses,compareTrials*2);
    CHECK(newMisses<oldMisses);CHECK(newPhaseMisses<oldPhaseMisses);
    Pearl step{{1,2,3},{.3,1.5,.2},0};Step(step);
    CHECK(std::abs(step.position.y-(2+1.47*double(.99f)))<1e-9);
    CHECK(step.age==1&&step.velocity.x<.3);
    int solved=0,compensated=0;
    for(double speed:{0.,.28,.40})for(int yaw=0;yaw<360;yaw+=45)for(int delay:{2,4,6,8}) {
        const auto movement=Look({float(yaw),0}).Scale(speed);
        Pearl p{{0,1.52,0},Add(Vec3{0,1.5,0},movement),0};
        for(int i=0;i<delay;++i)Step(p);
        const Vec3 eye=Add(Vec3{0,1.62,0},movement.Scale(delay));
        Solution solution;
        const bool found=Solve(p,eye,movement,quantize,clear,solution);
        if(!found){std::fprintf(stderr,"no solution speed %.2f yaw %d delay %d\n",speed,yaw,delay);}
        CHECK(found);++solved;
        CHECK(Hits(p,eye,movement,solution.angle,clear,solution));
        CHECK(solution.hit.y>eye.y+1&&solution.ticks>0);
        CHECK(!Hits(p,eye,movement,solution.angle,[](auto,auto){return false;},solution));
        if(speed>0&&!Hits(p,eye,movement,{float(yaw),-90},clear,solution))++compensated;
    }
    CHECK(compensated>0);
    Solution out;
    CHECK(!Solve({{0,2,0},{0,1,0},2},{NAN,0,0},{},quantize,clear,out));
    CHECK(!Hits({{0,2,0},{0,1,0},2},{},{},{0,0},clear,out));
    CHECK(!Solve({{100,2,100},{0,1,0},2},{},{},quantize,clear,out));
    std::printf("PASS: %d standing/cardinal/diagonal sprint interceptions; %d require XZ compensation; blockers and invalid input\n",solved,compensated);

    std::mt19937 random(20260928);std::uniform_real_distribution<double> unit(0,1);
    const auto spread=[&]{return Vec3{(unit(random)-unit(random))*.02584125,
        (unit(random)-unit(random))*.02584125,(unit(random)-unit(random))*.02584125};};
    int correctHits=0,wrongHits=0,trials=0,windows=0;
    for(double speed:{.28,.4})for(int yaw=0;yaw<360;yaw+=45)
    for(int launchPhase:{0,3,6,9})for(int delay:{2,4,6,8}) {
        double y=0,vy=.42,lastY=0;
        auto jumpStep=[&]{lastY=vy;y+=vy;vy=(vy-.08)*.98;if(y<=0){y=0;vy=0;lastY=0;}};
        for(int t=0;t<=launchPhase;++t)jumpStep();
        auto horizontal=Look({float(yaw),0}).Scale(speed);
        Pearl p{{0,y+1.52,0},Add(Add(Vec3{0,1.5+lastY,0},horizontal),spread()),0};
        for(int t=0;t<delay;++t){Step(p);jumpStep();}
        const auto eye=Add(Vec3{0,y+1.62,0},horizontal.Scale(delay));
        const auto inherited=horizontal.Add(0,lastY,0);
        const auto wrong=horizontal.Scale(y>0?.91:.546).Add(0,y>0?vy:0,0);
        Solution good,bad;
        if(!Solve(p,eye,inherited,quantize,clear,good)||good.clearance<.035)continue;
        ++windows;const bool oldFound=Solve(p,eye,wrong,quantize,clear,bad);
        for(int sample=0;sample<64;++sample){const auto noise=spread();++trials;
            correctHits+=ActualHit(p,eye,inherited,good.angle,noise);
            wrongHits+=oldFound&&ActualHit(p,eye,inherited,bad.angle,noise);
        }
    }
    std::printf("Jump/sprint spread regression: %d windows, %d trials; displacement %d hits, post-drag velocity %d hits\n",windows,trials,correctHits,wrongHits);
    CHECK(windows>100);CHECK(correctHits>wrongHits);CHECK(correctHits>trials*.90);

    int delayedHits=0,staleHits=0,delayedTrials=0,alignedCases=0;
    for(double speed:{.28,.4})for(int yaw=0;yaw<360;yaw+=45)
    for(int elapsed:{6,10,14})for(int lag:{0,2,4}){
        const auto horizontal=Look({float(yaw),0}).Scale(speed);
        const Vec3 origin{0,1.52,0};
        Pearl initial{origin,Add(Add(Vec3{0,1.5,0},horizontal),spread()),0},truth=initial,observed=initial;
        for(int i=0;i<elapsed;++i)Step(truth);
        for(int i=0;i<elapsed-lag;++i)Step(observed);
        observed.age=1; // Delayed spawn age must not become physical flight age.
        Pearl aligned;double residual=0;
        CHECK(AlignPearl(observed,origin,elapsed,aligned,residual));
        CHECK(aligned.position.DistanceTo(truth.position)<1e-8);
        CHECK(aligned.velocity.DistanceTo(truth.velocity)<1e-8);++alignedCases;
        auto noisy=observed;noisy.position=noisy.position.Add(.004,-.004,.004);
        noisy.velocity=noisy.velocity.Add(.0001,-.0001,.0001);
        Pearl estimated;
        CHECK(AlignPearl(noisy,origin,elapsed,estimated,residual));
        CHECK(estimated.position.DistanceTo(truth.position)<.03);
        const auto launchEye=Add(Vec3{0,1.62,0},horizontal.Scale(elapsed));
        Solution good,stale;
        if(!Solve(aligned,launchEye,horizontal,quantize,clear,good)||good.clearance<.035)continue;
        const bool staleFound=Solve(observed,launchEye,horizontal,quantize,clear,stale);
        for(int i=0;i<48;++i){const auto noise=spread();++delayedTrials;
            delayedHits+=ActualHit(truth,launchEye,horizontal,good.angle,noise);
            staleHits+=staleFound&&ActualHit(truth,launchEye,horizontal,stale.angle,noise);
        }
        auto corrupted=observed;corrupted.position.x+=2;
        CHECK(!AlignPearl(corrupted,origin,elapsed,aligned,residual));
    }
    std::printf("Delayed snapshot regression: %d aligned cases, %d trials; aligned %d hits, stale %d hits\n",alignedCases,delayedTrials,delayedHits,staleHits);
    CHECK(alignedCases==144);CHECK(delayedTrials>1000);CHECK(delayedHits>delayedTrials*.90);CHECK(delayedHits>staleHits);

    // Jumping at pearl launch can make the pearl temporarily faster than a
    // later grounded wind charge. Wait for a reachable window, including descent.
    Pearl rising{{0,1.52,0},{.3,1.92,0},0};bool laterWindow=false;
    for(int elapsed=1;elapsed<=50;++elapsed){
        Step(rising);if(elapsed<4)continue;
        Solution solution;const Vec3 eye{.3*elapsed,1.62,0};
        const bool found=Solve(rising,eye,{.3,0,0},quantize,clear,solution);
        if(found&&solution.clearance>=.035){laterWindow=true;break;}
    }
    CHECK(laterWindow);
}
