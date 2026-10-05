#pragma once
#include "minecraft/utils/rotation/Vec3.h"
#include "minecraft/utils/rotation/Rotation.h"
#include "minecraft/utils/rotation/RotationUtils.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <bit>
#include <cstdint>
#include <unordered_set>

namespace nova::module::autocharge {
using rotation::Vec3;
inline constexpr int kFlightHorizon=32;
// Verified Lunar 1.21.11 vanilla values. Keep float promotions exact.
inline constexpr double kPearlGravity=.03, kPearlDrag=double(.99f);
inline constexpr double kLaunchSpeed=1.5, kLaunchSpread=.0172275*kLaunchSpeed;
inline constexpr double kPearlEyeOffset=double(.1f);
inline constexpr double kWindHalf=.3125/2., kWindCenterY=kWindHalf-double(.15f);
inline constexpr int kPearlMaxAge=110;
struct Pearl { Vec3 position, velocity; int age=0; };
struct Solution { rotation::Rotation angle; double ticks=0; Vec3 hit; double clearance=0; };
struct HeightImpact { Vec3 destination{},contact{}; int ticks=0; double clearance=0; };
inline constexpr float kWindMaxPitch=-5.f;
inline bool Finite(Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
inline Vec3 Add(Vec3 a,Vec3 b){return a.Add(b.x,b.y,b.z);}
inline Vec3 Look(rotation::Rotation a){
    constexpr double rad=3.14159265358979323846/180.;
    return {-std::sin(a.yaw*rad)*std::cos(a.pitch*rad),-std::sin(a.pitch*rad),std::cos(a.yaw*rad)*std::cos(a.pitch*rad)};
}
inline Vec3 Step(Pearl& p){
    // Lunar 1.21.11 / 26.3 ThrowableProjectile: gravity, inertia, then movement.
    p.velocity=p.velocity.Add(0,-kPearlGravity,0).Scale(kPearlDrag);
    p.position=Add(p.position,p.velocity);++p.age;return p.position;
}
inline bool SegmentBox(Vec3 a,Vec3 b,Vec3 lo,Vec3 hi,double& fraction){
    double near=0,far=1;
    const double from[]{a.x,a.y,a.z},to[]{b.x,b.y,b.z},min[]{lo.x,lo.y,lo.z},max[]{hi.x,hi.y,hi.z};
    for(int axis=0;axis<3;++axis){
        const double d=to[axis]-from[axis];
        if(std::abs(d)<1e-10){if(from[axis]<min[axis]||from[axis]>max[axis])return false;continue;}
        double l=(min[axis]-from[axis])/d,h=(max[axis]-from[axis])/d;
        if(l>h)std::swap(l,h);near=std::max(near,l);far=std::min(far,h);
        if(near>far)return false;
    }
    fraction=near;return true;
}
// Cache the pearl path once per search. Tracking and firing agree on the first
// collision, including tick-count margin and vanilla old-position destination.
struct PearlPath {
    std::array<Vec3,kFlightHorizon+1> points{};
    std::array<double,kFlightHorizon> radii{};
    bool valid=false;
    explicit PearlPath(Pearl pearl){
        valid=Finite(pearl.position)&&Finite(pearl.velocity)&&pearl.age>=0;
        if(!valid)return;
        points[0]=pearl.position;
        for(int t=0;t<kFlightHorizon;++t){points[t+1]=Step(pearl);
            radii[t]=kWindHalf+double(std::clamp(float(pearl.age-2)/20.f,0.f,.3f));}
    }
};
template<class Clear>
bool Impact(const PearlPath& path,Vec3 eye,Vec3 inherited,rotation::Rotation angle,
            Vec3 spread,Clear&& clear,HeightImpact& out){
    if(!path.valid||!Finite(eye)||!Finite(inherited)||!Finite(spread)||
       !std::isfinite(angle.yaw)||!std::isfinite(angle.pitch)||angle.pitch>kWindMaxPitch||angle.pitch< -90)return false;
    const auto velocity=Add(Add(Look(angle).Scale(kLaunchSpeed),inherited),spread);
    Vec3 wind=eye;
    for(int t=0;t<kFlightHorizon;++t){
        const auto start=path.points[t],end=path.points[t+1];const double radius=path.radii[t];
        const auto center=wind.Add(0,kWindCenterY,0);double fraction=0;
        if(SegmentBox(start,end,center.Add(-radius,-radius,-radius),center.Add(radius,radius,radius),fraction)){
            const auto delta=end.Subtract(start),offset=center.Subtract(start);
            const auto contact=Add(start,delta.Scale(fraction));
            if(!clear(start,contact))return false;
            const double length2=delta.DistanceToSqr({});
            const double closest=length2>1e-12?std::clamp(
                (offset.x*delta.x+offset.y*delta.y+offset.z*delta.z)/length2,0.,1.):0.;
            const auto error=Add(start,delta.Scale(closest)).Subtract(center);
            out={start,contact,t+1,radius-std::max({std::abs(error.x),std::abs(error.y),std::abs(error.z)})};
            return true;
        }
        if(!clear(start,end))return false;
        const auto next=Add(wind,velocity);if(!clear(wind,next))return false;wind=next;
    }
    return false;
}
template<class Clear>
bool Impact(Pearl pearl,Vec3 eye,Vec3 inherited,rotation::Rotation angle,
            Vec3 spread,Clear&& clear,HeightImpact& out){
    return Impact(PearlPath(pearl),eye,inherited,angle,spread,clear,out);
}
template<class Clear>
bool Hits(Pearl pearl,Vec3 eye,Vec3 inherited,rotation::Rotation angle,Clear&& clear,Solution& out){
    HeightImpact hit;
    if(!Impact(pearl,eye,inherited,angle,{},clear,hit)||hit.contact.y<=eye.y+1)return false;
    out={angle,double(hit.ticks),hit.contact,hit.clearance};return true;
}
// Enumerate physically reachable angles independently of the selection goal.
// Height control and the original interception helper share this geometry.
template<class Quantize,class Candidate>
void Candidates(Pearl pearl,Vec3 eye,Vec3 inherited,Quantize&& quantize,Candidate&& candidate){
    if(!Finite(pearl.position)||!Finite(pearl.velocity)||!Finite(eye)||!Finite(inherited))return;
    Pearl predicted=pearl;
    for(int tick=1;tick<=kFlightHorizon;++tick){
        auto from=predicted.position;auto to=Step(predicted);
        if(tick<2)continue;
        // Intersect the pearl's swept segment with the sphere of positions a
        // speed-1.5 wind charge can reach after (tick-1) updates. Solve the
        // launch-speed equation rather than normalizing an arbitrary sampled
        // direction whose required speed only happened to be near 1.5.
        const double time=tick-1;
        const auto a=from.Add(0,-kWindCenterY,0).Subtract(eye).Scale(1./time).Subtract(inherited);
        const auto b=to.Subtract(from).Scale(1./time);
        const double aa=b.DistanceToSqr({}),bb=2*(a.x*b.x+a.y*b.y+a.z*b.z),cc=a.DistanceToSqr({})-2.25;
        std::array<double,5> fractions{0.,1.,.5,-1.,-1.};
        if(aa>1e-12){
            fractions[2]=std::clamp(-bb/(2*aa),0.,1.);
            const double discriminant=bb*bb-4*aa*cc;
            if(discriminant>=0){const double root=std::sqrt(discriminant);
                fractions[3]=(-bb-root)/(2*aa);fractions[4]=(-bb+root)/(2*aa);
            }
        }
        for(double fraction:fractions){
            if(fraction<0||fraction>1)continue;
            const auto point=Add(from,to.Subtract(from).Scale(fraction)).Add(0,-kWindCenterY,0);
            const auto required=point.Subtract(eye).Scale(1./(tick-1)).Subtract(inherited);
            const double length=required.DistanceTo({});
            // Boundary candidates are useful only within the wind hitbox;
            // exact interior roots already have the required launch speed.
            if(std::abs(length-kLaunchSpeed)*time>kWindHalf+.3)continue;
            auto angle=quantize(rotation::Calculate({},required));
            candidate(angle);
        }
    }
}
template<class Quantize,class Clear>
bool Solve(Pearl pearl,Vec3 eye,Vec3 inherited,Quantize&& quantize,Clear&& clear,Solution& out){
    bool found=false;double best=-1e9;
    Candidates(pearl,eye,inherited,quantize,[&](auto angle){
      for(const float pitchOffset:{0.f,-.25f,.25f,-.5f,.5f}) {
        Solution candidate;auto adjusted=quantize(rotation::Rotation{angle.yaw,angle.pitch+pitchOffset});
        if(Hits(pearl,eye,inherited,adjusted,clear,candidate)){
            const double score=candidate.clearance-.004*candidate.ticks;
            if(score>best){best=score;out=candidate;found=true;}
        }
      }
    });
    return found;
}
}
