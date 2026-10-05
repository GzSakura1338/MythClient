#pragma once
#include "minecraft/java/api/game_charge.h"
#include "minecraft/modules/player/autocharge/Trajectory.h"
#include "core/RuntimeControl.h"
#include <unordered_set>

namespace nova::module::autocharge {
// Read-only follow-up after releasing rotation/slot ownership. A successful
// use call is NOT evidence that the server spawned the wind or caught a pearl.
// Never consumes another item or uses these observations to steer the player.
class FlightObservation {
public:
    void Reset(){active_=false;self_.Reset();world_.Reset();oldWinds_.clear();}
    void Begin(const java::EntityHandle& self,const java::EntityHandle& world,int pearl,
               Vec3 windOrigin,Vec3 expectedVelocity,Vec3 destination){
        if(active_)LogTo("AutoCharge: observation superseded before result");
        Reset();self_=java::PlayerEntity();world_=java::LevelEntity();
        if(!self_||!world_||!self_.IsSame(self)||!world_.IsSame(world)){Reset();return;}
        pearl_=pearl;wind_=-1;age_=0;gone_=-1;
        origin_=windOrigin;expected_=expectedVelocity;destination_=destination;
        teleported_=false;self_.Position(previous_);lastPearl_=destination;
        // Called immediately before submission; ignore previously existing winds.
        std::vector<java::EntityHandle> entities;
        if(java::WorldEntities(self_,{origin_.x-40,origin_.y-10,origin_.z-40,origin_.x+40,origin_.y+60,origin_.z+40},entities))
            for(const auto& entity:entities){Vec3 v;int id;if(java::ChargeWind(entity,self_,v)&&java::EntityId(entity,id))oldWinds_.insert(id);}
        active_=true;
    }
    void Tick(){
        if(!active_)return;
        const auto self=java::PlayerEntity(),world=java::LevelEntity();
        if(!self||!world||!self.IsSame(self_)||!world.IsSame(world_)){LogTo("AutoCharge: observation ended on world/player change");Reset();return;}
        ++age_;
        if(wind_<0&&age_<=30){
            std::vector<java::EntityHandle> entities;double best=.5;
            Vec3 velocity;
            if(java::WorldEntities(self_,{origin_.x-40,origin_.y-10,origin_.z-40,origin_.x+40,origin_.y+70,origin_.z+40},entities))
                for(const auto& entity:entities){
                    Vec3 v,p;int id;
                    if(!java::ChargeWind(entity,self_,v)||!java::EntityId(entity,id)||oldWinds_.contains(id)||!entity.Position(p))continue;
                    const double length=v.DistanceToSqr({});if(length<1e-6)continue;
                    const auto delta=p.Subtract(origin_);
                    const double ticks=std::round((delta.x*v.x+delta.y*v.y+delta.z*v.z)/length);
                    if(ticks<0||ticks>age_+2)continue;
                    const double residual=p.Subtract(v.Scale(ticks)).DistanceTo(origin_);
                    if(residual<best){best=residual;wind_=id;velocity=v;}
                }
            if(wind_>=0){const auto error=velocity.Subtract(expected_);
                LogTo("AutoCharge: observed wind id=%d after=%d velocity=(%.5f,%.5f,%.5f) launchDelta=(%.5f,%.5f,%.5f) fit=%.4f",
                    wind_,age_,velocity.x,velocity.y,velocity.z,error.x,error.y,error.z,best);}
        }
        auto pearl=java::EntityById(pearl_);Vec3 position;
        if(pearl&&pearl.Position(position))lastPearl_=position;
        else if(gone_<0)gone_=age_;
        Vec3 feet;
        if(self_.Position(feet)){
            if(feet.DistanceTo(previous_)>4.&&(feet.DistanceTo(lastPearl_)<4.||feet.DistanceTo(destination_)<4.))teleported_=true;
            previous_=feet;
        }
        if(gone_>=0&&(teleported_||age_-gone_>=12)){
            LogTo("AutoCharge: observation pearlGone=1 windObserved=%d nearbyTeleport=%d after=%d position=(%.3f,%.3f,%.3f)",
                wind_>=0,teleported_,age_,previous_.x,previous_.y,previous_.z);
            Reset();
        }else if(age_>=90){
            LogTo("AutoCharge: observation timed out windObserved=%d pearlGone=%d nearbyTeleport=%d",wind_>=0,gone_>=0,teleported_);
            Reset();
        }
    }
private:
    bool active_=false,teleported_=false;
    int pearl_=-1,wind_=-1,age_=0,gone_=-1;
    Vec3 origin_{},expected_{},destination_{},previous_{},lastPearl_{};
    java::EntityHandle self_,world_;
    std::unordered_set<int> oldWinds_;
};
}
