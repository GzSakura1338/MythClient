#include "minecraft/modules/player/autocharge/Prediction.h"
#include "minecraft/modules/player/autocharge/WindPlanner.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include <cstdio>
#include <cstdlib>
#include <random>
using namespace nova::module::autocharge;
using nova::rotation::Rotation;
namespace nova::rotation {double Random(){return .5;}}
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)

// Separate forward integration, full vanilla boxes, no planning safety margin.
static bool Hit(Pearl pearl,Vec3 eye,Vec3 inherited,Rotation angle,Vec3 noise,int phase){
    if(phase>0)Step(pearl);
    if(phase<0){pearl.position=pearl.position.Subtract(pearl.velocity);pearl.velocity=pearl.velocity.Scale(1./kPearlDrag).Add(0,.03,0);--pearl.age;}
    Vec3 wind=eye,velocity=Add(Add(Look(angle).Scale(1.5),inherited),noise);
    for(int t=0;t<32;++t){
        const auto previous=pearl.position;
        pearl.velocity={pearl.velocity.x*double(.99f),(pearl.velocity.y-.03)*double(.99f),pearl.velocity.z*double(.99f)};
        pearl.position=Add(pearl.position,pearl.velocity);++pearl.age;
        const double radius=.15625+double(std::clamp(float(pearl.age-2)/20.f,0.f,.3f));
        const Vec3 center=wind.Add(0,.15625-double(.15f),0);double fraction=0;
        if(SegmentBox(previous,pearl.position,center.Add(-radius,-radius,-radius),center.Add(radius,radius,radius),fraction))return true;
        wind=Add(wind,velocity);
    }
    return false;
}
int main(){
    const char* names[]{"stationary","walking","sprinting","diagonal","turning","stopping","accelerating"};
    for(const auto validationSeed:{2026092920u,0x90e020u}){
    std::printf("Independent validation seed %u\n",validationSeed);
    std::mt19937 random(validationSeed);std::uniform_real_distribution<double> uniform(0.,1.);
    auto noise=[&]{auto v=[&]{double a=uniform(random),b=uniform(random);return (a-b)*kLaunchSpread;};return Vec3{v(),v(),v()};};
    for(int mode=0;mode<7;++mode){
        int shots=0,total=0,hits[3]{},maxDelay=0;double worstError=0;
        for(double sensitivity:{.2,.5,.8})for(int yaw:{0,45,135,270})for(int seed=0;seed<4;++seed){
            const double speed=mode==0||mode==6?0:mode==1?.215:.28;
            Vec3 inherited=Look({float(yaw),0}).Scale(speed),feet{};
            const Vec3 origin{0,1.62-double(.1f),0};
            Pearl truth{origin,Add(inherited.Add(0,1.5,0),noise()),0};
            std::array<Pearl,kPearlMaxAge+1> history{};history[0]=truth;
            Rotation angle=PrimeAngle(inherited,float(yaw));bool sent=false;
            for(int elapsed=1;elapsed<=kPearlMaxAge;++elapsed){
                Step(truth);history[elapsed]=truth;
                if(mode==4)inherited=Look({float(yaw+std::min(elapsed,5)*12),0}).Scale(.28);
                if(mode==5&&elapsed>=3)inherited=inherited.Scale(.546);
                if(mode==6)inherited=Look({float(yaw),0}).Scale(std::min(.28,.075*elapsed));
                feet=Add(feet,inherited);if(elapsed<4)continue;
                auto observed=history[elapsed-2];
                // Client velocity packets quantize each axis to 1/8000.
                observed.velocity={std::round(observed.velocity.x*8000)/8000,std::round(observed.velocity.y*8000)/8000,std::round(observed.velocity.z*8000)/8000};
                Pearl aligned;double residual=0;if(!AlignPearl(observed,origin,elapsed,aligned,residual))continue;
                const auto eye=feet.Add(0,1.62,0);
                const bool steady=mode<4||(mode==4&&elapsed>5)||(mode==5&&elapsed>8)||(mode==6&&elapsed>4);
                const auto plan=PlanWind(aligned,eye,inherited,20.,elapsed,angle,90,sensitivity,true,[](auto,auto){return true;},steady);
                angle=plan.angle;const auto impact=plan.impact;if(plan.decision!=WindDecision::Fire)continue;
                ++shots;maxDelay=std::max(maxDelay,elapsed);worstError=std::max(worstError,std::abs(impact.destination.y-20.));
                for(int trial=0;trial<1024;++trial){const auto spread=noise();++total;for(int phase=-1;phase<=1;++phase)hits[phase+1]+=Hit(truth,eye,inherited,angle,spread,phase);}
                sent=true;break;
            }
            CHECK(sent);
        }
        std::printf("20 blocks %-12s shots=%d/48 maxDelay=%d error<=%.3f nominal=%.3f%% phase-1=%.3f%% phase+1=%.3f%% (%d trials each)\n",
            names[mode],shots,maxDelay,worstError,100.*hits[1]/total,100.*hits[0]/total,100.*hits[2]/total,total);
        CHECK(shots==48);
        for(const int successes:hits){
            // 95% Wilson lower confidence bound must itself exceed 95%, for
            // EACH movement and timing bucket, not just their pooled average.
            const double n=total,p=double(successes)/n,z=1.96,z2=z*z;
            const double lower=(p+z2/(2*n)-z*std::sqrt((p*(1-p)+z2/(4*n))/n))/(1+z2/n);
            CHECK(lower>.95);
        }
    }
    }
}
