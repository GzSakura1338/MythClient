#pragma once
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_crystal.h"
#include "minecraft/utils/rotation/RotationUtils.h"
#include "minecraft/utils/rotation/RayCastUtil.h"
#include <algorithm>
#include <cmath>
namespace nova::module::explosive {
inline bool SameBlock(const java::BlockPos& a,const java::BlockPos& b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
inline rotation::Vec3 Center(const java::BlockPos& p){return {p.x+.5,p.y+.5,p.z+.5};}
inline bool BlockHit(const java::EntityHandle& self,const java::BlockPos& block,rotation::Rotation angle,double reach,bool walls,java::BlockHitResultInfo& hit){
    rotation::Vec3 eye,d;if(!self.EyePosition(1,eye)||!java::Vec3DirectionFromRotation(angle.pitch,angle.yaw,d))return false;
    if(!walls)return java::WorldClip(eye,eye.Add(d.x*reach,d.y*reach,d.z*reach),self,hit)&&hit.kind==java::HitKind::Block&&hit.hasBlockPos&&SameBlock(hit.blockPos,block)&&rotation::InteractionRayCast(angle,reach).type==rotation::HitResultType::Block;
    const double p[]{eye.x,eye.y,eye.z},v[]{d.x,d.y,d.z},lo[]{double(block.x),double(block.y),double(block.z)};
    double enter=0,leave=reach;int side=0;
    for(int i=0;i<3;++i){
        if(std::abs(v[i])<1e-12){if(p[i]<lo[i]||p[i]>lo[i]+1)return false;continue;}
        double a=(lo[i]-p[i])/v[i],b=(lo[i]+1-p[i])/v[i];int face=i==0?4:i==1?0:2;
        if(a>b){std::swap(a,b);++face;}if(a>=enter){enter=a;side=face;}
        leave=std::min(leave,b);if(enter>leave)return false;
    }
    hit={};hit.kind=java::HitKind::Block;hit.hasBlockPos=true;hit.blockPos=block;hit.direction=side;
    hit.location=eye.Add(d.x*enter,d.y*enter,d.z*enter);return true;
}
inline bool AimBlock(const java::EntityHandle& self,const java::BlockPos& p,double reach,bool walls,
                     rotation::Rotation current,rotation::Rotation& out,java::BlockHitResultInfo& hit,int face=-1,bool changePoint=false){
    if(!changePoint&&BlockHit(self,p,current,reach,walls,hit)&&(face<0||hit.direction==face)){out=current;return true;}
    rotation::Vec3 eye;if(!self.EyePosition(1,eye))return false;
    const rotation::Vec3 normals[]={{0,-.5,0},{0,.5,0},{0,0,-.5},{0,0,.5},{-.5,0,0},{.5,0,0}};
    for(int i=0;i<6;++i){if(face>=0&&face!=i)continue;
        const auto n=normals[i],c=Center(p).Add(n.x,n.y,n.z);
        const auto u=n.x?rotation::Vec3{0,.3,0}:rotation::Vec3{.3,0,0};
        const auto v=n.z?rotation::Vec3{0,.3,0}:rotation::Vec3{0,0,.3};
        for(auto o:{rotation::Vec3{},u,u.Scale(-1),v,v.Scale(-1)}){
            auto a=rotation::Calculate(eye,c.Add(o.x,o.y,o.z));
            if(changePoint&&std::abs(std::remainder(double(a.yaw-current.yaw),360.))<.0001&&std::abs(a.pitch-current.pitch)<.0001)continue;
            if(BlockHit(self,p,a,reach,walls,hit)&&(face<0||hit.direction==face)){out=a;return true;}
        }
    }return false;
}
inline bool AimPlacement(const java::EntityHandle& self,const java::BlockPos& p,double reach,bool walls,
                         rotation::Rotation current,rotation::Rotation& out,java::BlockHitResultInfo& hit,bool changePoint=false){
    const java::BlockPos steps[]={{0,1,0},{1,0,0},{-1,0,0},{0,0,1},{0,0,-1},{0,-1,0}};
    const int faces[]={1,5,4,3,2,0};
    for(int i=0;i<6;++i){auto d=steps[i];java::BlockPos b{p.x-d.x,p.y-d.y,p.z-d.z};
        if(java::CrystalBaseSupport(b)&&AimBlock(self,b,reach,walls,current,out,hit,faces[i],changePoint))return true;}
    return false;
}
}
