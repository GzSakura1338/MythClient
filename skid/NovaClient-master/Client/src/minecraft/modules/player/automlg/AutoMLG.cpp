#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/combat/automace/AutoMace.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/modules/player/automlg/Prediction.h"
#include "minecraft/modules/player/automlg/WaterSearch.h"
#include "minecraft/modules/player/automlg/WebPlacement.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_mlg.h"
#include "minecraft/java/api/game_itemuse.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/RotationUtils.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/modules/combat/autoblock/AutoBlockRuntime.h"
#include "ui/clickgui/ClickGui.h"
#include <atomic>
#include <chrono>
#include <format>

namespace nova::module {
namespace {
std::atomic<bool> working{false};
class AutoMLG;
AutoMLG* instance=nullptr;
using Clock=std::chrono::steady_clock;
enum class Phase { Idle, Prepare, Place, WaitWater, Recover, WaitBucket };
enum class WaterTask { Fall, Fire, Web };
struct ConditionDelay {
    Clock::time_point since{};bool armed=false;
    bool Ready(bool condition,double delay) {
        if(!condition){armed=false;return false;}
        if(!armed){since=Clock::now();armed=true;}
        const double ms=std::isfinite(delay)?std::clamp(delay,0.,5000.):0.;
        return Clock::now()-since>=std::chrono::milliseconds(static_cast<int>(ms));
    }
};
class AutoMLG final:public Module {
public:
    AutoMLG():Module("AutoMLG",Category::Player,"General","General") {
        instance=this;
        minimum_=&addNumber("Min Fall Distance",3,15,.5,3,{},[](double n){return std::format("{:.1f}",n);});
        rotationSpeed_=&addNumber("Rotation Speed",1,180,1,45,{},[](double n){return std::format("{:.0f}",n);});
        auto delay=subNumber("Recovery Delay",0,1000,5,0,[](double n){return std::format("{:.0f} ms",n);});
        delay_=static_cast<Properties::NumberProp*>(delay.getProp());
        recover_=&addBool("Recover Water",true,{delay});
        auto collectDelay=subNumber("Delay",0,5000,5,0,[](double n){return std::format("{:.0f} ms",n);});
        collectDelay_=static_cast<Properties::NumberProp*>(collectDelay.getProp());
        autoCollect_=&addBool("Auto Collect Water",true,{collectDelay});
        auto fireDelay=subNumber("Delay",0,5000,5,0,[](double n){return std::format("{:.0f} ms",n);});
        fireDelay_=static_cast<Properties::NumberProp*>(fireDelay.getProp());
        antiFire_=&addBool("Anti Fire",false,{fireDelay});
        auto webDelay=subNumber("Delay",0,5000,5,0,[](double n){return std::format("{:.0f} ms",n);});
        webDelay_=static_cast<Properties::NumberProp*>(webDelay.getProp());
        antiWeb_=&addBool("Anti Web",false,{webDelay});
        auto& bus=event::EventBus::Get();
        tick_=bus.Subscribe<event::GameTickEvent>([this](auto& e){if(e.type==event::EventType::Pre)Tick();},event::Priority::Highest);
        interact_=bus.Subscribe<event::PrePlayerInteractEvent>([this](auto&){Interact();},event::Priority::Lowest);
        motion_=bus.Subscribe<event::MotionEvent>([this](auto& e){
            if(e.type==event::EventType::Post && phase_!=Phase::Idle && rotation::OwnsExclusiveRotations(this)) {
                published_={e.yaw,e.pitch};publishedValid_=true;
            }
        },event::Priority::Lowest);
        respawn_=bus.Subscribe<event::RespawnEvent>([this](auto&){reset_.store(true);},event::Priority::Highest);
    }
    ~AutoMLG() override {tick_.Reset();interact_.Reset();motion_.Reset();respawn_.Reset();java::ReleaseSilentSlot(this);working=false;rotation::ReleaseExclusiveRotations(this);instance=nullptr;}
    bool YieldForMace() {
        if(rotation::AutomatedItemUseSubmitted())return false;
        if(phase_!=Phase::Idle)Finish(false);
        next_={};return true; // No recovery cooldown when mace hands control back.
    }
private:
    void onEnable() override {enabled_=true;reset_=true;}
    void onDisable() override {enabled_=false;reset_=true;}
    bool SameWorld() {
        auto self=java::PlayerEntity(),world=java::LevelEntity();
        return self && world && player_ && world_ && self.IsSame(player_) && world.IsSame(world_);
    }
    void Finish(bool restore=true) {
        java::ReleaseSilentSlot(this);
        if(restore && SameWorld() && original_>=0)java::SyncSelectedItem();
        rotation::ReleaseExclusiveRotations(this);working=false;phase_=Phase::Idle;
        original_=slot_=-1;publishedValid_=enteredWater_=collecting_=false;safeSince_={};search_.Reset();
        fireTimer_=webTimer_={};task_=WaterTask::Fall;
        player_.Reset();world_.Reset();next_=Clock::now()+std::chrono::milliseconds(250);
    }
    bool Trace(const rotation::Rotation& angle,java::BlockHitResultInfo& hit,bool fluid) {
        rotation::Vec3 eye,look;
        return player_.EyePosition(1,eye) && java::Vec3DirectionFromRotation(angle.pitch,angle.yaw,look) &&
            java::WorldClip(eye,eye.Add(look.x*4.5,look.y*4.5,look.z*4.5),player_,hit,
                fluid?java::ClipMode::WaterSource:java::ClipMode::Outline) && hit.kind==java::HitKind::Block && hit.hasBlockPos;
    }
    bool Aim(rotation::Vec3 point) {
        rotation::Vec3 eye;if(!player_.EyePosition(1,eye))return false;
        rotation::Rotation from;
        if(rotation::State().active && rotation::State().hasRotations)from=rotation::GetRotation();
        else if(!java::PlayerRotation(from.yaw,from.pitch))return false;
        const double configured=rotationSpeed_->get();
        const double speed=std::isfinite(configured)?std::clamp(configured,1.,180.):45.;
        // One angular budget per game tick, shared by yaw and pitch. Follow the
        // shortest yaw arc; never snap to the landing point when time runs out.
        auto target=rotation::LinearStep(from,rotation::Calculate(eye,point),speed);
        double sensitivity=0;
        if(java::PlayerSensitivity(sensitivity))
            target=rotation::ApplySensitivityPatch(target,from,sensitivity);
        return rotation::SetExclusiveRotations(this,target);
    }
    bool FindLanding(const java::EntityHandle& self,automlg::Landing& landing) {
        java::Box box;rotation::Vec3 velocity;
        return self.BoundingBox(box) && java::PlayerDeltaMovement(velocity) &&
            automlg::Predict(box,velocity,[&](auto from,auto to,auto& hit){
                return java::WorldClip(from,to,self,hit,java::ClipMode::Collision);
            },landing);
    }
    bool AimWater() {
        rotation::Vec3 eye;
        return player_.EyePosition(1,eye) && Aim(automlg::WaterAimPoint(source_,eye));
    }
    bool FeetPlacement(const java::EntityHandle& self,automlg::Landing& out) {
        java::Box box;rotation::Vec3 eye;
        if(!self.BoundingBox(box)||!self.EyePosition(1,eye))return false;
        // Bucket use follows the outline ray. In a web this targets its top,
        // letting water flow down through it; no fake block edits are needed.
        const rotation::Vec3 end{(box.minX+box.maxX)*.5,box.minY-1.5,(box.minZ+box.maxZ)*.5};
        auto& hit=out.hit;
        if(!java::WorldClip(eye,end,self,hit,java::ClipMode::Outline)||hit.kind!=java::HitKind::Block||
           !hit.hasBlockPos||hit.direction!=1||automlg::DistanceSquared(eye,hit.location)>4.5*4.5)return false;
        auto pos=hit.blockPos;pos.y++;
        bool water=false,source=false;
        return std::abs(pos.y-box.minY)<=1.5 && java::MlgAllowsWater(pos)&&java::BlockStateReplaceable(pos)&&
            java::MlgWaterAt(pos,water,source)&&!water;
    }
    bool TryHazard(java::EntityHandle& self,java::EntityHandle& world,bool burning,bool web,bool inWater) {
        if(!antiFire_->get()&&!antiWeb_->get()){fireTimer_=webTimer_={};return false;}
        int bucket=-1;
        for(int i=0;i<9;++i)if(java::MlgBucketAt(i)==java::MlgBucket::Water){bucket=i;break;}
        const bool fireReady=fireTimer_.Ready(antiFire_->get()&&burning&&!inWater&&bucket>=0,fireDelay_->get());
        const bool webReady=webTimer_.Ready(antiWeb_->get()&&web&&bucket>=0,webDelay_->get());
        if(!fireReady&&!webReady)return false;
        // A head-level web also takes priority over fire; low water cannot clear it.
        const bool webTask=antiWeb_->get()&&web;
        if(webTask&&!webReady)return false;
        automlg::Landing target;if(!(webTask?WebPlacement(self,target):FeetPlacement(self,target)))return false;
        autocharge::YieldForRescue();
        player_=std::move(self);world_=std::move(world);slot_=bucket;landing_=target;
        task_=webTask?WaterTask::Web:WaterTask::Fire;
        phase_=Phase::Prepare;working=true;started_=Clock::now();publishedValid_=false;
        autoblock::SuspendForInteraction();if(!Aim(target.hit.location))Finish();
        return true;
    }
    bool WebPlacement(const java::EntityHandle& self,automlg::Landing& out) {
        rotation::Vec3 eye;if(!self.EyePosition(1,eye))return false;
        return automlg::FindWebPlacement(eye,webPos_,[&](auto a,auto b,auto& hit){
            return java::WorldClip(a,b,self,hit,java::ClipMode::Outline);
        },[](const auto& pos){bool water=false,source=false;
            return java::MlgAllowsWater(pos)&&java::BlockStateReplaceable(pos)&&
                java::MlgWaterAt(pos,water,source)&&!water;
        },out);
    }
    void TryCollect(java::EntityHandle& self,java::EntityHandle& world) {
        if(autocharge::IsWorking())return;
        if(!autoCollect_->get()){search_.Reset();return;}
        int bucket=-1;
        for(int i=0;i<9;++i)if(java::MlgBucketAt(i)==java::MlgBucket::Empty){bucket=i;break;}
        if(bucket<0){search_.Reset();return;}
        rotation::Vec3 eye;if(!self.EyePosition(1,eye))return;
        java::BlockPos source;
        if(!search_.Step(eye,[](const auto& pos){bool water=false,isSource=false;
            return java::MlgWaterAt(pos,water,isSource)&&water&&isSource;
        },[&](const auto& pos,const auto& point){java::BlockHitResultInfo hit;
            return java::WorldClip(eye,point,self,hit,java::ClipMode::WaterSource) &&
                hit.kind==java::HitKind::Block && hit.hasBlockPos && automlg::SameBlock(hit.blockPos,pos);
        },source))return;
        player_=std::move(self);world_=std::move(world);slot_=bucket;source_=source;
        collecting_=true;phase_=Phase::Recover;working=true;started_=Clock::now();publishedValid_=false;
        safeSince_=started_;
        autoblock::SuspendForInteraction();
        if(!AimWater())Finish();
    }
    void Tick() {
        if(reset_.exchange(false)){Finish();}
        if(!enabled_)return;
        if(automace::IsWorking()){if(phase_!=Phase::Idle)Finish(false);next_={};return;}
        auto self=java::PlayerEntity();auto world=java::LevelEntity();
        if(!self || !world || !self.IsAlive() || self.IsSpectator() ||
           java::ScreenOpen() || clickgui::IsOpen()) {if(phase_!=Phase::Idle)Finish();return;}
        if(phase_!=Phase::Idle && (!self.IsSame(player_) || !world.IsSame(world_))){Finish(false);return;}
        java::MlgEnvironment env;rotation::Vec3 velocity;float fall=0;
        if(!java::ReadMlgEnvironment(env) || !java::PlayerDeltaMovement(velocity) || !java::PlayerFallDistance(fall) ||
           env.lava || env.flying || env.passenger){if(phase_!=Phase::Idle)Finish();return;}
        const auto now=Clock::now();
        bool burning=false,web=false;
        if((antiFire_->get()||antiWeb_->get()||task_!=WaterTask::Fall) && !java::ReadMlgHazards(burning,web,webPos_)) {
            fireTimer_=webTimer_={};if(phase_!=Phase::Idle)Finish();return;
        }
        if(phase_==Phase::Idle) {
            if(now<next_)return;
            if(fall<3 || web){if(TryHazard(self,world,burning,web,env.water))return;}
            else fireTimer_=webTimer_={};
            // Do not collect the safety water while a dangerous fall is still in progress.
            if(fall<.1f && velocity.y>-.12 && !(antiFire_->get()&&burning) && !(antiWeb_->get()&&web))TryCollect(self,world);
            if(phase_!=Phase::Idle || env.water || java::PlayerOnGround() || velocity.y>=-.08)return;
            automlg::Landing landing;
            if(!FindLanding(self,landing) || landing.ticks>5)return;
            java::Box bounds;
            if(!self.BoundingBox(bounds) || fall+std::max(0.,bounds.minY-landing.hit.location.y)<minimum_->get())return;
            java::BlockPos waterPos=landing.hit.blockPos;waterPos.y++;
            if(!java::MlgAllowsWater(waterPos) || !java::BlockStateReplaceable(waterPos))return;
            int bucket=-1;
            for(int i=0;i<9;++i)if(java::MlgBucketAt(i)==java::MlgBucket::Water){bucket=i;break;}
            if(bucket<0)return;
            autocharge::YieldForRescue();
            player_=std::move(self);world_=std::move(world);slot_=bucket;landing_=landing;
            phase_=Phase::Prepare;working=true;started_=now;publishedValid_=false;
            autoblock::SuspendForInteraction();
            if(!Aim(landing_.hit.location))Finish();
            return; // Allow existing slot users and block queues to finish first.
        }
        int selected=-1;
        if(now-started_>std::chrono::seconds(5) || !java::SelectedSlot(selected)){Finish();return;}
        if(original_>=0 && selected!=original_){Finish();return;}
        if(collecting_ && (!autoCollect_->get() || fall>=.1f || velocity.y<=-.12)){Finish();return;}
        if(phase_==Phase::Prepare || phase_==Phase::Place) {
            const bool hazard=task_!=WaterTask::Fall;
            if((env.water&&task_!=WaterTask::Web) || (!hazard&&(java::PlayerOnGround()||velocity.y>=0)) ||
               (task_==WaterTask::Fire&&(!antiFire_->get()||!burning)) ||
               (task_==WaterTask::Web&&(!antiWeb_->get()||!web))){Finish();return;}
            automlg::Landing landing;
            const bool found=task_==WaterTask::Web?WebPlacement(player_,landing):hazard?FeetPlacement(player_,landing):FindLanding(player_,landing);
            if(!found || java::MlgBucketAt(slot_)!=java::MlgBucket::Water){Finish();return;}
            landing_=landing;
            if(!Aim(landing.hit.location)){Finish();return;}
            if(phase_==Phase::Prepare) {
                rotation::Vec3 eye;
                if(!player_.EyePosition(1,eye))return;
                const auto delta=landing.hit.location.Subtract(eye);
                const double distance=std::sqrt(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z);
                if(!hazard&&!automlg::SwitchReady(landing.ticks,distance,velocity.y))return;
                if(!java::SetSilentSlot(this,slot_))return;
                original_=selected;
                phase_=Phase::Place;
            }
            return;
        }
        bool water=false,source=false;
        if(!java::MlgWaterAt(source_,water,source)){Finish();return;}
        const auto bucket=java::MlgBucketAt(slot_);
        if(phase_==Phase::WaitBucket) {
            if(bucket==java::MlgBucket::Water){Finish();return;}
            if(now-lastUse_>std::chrono::milliseconds(700)) {
                if(source && bucket==java::MlgBucket::Empty){phase_=Phase::Recover;publishedValid_=false;}
                else Finish();
            }
            return;
        }
        enteredWater_|=env.water && water;
        // The brief immersion can happen between tick callbacks. A grounded,
        // reset fall distance also confirms landing beside our placed source.
        enteredWater_|=water && java::PlayerOnGround() && fall<.1f;
        if(phase_==Phase::WaitWater) {
            // A denied/rolled-back placement may return the full bucket. Retry
            // only when the world still has no water and another landing exists.
            if(!water && bucket==java::MlgBucket::Water && now-lastUse_>std::chrono::milliseconds(200) &&
               (task_!=WaterTask::Fall||(velocity.y<0&&!java::PlayerOnGround()))) {phase_=Phase::Place;publishedValid_=false;return;}
            if(task_!=WaterTask::Fall && ((antiFire_->get()&&burning)||(antiWeb_->get()&&web))){safeSince_={};return;}
            if(!automlg::SafeToRecover(enteredWater_,env.water,java::PlayerOnGround(),fall,velocity.y)){safeSince_={};return;}
            if(!recover_->get()){Finish();return;}
            if(safeSince_==Clock::time_point{})safeSince_=now;
            if(now-safeSince_<std::chrono::milliseconds(static_cast<int>(delay_->get())))return;
            if(!source || bucket!=java::MlgBucket::Empty){Finish();return;}
            // Never remove the only safety net above another dangerous drop.
            automlg::Landing below;auto probe=velocity;probe.y=-.15;
            java::Box box;
            if(!player_.BoundingBox(box) || !automlg::Predict(box,probe,[&](auto a,auto b,auto& hit){return java::WorldClip(a,b,player_,hit,java::ClipMode::Collision);},below) || box.minY-below.hit.location.y>2)return;
            phase_=Phase::Recover;publishedValid_=false;
        }
        if(phase_==Phase::Recover) {
            if(!source || bucket!=java::MlgBucket::Empty ||
               (!collecting_ && !automlg::SafeToRecover(enteredWater_,env.water,java::PlayerOnGround(),fall,velocity.y))){Finish();return;}
            rotation::Vec3 eye;
            if(!player_.EyePosition(1,eye) || automlg::DistanceSquared(eye,automlg::WaterAimPoint(source_,eye))>4.5*4.5){Finish();return;}
            java::BlockHitResultInfo visible;
            if(!java::WorldClip(eye,automlg::WaterAimPoint(source_,eye),player_,visible,java::ClipMode::WaterSource)||
               visible.kind!=java::HitKind::Block||!visible.hasBlockPos||!automlg::SameBlock(visible.blockPos,source_)){Finish();return;}
            if(collecting_&&now-safeSince_<std::chrono::milliseconds(static_cast<int>(std::clamp(collectDelay_->get(),0.,5000.))))return;
            if(original_<0) {
                if(!java::SetSilentSlot(this,slot_))return;
                original_=selected;
            }
            if(!AimWater()){Finish();return;}
        }
    }
    void Interact() {
        if(automace::IsWorking())return;
        if(compat::UsesModernRuntime() && !rotation::CanSubmitItemUse())return;
        if(!enabled_ || reset_ || !SameWorld() || !publishedValid_ || !rotation::OwnsExclusiveRotations(this) ||
           java::ScreenOpen() || clickgui::IsOpen() || (phase_!=Phase::Place && phase_!=Phase::Recover))return;
        if(original_<0)return; // A delayed collection has not selected its bucket yet.
        const auto now=Clock::now();if(now-lastUse_<std::chrono::milliseconds(100))return;
        int selected=-1,effective=-1;
        if(!java::SelectedSlot(selected)||selected!=original_||
           !java::EffectiveSelectedSlot(effective)||effective!=slot_){Finish();return;}
        auto angle=published_;
        if(compat::IsLegacyLunar() && !compat::lunar::PlayerReportedRotation(angle.yaw,angle.pitch))return;
        // Modern handleKeybinds runs before this tick's movement packet. USE_ITEM
        // carries explicit angles and must match that upcoming motion rotation.
        if(compat::UsesModernRuntime())angle=rotation::GetRotation();
        java::BlockHitResultInfo hit;
        if(!Trace(angle,hit,phase_==Phase::Recover))return;
        if(phase_==Phase::Place) {
            if(hit.direction!=landing_.hit.direction || !automlg::SameBlock(hit.blockPos,landing_.hit.blockPos))return;
            source_=automlg::Adjacent(hit);
            if(task_==WaterTask::Web&&!automlg::CanFlowToWeb(source_,webPos_))return;
            bool water=false,isSource=false;
            if(!java::MlgAllowsWater(source_) || !java::BlockStateReplaceable(source_) ||
               !java::MlgWaterAt(source_,water,isSource) || water)return;
        } else {
            bool water=false,isSource=false;
            if(!automlg::SameBlock(hit.blockPos,source_) || !java::MlgWaterAt(source_,water,isSource) || !water || !isSource)return;
        }
        const auto nextRotation=rotation::GetRotation();
        if(!rotation::SetExclusiveRotations(this,angle))return;
        struct RestoreRotation {
            const void* owner;rotation::Rotation angle;
            ~RestoreRotation(){if(rotation::OwnsExclusiveRotations(owner))rotation::SetExclusiveRotations(owner,angle);}
        } restore{this,nextRotation};
        lastUse_=now;
        const bool placed=java::UseMlgBucket(phase_==Phase::Place?java::MlgBucket::Water:java::MlgBucket::Empty,angle.yaw,angle.pitch);
        if(placed){phase_=phase_==Phase::Place?Phase::WaitWater:Phase::WaitBucket;publishedValid_=false;}
    }
    Properties::NumberProp *minimum_=nullptr,*delay_=nullptr,*rotationSpeed_=nullptr,*collectDelay_=nullptr,*fireDelay_=nullptr,*webDelay_=nullptr;
    Properties::BoolProperty *recover_=nullptr,*autoCollect_=nullptr,*antiFire_=nullptr,*antiWeb_=nullptr;
    std::atomic<bool> enabled_{false},reset_{false};
    Phase phase_=Phase::Idle;
    WaterTask task_=WaterTask::Fall;
    ConditionDelay fireTimer_,webTimer_;
    java::EntityHandle player_,world_;
    automlg::Landing landing_;
    automlg::WaterSearch search_;
    java::BlockPos source_,webPos_;
    rotation::Rotation published_;
    bool publishedValid_=false,enteredWater_=false,collecting_=false;
    int original_=-1,slot_=-1;
    Clock::time_point started_,next_,lastUse_,safeSince_;
    event::Subscription tick_,interact_,motion_,respawn_;
};
}
namespace automlg {
bool IsWorking(){return working.load();}
bool YieldForMace(){return instance?instance->YieldForMace():!rotation::AutomatedItemUseSubmitted();}
}
std::unique_ptr<Module> MakeAutoMLGModule(){return std::make_unique<AutoMLG>();}
}
