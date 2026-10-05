#include "minecraft/modules/player/autocharge/Prediction.h"
#include "minecraft/modules/player/autocharge/WindPlanner.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include <chrono>
#include <cstdio>
#include <random>
#include <vector>
using namespace nova::module::autocharge;
using nova::rotation::Rotation;
namespace nova::rotation {double Random(){return .5;}}
// Independent collision oracle: the planner's Impact / SegmentBox is not used.
bool Actual(Pearl p,Vec3 eye,Vec3 inherited,Rotation angle,Vec3 spread,int phase){
    if(phase>0){p.velocity={p.velocity.x*double(.99f),(p.velocity.y-.03)*double(.99f),p.velocity.z*double(.99f)};p.position=Add(p.position,p.velocity);++p.age;}
    if(phase<0){p.position=p.position.Subtract(p.velocity);p.velocity=p.velocity.Scale(1./double(.99f)).Add(0,.03,0);--p.age;}
    const double radians=3.14159265358979323846/180;
    Vec3 v{-std::sin(angle.yaw*radians)*std::cos(angle.pitch*radians)*1.5,
           -std::sin(angle.pitch*radians)*1.5,std::cos(angle.yaw*radians)*std::cos(angle.pitch*radians)*1.5};
    v=Add(Add(v,inherited),spread);
    for(int tick=0;tick<32;++tick){
        const auto from=p.position;
        p.velocity={p.velocity.x*double(.99f),(p.velocity.y-.03)*double(.99f),p.velocity.z*double(.99f)};
        p.position=Add(p.position,p.velocity);++p.age;
        const double r=.15625+double(std::clamp(float(p.age-2)/20.f,0.f,.3f));
        const double a[]{from.x,from.y,from.z},d[]{p.velocity.x,p.velocity.y,p.velocity.z};
        const double center[]{eye.x,eye.y+.15625-double(.15f),eye.z};
        double enter=0,leave=1;bool hit=true;
        for(int axis=0;axis<3;++axis){
            if(std::abs(d[axis])<1e-12){if(std::abs(a[axis]-center[axis])>r)hit=false;}
            else{double lo=(center[axis]-r-a[axis])/d[axis],hi=(center[axis]+r-a[axis])/d[axis];
                if(lo>hi)std::swap(lo,hi);enter=std::max(enter,lo);leave=std::min(leave,hi);}
        }
        if(hit&&enter<=leave)return true;
        eye=Add(eye,v);
    }
    return false;
}
struct Jump {
    double y=0,vy=.42,displacement=0;
    void Step(){const double old=y;y+=vy;vy=(vy-.08)*.98;if(y<=0){y=0;vy=0;}displacement=y==0?0:y-old;}
};
int main(){
    int all=0,noShot=0,unexpectedNoShot=0;double worstCase=1;std::vector<double> times;
    for(double height:{20.,25.,45.})for(int phase:{0,4,8}){
        int shots=0,attempts=0,total=0,hits[3]{};double maxError=0;
        for(double sensitivity:{.2,.8})for(int yaw:{0,135})for(int lag:{0,3,8})for(int seed=0;seed<2;++seed){
            ++attempts;++all;Jump player;for(int t=0;t<=phase;++t)player.Step();
            // Keep each case's pearl and validation samples independent of
            // whether an earlier case fired or cancelled.
            std::mt19937 rng(0x73581924u+std::uint32_t(all)*7919u);std::uniform_real_distribution<double> unit(0,1);
            auto noise=[&]{auto axis=[&]{double a=unit(rng),b=unit(rng);return (a-b)*.02584125;};return Vec3{axis(),axis(),axis()};};
            int priming=0;while(!ReadyToLaunch({0,player.displacement,0})&&++priming<20)player.Step();
            const double base=player.y;Vec3 horizontal=Look({float(yaw),0}).Scale(.3),feet{0,base,0};
            const Vec3 origin=feet.Add(0,1.62-double(.1f),0);
            Pearl pearl{origin,Add(horizontal.Add(0,1.5+player.displacement,0),noise()),0};
            std::array<Pearl,kPearlMaxAge+1> history;history[0]=pearl;
            Rotation angle=PrimeAngle(horizontal,float(yaw));bool sent=false;
            for(int elapsed=1;elapsed<=kPearlMaxAge;++elapsed){
                Step(pearl);history[elapsed]=pearl;player.Step();
                if(seed)horizontal=Look({float(yaw+std::min(elapsed,8)*9),0}).Scale(.3);
                feet=Add(feet,horizontal);feet.y=player.y;
                if(elapsed<lag+2)continue;
                auto observed=history[elapsed-lag];
                observed.velocity={std::round(observed.velocity.x*8000)/8000,std::round(observed.velocity.y*8000)/8000,std::round(observed.velocity.z*8000)/8000};
                Pearl aligned;double residual=0;if(!AlignPearl(observed,origin,elapsed,aligned,residual))continue;
                const auto eye=feet.Add(0,1.62,0),inherited=horizontal.Add(0,player.displacement,0);
                auto start=std::chrono::steady_clock::now();
                const auto plan=PlanWind(aligned,eye,inherited,base+height,elapsed,angle,90,sensitivity,true,[](auto,auto){return true;},player.y==0&&(!seed||elapsed>8));
                angle=plan.angle;const auto impact=plan.impact;bool ready=plan.decision==WindDecision::Fire;
                times.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
                if(!ready)continue;
                ++shots;maxError=std::max(maxError,std::abs(impact.destination.y-base-height));
                int caseHits[3]{};
                for(int i=0;i<512;++i){auto n=noise();++total;for(int p=-1;p<=1;++p){bool h=Actual(pearl,eye,inherited,angle,n,p);hits[p+1]+=h;caseHits[p+1]+=h;}}
                for(int h:caseHits)worstCase=std::min(worstCase,double(h)/512);
                sent=true;break;
            }
            if(!sent){++noShot;if(lag!=8)++unexpectedNoShot;std::printf("NO SHOT height=%.0f phase=%d sensitivity=%.1f yaw=%d lag=%d seed=%d\n",height,phase,sensitivity,yaw,lag,seed);}
        }
        std::printf("Air height=%.0f jumpPhase=%d shots=%d/%d nominal=%.3f%% phase-1=%.3f%% phase+1=%.3f%% maxHeightError=%.3f\n",
            height,phase,shots,attempts,100.*hits[1]/total,100.*hits[0]/total,100.*hits[2]/total,maxError);
    }
    std::sort(times.begin(),times.end());
    std::printf("SUMMARY scenarios=%d noShot=%d worstIndividual=%.3f%% plannerP95=%.3fms max=%.3fms\n",all,noShot,worstCase*100,times[times.size()*95/100],times.back());
    // At 400ms observation delay a reliable window may already be gone. Count
    // cancellations explicitly; every ordinary (0/3 tick) case must still fire.
    return unexpectedNoShot||worstCase<.95?1:0;
}
