#include "minecraft/modules/Module.h"
#include "core/RuntimeControl.h"
#include "minecraft/modules/combat/autocrystal/MioPolicy.h"
#include "minecraft/modules/combat/legitcrystalaura/CrystalRules.h"
#include "minecraft/modules/combat/legitcrystalaura/PlacementPrediction.h"
#include "minecraft/modules/combat/legitcrystalaura/PlacementEsp.h"
#include "minecraft/modules/combat/shared/ExplosionCoordinator.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_crystal.h"
#include "minecraft/java/api/game_mace.h"
#include "minecraft/java/api/game_itemuse.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_freelook.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/PlacementWire.h"
#include "minecraft/utils/rotation/RotationUtils.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include "minecraft/utils/rotation/RayCastUtil.h"
#include "minecraft/utils/rotation/AimUtils.h"
#include "minecraft/modules/misc/teams/Teams.h"
#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/modules/player/refill/Refill.h"
#include "minecraft/modules/combat/automace/AutoMace.h"
#include "minecraft/modules/combat/autoblock/AutoBlockRuntime.h"
#include "minecraft/modules/combat/autorod/AutoRod.h"
#include "minecraft/modules/combat/velocity/Velocity.h"
#include "minecraft/modules/combat/antikb/AntiKB.h"
#include "minecraft/modules/combat/backtrack/Backtrack.h"
#include "ui/clickgui/ClickGui.h"
#include "ui/notifications/Notifications.h"
#include "utils/Keyboard.h"
#include <atomic>
#include <chrono>
#include <format>
#include <vector>

namespace nova::module::autocrystal {
using namespace crystal;
namespace {
auto& working=explosive::autoCrystalWorking;
using Vec=rotation::Vec3;
double Distance(const Vec& a,const Vec& b){return a.DistanceTo(b);}
Vec CrystalPosition(const java::BlockPos& p){return {p.x+.5,double(p.y+1),p.z+.5};}
bool SameBlock(const java::BlockPos& a,const java::BlockPos& b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
// Intersect the actual aim ray with the selected shape, ignoring world occlusion only.
bool RayBox(const Vec& eye,const Vec& direction,const java::Box& box,double reach,Vec& point,int& face) {
    const double length=std::hypot(direction.x,direction.y,direction.z);
    if(!std::isfinite(reach)||reach<=0||!std::isfinite(length)||length<1e-9)return false;
    const double origin[]{eye.x,eye.y,eye.z},delta[]{direction.x/length,direction.y/length,direction.z/length};
    const double minimum[]{box.minX,box.minY,box.minZ},maximum[]{box.maxX,box.maxY,box.maxZ};
    double entryDistance=0,exitDistance=reach;face=0;
    for(int axis=0;axis<3;++axis) {
        if(!std::isfinite(origin[axis])||!std::isfinite(minimum[axis])||!std::isfinite(maximum[axis])||minimum[axis]>maximum[axis])return false;
        if(std::abs(delta[axis])<1e-12) {
            if(origin[axis]<minimum[axis]||origin[axis]>maximum[axis])return false;
            continue;
        }
        double entry=(minimum[axis]-origin[axis])/delta[axis],exit=(maximum[axis]-origin[axis])/delta[axis];
        int side=axis==0?4:axis==1?0:2;
        if(entry>exit){std::swap(entry,exit);++side;}
        if(entry>=entryDistance){entryDistance=entry;face=side;}
        exitDistance=std::min(exitDistance,exit);
        if(entryDistance>exitDistance)return false;
    }
    point=eye.Add(delta[0]*entryDistance,delta[1]*entryDistance,delta[2]*entryDistance);return true;
}
std::int64_t Now(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
}

class AutoCrystal final : public Module {
public:
    AutoCrystal():Module("AutoCrystal",Category::Combat,"General","General") {
        enableOverlay();
        const auto integer=[](double v){return std::format("{:.0f}",v);};
        const auto decimal=[](double v){return std::format("{:.1f}",v);};
        auto range=subNumber("Range",1,6,.1,4.5,decimal),fov=subNumber("FOV",1,360,1,360,integer);
        range_=Number(range);fov_=Number(fov);
        auto walls=subBool("Through Walls",false);walls_=Bool(walls);
        addSettings({Properties::PropDesc("Target",nullptr,{range,fov,walls})});
        auto placeDelay=subNumber("Delay",0,1000,5,0,integer),autoSwitch=subBool("Auto Switch",true);
        placeDelay_=Number(placeDelay);autoSwitch_=Bool(autoSwitch);
        auto autoBase=subBool("Auto Base",false);autoBase_=Bool(autoBase);
        auto baseDelay=subNumber("Place Delay",0,1000,5,0,[](double v){return std::format("{:.0f} ms",v);});
        baseDelay_=Number(baseDelay);
        autoBase=Properties::PropDesc("Auto Base",autoBase_,{baseDelay});
        auto silent=subBool("Silent",false);silent_=Bool(silent);
        place_=&addBool("Place",true,{placeDelay,autoSwitch,autoBase,silent});
        swing_=&addBool("Swing",true);
        auto breakDelay=subNumber("Delay",0,1000,5,0,integer);breakDelay_=Number(breakDelay);
        break_=&addBool("Break",true,{breakDelay});
        auto speed=subNumber("Speed",1,180,1,45,integer);speed_=Number(speed);
        addSettings({Properties::PropDesc("Rotation",nullptr,{speed})});
        auto above=subBool("Only Above Crystal",false);
        auto ignoreSelf=subBool("Ignore Self Damage",false);ignoreSelf_=Bool(ignoreSelf);
        auto minimum=subNumber("Min Target Damage",0,36,.5,4,decimal);
        auto maximum=subNumber("Max Self Damage",0,36,.5,6,decimal);
        auto remaining=subNumber("Min Remaining Health",1,20,.5,4,decimal);
        above_=Bool(above);minimum_=Number(minimum);maximum_=Number(maximum);remaining_=Number(remaining);
        addSettings({Properties::PropDesc("Safety",nullptr,{above,ignoreSelf,minimum,maximum,remaining})});
        esp_=&addBool("Place ESP",true);
        auto& bus=event::EventBus::Get();
        // Return previous-tick inventory/rotation ownership before emergency modules run.
        tick_=bus.Subscribe<event::GameTickEvent>([this](auto& e){if(e.type==event::EventType::Pre)BeginTick();},event::Priority::Highest-2);
        // Decide after emergency modules, before KillAura requests rotations/attacks.
        prepare_=bus.Subscribe<event::GameTickEvent>([this](auto& e){if(e.type==event::EventType::Pre)Interact(true);},event::Priority::Highest+1);
        interact_=bus.Subscribe<event::PrePlayerInteractEvent>([this](auto&){Interact();},event::Priority::High);
        respawn_=bus.Subscribe<event::RespawnEvent>([this](auto&){reset_=true;placementEsp_.Clear();},event::Priority::Highest);
    }
    ~AutoCrystal() override {
        tick_.Reset();prepare_.Reset();interact_.Reset();respawn_.Reset();working=false;
        explosive::autoCrystalEnabled=false;explosive::autoCrystalConfirming=false;
        java::ReleaseSilentSlot(this);rotation::ReleaseExclusiveRotations(this);rotation::ReleaseAttack(this);
    }
private:
    enum Reject { NoSupport, Occupied, AimRay, LiveSafety, FutureSpace, Reach, ForecastDamage, Viable, RejectCount };
    void Report() {
        if(!enabled_)return;
        const auto now=Now();if(now<nextReport_)return;nextReport_=now+1000;
        const auto wire=rotation::ReadPlacementWire();
        LogTo("AutoCrystal: stage=%s autoBase=%d above=%d ignoreSelf=%d target=%d y=%.2f vy=%.3f grounded=%d selected=%d building=%d search=%zu/%zu reject[support=%u occupied=%u ray=%u safety=%u future=%u reach=%u damage=%u viable=%u] wire[known=%d rotated=%d pending=%d delta=%.6f placed=%.6f placedKnown=%d]",
            stage_,int(autoBase_->get()),int(above_->get()),int(ignoreSelf_->get()),targetId_,targetMotion_.position.y,targetMotion_.velocity.y,int(targetForecast_.grounded),int(haveBase_),int(haveBase_&&!committedBase_),searchCursor_,ranked_.size(),
            rejected_[NoSupport],rejected_[Occupied],rejected_[AimRay],rejected_[LiveSafety],rejected_[FutureSpace],rejected_[Reach],rejected_[ForecastDamage],rejected_[Viable],
            int(wire.deltaKnown),int(wire.rotated),int(wire.pendingPlace),double(wire.delta),double(wire.placedDelta),int(wire.hasPlaced));
        for(auto& count:rejected_)count=0;
    }
    static Properties::NumberProp* Number(const Properties::PropDesc& p){return static_cast<Properties::NumberProp*>(p.getProp());}
    static Properties::BoolProperty* Bool(const Properties::PropDesc& p){return static_cast<Properties::BoolProperty*>(p.getProp());}
    void onEnable() override {enabled_=true;reset_=true;explosive::autoCrystalEnabled=true;}
    void onDisable() override {enabled_=false;reset_=true;working=false;explosive::autoCrystalEnabled=false;explosive::autoCrystalConfirming=false;placementEsp_.Clear();}
    void onRender() override {if(enabled_&&esp_->get())placementEsp_.Draw();}
    bool SameWorld()const {
        auto self=java::PlayerEntity(),world=java::LevelEntity();
        return self&&world&&self.IsSame(player_)&&world.IsSame(world_);
    }
    bool Restore() {
        if(!slotActive_)return true;
        if(SameWorld()&&slotSilent_) {
            const bool owned=java::OwnsSlot(this);
            java::ReleaseSilentSlot(this);
            if((owned||restored_)&&!java::SyncSelectedItem()){restored_=true;return false;}
        } else if(switched_&&SameWorld()) {
            // A preempting module may temporarily equip a different item. Wait for
            // its lease to finish before deciding whether this was a manual scroll.
            if(!java::AcquirePhysicalSlot(this))return false;
            int selected=-1;
            // Do not undo a manual scroll or a replacement of the original stack.
            if(!java::SelectedSlot(selected))return false;
            if(selected==(restored_?oldSlot_:usedSlot_)&&java::MaceSlotMatches(oldSlot_,oldStack_)) {
                if(!restored_) {
                    if(!java::SetOwnedPhysicalSlot(this,oldSlot_))return false;
                    restored_=true;
                }
                if(!java::SyncSelectedItem())return false;
            }
        }
        slotActive_=switched_=restored_=false;oldStack_.Reset();java::ReleaseSilentSlot(this);return true;
    }
    bool Equip(int slot) {
        int selected=-1;if(!java::SelectedSlot(selected))return false;
        if(!slotActive_) {
            oldSlot_=selected;usedSlot_=selected;slotSilent_=silent_->get();
            oldStack_=java::MaceSlotIdentity(selected);if(!oldStack_)return false;
            slotActive_=true;
        }
        if(slotSilent_) {
            if(!java::SetSilentSlot(this,slot))return false;
        } else {
            if(!java::AcquirePhysicalSlot(this))return false;
            if(selected!=slot&&!java::SetOwnedPhysicalSlot(this,slot))return false;
            switched_=slot!=oldSlot_;
        }
        usedSlot_=slot;keepSlot_=true;return true;
    }
    void Clear() {
        Restore();confirmation_.Reset();pendingCrystal_.Reset();player_.Reset();world_.Reset();
        grid_.clear();searchActive_=searchFound_=false;searchTick_=-1;targetId_=-1;haveBase_=false;committedBase_=false;
        trackedTarget_.Reset();selfMotion_.Reset();targetMotion_.Reset();
        targets_.clear();targetsTick_=-1;
        retryAfter_=0;ackTicks_=1;blockedSince_=-1;replacementAllowed_=false;
        placementEsp_.Clear();
    }
    void BeginTick() {
        ++tickSerial_;
        exposureCache_.clear();
        usedTick_=aimedTick_=false;working=false;
        rotation::ReleaseExclusiveRotations(this);rotation::ReleaseAttack(this);
        int selected=-1;
        if(slotActive_) {
            java::EntityHandle self,world;
            if(!Context(self,world))restorePending_=true;
        }
        const bool manual=slotActive_&&java::SelectedSlot(selected)&&selected!=(slotSilent_?oldSlot_:(restored_?oldSlot_:usedSlot_));
        if(reset_||restorePending_||manual||(slotActive_&&(!SameWorld()||slotSilent_!=silent_->get()||!autoSwitch_->get()||!java::OwnsSlot(this)))) {
            restorePending_=!Restore();
            if(restorePending_||manual){usedTick_=true;return;}
        }
        // Keep effective selection stable while allowing emergency modules to take over.
        java::YieldSlot(this);
        if(reset_.exchange(false)) {
            Clear();nextPlace_=nextBase_=nextBreak_=0;
            if(enabled_&&!java::CrystalSupported()) {
                notifications::Post("AutoCrystal: requires Lunar 1.21.11 or Minecraft 26.3",notifications::Type::Warning);
                enabled_=false;setEnabled(false);
            }
        }
        if(player_&&!SameWorld()){Clear();usedTick_=true;}
        explosive::autoCrystalConfirming=enabled_&&confirmation_.Action()!=Confirmation::Kind::None;
    }
    bool Context(java::EntityHandle& self,java::EntityHandle& world)const {
        float yaw=0,pitch=0;self=java::PlayerEntity();world=java::LevelEntity();
        return enabled_&&!reset_&&self&&world&&self.IsAlive()&&!self.IsSpectator()&&java::CrystalContextReady()&&
            input::IsProcessForeground()&&!java::ScreenOpen()&&!clickgui::IsOpen()&&!java::FreeCamActive()&&!java::FreeLookAngles(yaw,pitch)&&
            !scaffold::IsEnabled()&&!automace::IsWorking()&&!automlg::IsWorking()&&!autocharge::IsWorking()&&
            !refill::IsWorking()&&!autoblock::OwnsItemUse()&&!autorod::OwnsItemUse()&&
            !velocity::IsDelaying()&&!antikb::IsDelaying()&&!backtrack::IsLagging();
    }
    bool Allowed(const java::EntityHandle& self,const java::EntityHandle& e)const {
        if(!e||e.IsSame(self)||!e.IsPlayer()||!e.IsAlive()||e.IsSpectator()||teams::IsTeammate(self,e)||antibot::IsBot(e))return false;
        bool creative=false;if(!java::CrystalCreative(e,creative)||creative)return false;
        Vec eye,point;java::Box box;float yaw=0,pitch=0;
        if(!self.EyePosition(1,eye)||!e.BoundingBox(box)||!e.EyePosition(1,point)||!java::PlayerRotation(yaw,pitch))return false;
        return Distance(eye,rotation::ClosestPointOnAabb(box,eye))<=TargetRange&&
            std::abs(std::remainder(double(rotation::Calculate(eye,point).yaw-yaw),360.))<=fov_->get()*.5;
    }
    java::EntityHandle Target(const java::EntityHandle& self) {
        if(targetsTick_!=tickSerial_) {
            targetsTick_=tickSerial_;
            std::vector<java::EntityHandle> players;
            std::vector<TargetSnapshot> next;
            if(!java::WorldPlayers(players)){targets_.clear();return {};}
            for(auto& e:players) {
                if(!Allowed(self,e))continue;
                TargetSnapshot snapshot;
                if(!java::EntityId(e,snapshot.id)||!java::ReadCrystalDefense(e,snapshot.defense))continue;
                for(const auto& old:targets_)if(old.entity.IsSame(e)){snapshot.motion=old.motion;break;}
                if(!SampleMotion(e,snapshot.motion,snapshot.forecast))continue;
                snapshot.entity=std::move(e);next.push_back(std::move(snapshot));
            }
            targets_=std::move(next);
        }
        for(auto& t:targets_)if(t.id==targetId_){UseTarget(t);return java::EntityById(t.id);}
        if(targets_.empty())return {};
        UseTarget(targets_.front());return java::EntityById(targetId_);
    }
    struct TargetSnapshot {
        java::EntityHandle entity;
        int id=-1;
        MotionSample motion;
        MotionForecast forecast;
        java::CrystalDefense defense;
    };
    void UseTarget(const TargetSnapshot& target) {
        targetId_=target.id;targetMotion_=target.motion;targetForecast_=target.forecast;
    }
    java::EntityHandle SelectTarget(int id) {
        for(const auto& t:targets_)if(t.id==id){UseTarget(t);return java::EntityById(id);}
        return {};
    }
    bool SampleMotion(const java::EntityHandle& entity,MotionSample& sample,MotionForecast& forecast) {
        if(sample.tick==tickSerial_)return true;
        Vec at,before;java::Box box;bool grounded=true;
        if(!entity.PositionPair(at,before)||!entity.BoundingBox(box)||!entity.OnGround(grounded)||!Finite(at)||!Finite(before))return false;
        sample.Update(tickSerial_,at,at.Subtract(before),grounded);
        forecast=Forecast(sample,box,grounded,[&](const java::Box& bounds){return java::CrystalMotionClear(entity,bounds);});
        return true;
    }
    bool ForecastExposure(const java::EntityHandle& entity,const Vec& point,const java::Box& box,
                          const java::BlockPos* base,double& exposure)const {
        int id=-1;if(!java::EntityId(entity,id))return false;
        for(const auto& c:exposureCache_)if(c.id==id&&c.point.DistanceToSqr(point)<1e-12&&c.building==(base!=nullptr)&&
            c.box.minX==box.minX&&c.box.minY==box.minY&&c.box.minZ==box.minZ&&
            c.box.maxX==box.maxX&&c.box.maxY==box.maxY&&c.box.maxZ==box.maxZ){exposure=c.value;return true;}
        if(!java::MioCrystalExposureAt(entity,point,box,base,exposure))return false;
        exposureCache_.push_back({id,point,box,base!=nullptr,exposure});return true;
    }
    bool PredictDamage(const java::EntityHandle& self,const java::EntityHandle& target,const Vec& point,double ticks,bool building,double& targetDamage)const {
        java::CrystalDefense enemy,own;double exposure=0;
        const java::BlockPos base{int(std::floor(point.x)),int(std::floor(point.y))-1,int(std::floor(point.z))};
        const auto* planned=building?&base:nullptr;
        const auto& enemyPath=targetForecast_;
        if(!building&&!Above(above_->get(),enemyPath.At(ticks).y,point.y))return false;
        if(!java::ReadCrystalDefense(target,enemy)||!ForecastExposure(target,point,enemyPath.BoxAt(ticks),planned,exposure))return false;
        double damage=Damage(Distance(enemyPath.At(ticks),point),exposure,enemy.difficulty,
            enemy.armor,enemy.toughness,enemy.resistance,enemy.protection);
        // Mio's execution fallback: keep useful live damage when extrapolation
        // alone falls below the configured minimum (but never rescue <=2 damage).
        const double threshold=Minimum(minimum_->get(),enemy.health,enemy.absorption);
        if(damage>2&&damage<threshold) {
            if(!ForecastExposure(target,point,targetForecast_.bounds,planned,exposure))return false;
            damage=std::max(damage,Damage(Distance(targetForecast_.origin,point),exposure,enemy.difficulty,
                enemy.armor,enemy.toughness,enemy.resistance,enemy.protection));
        }
        if(!std::isfinite(damage)||damage<threshold)return false;
        double selfDamage=0;
        if(!ignoreSelf_->get()) {
            const auto& ownPath=selfForecast_;
            if(!java::ReadCrystalDefense(self,own)||!ForecastExposure(self,point,ownPath.BoxAt(ticks),planned,exposure))return false;
            selfDamage=Damage(Distance(ownPath.At(ticks),point),exposure,own.difficulty,
                own.armor,own.toughness,own.resistance,own.protection);
            if(!Safe(selfDamage,own.health,own.absorption,maximum_->get(),remaining_->get()))return false;
        }
        targetDamage=damage;return true;
    }
    bool DamageTo(const java::EntityHandle& entity,const Vec& point,const java::CrystalDefense& defense,double& out,const java::BlockPos* base=nullptr)const {
        Vec p;double exposure=0;if(!entity.Position(p))return false;
        const double distance=Distance(p,point);
        if(distance>12){out=0;return true;}
        java::Box bounds;
        if(!entity.BoundingBox(bounds)||!ForecastExposure(entity,point,bounds,base,exposure))return false;
        out=Damage(distance,exposure,defense.difficulty,defense.armor,defense.toughness,defense.resistance,defense.protection);
        return std::isfinite(out);
    }
    bool Safety(const java::EntityHandle& self,const java::EntityHandle& target,const Vec& point,double& score,bool planning=false,bool building=false)const {
        // Only Above Crystal governs crystal use, not preparatory obsidian.
        // A normal knockback arc can expose a valid adjacent base cell while
        // the target's feet remain below that base's future crystal height.
        Vec p;if(!Allowed(self,target)||!target.Position(p)||(!building&&!Above(above_->get(),p.y,point.y)))return false;
        const java::BlockPos base{int(std::floor(point.x)),int(std::floor(point.y))-1,int(std::floor(point.z))};
        const auto* planned=building?&base:nullptr;
        java::CrystalDefense own,enemy;double toSelf=0,toEnemy=0;
        if(!java::ReadCrystalDefense(target,enemy)||!DamageTo(target,point,enemy,toEnemy,planned)||
           (!planning&&toEnemy<Minimum(minimum_->get(),enemy.health,enemy.absorption)))return false;
        if(!ignoreSelf_->get()&&(!java::ReadCrystalDefense(self,own)||!DamageTo(self,point,own,toSelf,planned)||
           !Safe(toSelf,own.health,own.absorption,maximum_->get(),remaining_->get())))return false;
        std::vector<java::EntityHandle> players;if(!java::WorldPlayers(players))return false;
        for(const auto& teammate:players) {
            if(teammate.IsSame(self)||!teammate.IsAlive()||teammate.IsSpectator()||!teams::IsTeammate(self,teammate))continue;
            Vec pos;if(!teammate.Position(pos))return false;
            if(Distance(pos,point)>12)continue;
            java::CrystalDefense defense;double damage=0;
            if(!java::ReadCrystalDefense(teammate,defense)||!DamageTo(teammate,point,defense,damage,planned)||
               !Safe(damage,defense.health,defense.absorption,maximum_->get(),remaining_->get()))return false;
        }
        score=toEnemy;return true;
    }
    bool Aim(const rotation::Rotation& wanted) {
        if(aimedTick_)return rotation::OwnsExclusiveRotations(this);
        if(!explosive::ChooseCrystal(offerDamage_,confirmation_.Action()!=Confirmation::Kind::None))return false;
        const auto from=rotation::GetRotation();
        const double remaining=std::hypot(std::remainder(double(wanted.yaw-from.yaw),360.),double(wanted.pitch-from.pitch));
        auto next=rotation::LinearStep(from,wanted,std::min(speed_->get(),remaining>1?remaining*.5:remaining));double sensitivity=0;
        if(java::PlayerSensitivity(sensitivity))next=rotation::ApplySensitivityPatch(next,from,sensitivity);
        if(!rotation::SetExclusiveRotations(this,next))return false;
        aimedTick_=true;working=true;return true;
    }
    bool PlacementHit(const java::EntityHandle& self,const java::BlockPos& base,const rotation::Rotation& angle,
                      double reach,java::BlockHitResultInfo& hit,bool visibleOnly=false)const {
        Vec eye,direction;if(!self.EyePosition(1,eye)||!java::Vec3DirectionFromRotation(angle.pitch,angle.yaw,direction))return false;
        if(walls_->get()&&!visibleOnly) {
            hit={};
            const java::Box box{double(base.x),double(base.y),double(base.z),double(base.x+1),double(base.y+1),double(base.z+1)};
            if(!RayBox(eye,direction,box,reach,hit.location,hit.direction))return false;
            hit.kind=java::HitKind::Block;hit.hasBlockPos=true;hit.blockPos=base;return true;
        }
        if(!java::WorldClip(eye,eye.Add(direction.x*reach,direction.y*reach,direction.z*reach),self,hit)||
           hit.kind!=java::HitKind::Block||!hit.hasBlockPos||!SameBlock(hit.blockPos,base))return false;
        return rotation::InteractionRayCast(angle,reach).type==rotation::HitResultType::Block;
    }
    bool BreakHit(const java::EntityHandle& self,const java::EntityHandle& crystal,const rotation::Rotation& angle,double reach)const {
        if(!walls_->get()) {
            const auto hit=rotation::InteractionRayCast(angle,reach);
            return hit.entity&&hit.entity.IsSame(crystal);
        }
        Vec eye,direction,point;java::Box box;int face=0;
        return self.EyePosition(1,eye)&&crystal.BoundingBox(box)&&
            java::Vec3DirectionFromRotation(angle.pitch,angle.yaw,direction)&&RayBox(eye,direction,box,reach,point,face);
    }
    bool PlaceAngle(const java::EntityHandle& self,const java::BlockPos& base,double reach,rotation::Rotation& angle,bool visibleOnly=false)const {
        Vec eye;if(!self.EyePosition(1,eye))return false;
        const auto current=rotation::GetRotation();java::BlockHitResultInfo currentHit;
        if(rotation::ReadPlacementWire().Ready()&&PlacementHit(self,base,current,reach,currentHit,visibleOnly)){angle=current;return true;}
        bool found=false;double nearest=1e9;
        // AutoCrystal.do1133 samples 2 x 3 x 2 interior points and takes
        // the closest legal surface hit. The submitted hit uses the actual ray.
        for(double x:{.05,.95})for(double y:{.05,.95,.99})for(double z:{.05,.95}) {
            const Vec point{base.x+x,base.y+y,base.z+z};
            if(Distance(eye,point)>reach)continue;
            auto desired=rotation::Calculate(eye,point);java::BlockHitResultInfo hit;
            if(!rotation::ReadPlacementWire().Ready()&&std::abs(std::remainder(double(desired.yaw-current.yaw),360.))<.0001&&
               std::abs(desired.pitch-current.pitch)<.0001)continue;
            if(PlacementHit(self,base,desired,reach,hit,visibleOnly)&&Distance(eye,hit.location)<nearest) {
                angle=desired;nearest=Distance(eye,hit.location);found=true;
            }
        }
        return found;
    }
    bool BaseAngle(const java::EntityHandle& self,const java::BlockPos& base,double reach,
                   rotation::Rotation& angle,java::BlockHitResultInfo& support)const {
        Vec eye;if(!self.EyePosition(1,eye))return false;
        // Prefer the floor, then side supports. The clicked face must lead to the base cell.
        const java::BlockPos steps[]={{0,1,0},{-1,0,0},{1,0,0},{0,0,-1},{0,0,1},{0,-1,0}};
        const int faces[]={1,4,5,2,3,0};
        for(int i=0;i<6;++i) {
            const auto d=steps[i];const java::BlockPos neighbor{base.x-d.x,base.y-d.y,base.z-d.z};
            if(!java::CrystalBaseSupport(neighbor))continue;
            const auto current=rotation::GetRotation();java::BlockHitResultInfo hit;
            if(rotation::ReadPlacementWire().Ready()&&PlacementHit(self,neighbor,current,reach,hit)&&hit.direction==faces[i]) {
                angle=current;support=hit;return true;
            }
            const Vec center{neighbor.x+.5+d.x*.5,neighbor.y+.5+d.y*.5,neighbor.z+.5+d.z*.5};
            // Sample inside the support face, retaining the same placement cell.
            const Vec u=d.x?Vec{0,.42,0}:Vec{.42,0,0};
            const Vec v=d.z?Vec{0,.42,0}:Vec{0,0,.42};
            for(const auto& offset:{Vec{},u,u.Scale(-1),v,v.Scale(-1)}) {
                const auto desired=rotation::Calculate(eye,center.Add(offset.x,offset.y,offset.z));
                if(!rotation::ReadPlacementWire().Ready()&&std::abs(std::remainder(double(desired.yaw-current.yaw),360.))<.0001&&
                   std::abs(desired.pitch-current.pitch)<.0001)continue;
                if(PlacementHit(self,neighbor,desired,reach,hit)&&hit.direction==faces[i]){angle=desired;support=hit;return true;}
            }
        }
        return false;
    }
    bool Crystals(const java::EntityHandle& self,std::vector<java::EntityHandle>& out)const {
        Vec p;if(!self.Position(p))return false;
        std::vector<java::EntityHandle> entities;
        if(!java::WorldEntities(self,{p.x-20,p.y-20,p.z-20,p.x+20,p.y+20,p.z+20},entities))return false;
        for(auto& e:entities)if(java::IsEndCrystal(e)&&e.IsAlive())out.push_back(std::move(e));
        return true;
    }
    bool Confirm(const std::vector<java::EntityHandle>& crystals,std::int64_t now) {
        const auto action=confirmation_.Action();
        if(confirmation_.Action()==Confirmation::Kind::Base) {
            if(java::CrystalBase(pendingBase_))confirmation_.Reset();
        } else if(confirmation_.Action()==Confirmation::Kind::Place) {
            for(const auto& e:crystals) {
                Vec p;
                if(e.Position(p)&&Distance(p,pendingPosition_)<.2){confirmation_.Reset();break;}
            }
        } else if(confirmation_.Action()==Confirmation::Kind::Break) {
            auto live=java::EntityById(pendingId_);
            if(!live||!live.IsSame(pendingCrystal_)||!live.IsAlive()){confirmation_.Reset();pendingCrystal_.Reset();}
        }
        if(action!=Confirmation::Kind::None&&confirmation_.Action()==Confirmation::Kind::None)
            ackTicks_=std::clamp(ackTicks_*.75+std::max(1,tickSerial_-submittedTick_)*.25,1.,4.);
        if(confirmation_.Waiting(now))return false;
        if(action==Confirmation::Kind::Base&&confirmation_.Action()==Confirmation::Kind::Base) {
            // Do not keep consuming slots/retrying the same unconfirmed support every tick.
            failedBase_=pendingBase_;retryAfter_=now+500;haveBase_=false;committedBase_=false;
        }
        confirmation_.Reset();pendingCrystal_.Reset();return true;
    }
    bool BreakCrystal(const java::EntityHandle& self,java::EntityHandle& target,
                      const std::vector<java::EntityHandle>& crystals,std::int64_t now,bool preview) {
        if(!break_->get()||now<nextBreak_)return false;
        int slot=-1;double reach=0;Vec eye;
        if(!java::EffectiveSelectedSlot(slot)||!java::MaceReach(slot,reach)||!self.EyePosition(1,eye))return false;
        reach=std::min(reach,range_->get());
        const java::EntityHandle* best=nullptr;rotation::Rotation desired;double bestScore=-1e9;
        int bestTarget=-1;
        for(const auto& e:crystals) {
            java::Box box;Vec point;double score=0;
            if(!java::MaceCanReach(slot,e)||!e.BoundingBox(box)||!e.Position(point))continue;
            const Vec center{(box.minX+box.maxX)*.5,(box.minY+box.maxY)*.5,(box.minZ+box.maxZ)*.5};
            const auto nearest=rotation::ClosestPointOnAabb(box,eye);
            const Vec inset=nearest.Add((center.x-nearest.x)*.08,(center.y-nearest.y)*.08,(center.z-nearest.z)*.08);
            auto angle=rotation::GetRotation();
            // A placed crystal usually intersects the placement ray already. Avoid
            // alternating fixed corner/face yaws on every place/break cycle.
            if(!BreakHit(self,e,angle,reach))angle=rotation::Calculate(eye,inset);
            if(!BreakHit(self,e,angle,reach))continue;
            for(const auto& candidate:targets_) {
                if(!BreakScore(self,candidate.entity,point,score)||score<=bestScore)continue;
                best=&e;bestScore=score;desired=angle;bestTarget=candidate.id;
            }
        }
        if(!best)return false;
        target=SelectTarget(bestTarget);if(!target)return false;
        keepSlot_=slotActive_;
        if(!preview&&slotActive_&&!Equip(usedSlot_))return true;
        Vec selectedPosition;
        if(haveBase_&&place_->get()&&best->Position(selectedPosition)&&Distance(selectedPosition,CrystalPosition(base_))<.2)
            placementEsp_.Show(base_);
        double targetDamage=0;java::CrystalDefense defense;Vec explosion;
        if(!best->Position(explosion)||!java::ReadCrystalDefense(target,defense)||!DamageTo(target,explosion,defense,targetDamage))return false;
        offerDamage_=targetDamage;
        if(!Aim(desired))return true;
        if(!rotation::AcquireAttack(this))return true;
        if(preview)return true;
        Vec point;double score=0;
        if(!BreakHit(self,*best,rotation::GetRotation(),reach)||!best->Position(point)||!BreakScore(self,target,point,score)||
           !java::MaceCanReach(slot,*best)||!rotation::CanSubmitAttack(this))return true;
        int id=-1;if(!java::EntityId(*best,id)||!rotation::BeginAttackSubmission(this))return true;
        bool sent=java::AttackEntity(*best);
        if(sent)sent=swing_->get()?java::SwingHand():java::SendSwingPacket();
        const bool entered=rotation::EndAttackSubmission();
        if(sent||entered) {
            pendingAngle_=rotation::GetRotation();
            nextBreak_=now+static_cast<std::int64_t>(breakDelay_->get());
            pendingId_=id;pendingCrystal_=java::EntityById(id);
            confirmation_.Start(Confirmation::Kind::Break,now,static_cast<std::int64_t>((ackTicks_+2)*50));
            submittedTick_=tickSerial_;
        }
        return true;
    }
    bool BreakScore(const java::EntityHandle& self,const java::EntityHandle& target,const Vec& point,double& score)const {
        if(!Safety(self,target,point,score,true))return false;
        for(const auto& candidate:targets_)if(candidate.entity.IsSame(target)) {
            double exposure=0;
            if(!ForecastExposure(target,point,candidate.forecast.BoxAt(PredictionTicks),nullptr,exposure))return false;
            const auto& d=candidate.defense;
            score=Damage(Distance(candidate.forecast.At(PredictionTicks),point),exposure,d.difficulty,d.armor,d.toughness,d.resistance,d.protection);
            return score>=Minimum(minimum_->get(),d.health,d.absorption);
        }
        return false;
    }
    void BuildGrid(const Vec& eye,const Vec& target,double reach) {
        grid_.clear();gridReach_=reach;
        for(int dx=-int(std::floor(reach));dx<int(std::ceil(reach));++dx)
            for(int dy=-int(std::floor(reach));dy<int(std::ceil(reach));++dy)
                for(int dz=-int(std::floor(reach));dz<int(std::ceil(reach));++dz) {
                    const java::BlockPos base{int(std::floor(eye.x+dx)),int(std::floor(eye.y+dy)),int(std::floor(eye.z+dz))};
                    if(Distance(eye,{base.x+.5,base.y+.5,base.z+.5})<=reach)grid_.push_back(base);
                }
        std::stable_sort(grid_.begin(),grid_.end(),[&](const auto& a,const auto& b){return CrystalPosition(a).DistanceToSqr(target)<CrystalPosition(b).DistanceToSqr(target);});
    }
    struct PlacementPlan {
        java::BlockPos base;
        java::BlockHitResultInfo support;
        rotation::Rotation angle;
        double targetDamage=0,ticks=1;
        int targetId=-1;
        PlacementStage stage=PlacementStage::Direct;
        bool building=false;
    };
    static bool BetterPlan(const PlacementPlan& next,const PlacementPlan& old,bool retained=false) {
        return Better(next.stage,next.targetDamage,old.stage,old.targetDamage,retained);
    }
    bool EvaluatePlan(const java::EntityHandle& self,const java::EntityHandle& target,
                      const std::vector<java::EntityHandle>& crystals,const java::BlockPos& base,
                      double reach,bool canBuild,std::int64_t now,PlacementPlan& plan)const {
        plan={};plan.base=base;plan.building=!java::CrystalBase(base);
        java::EntityId(target,plan.targetId);
        if(plan.building) {
            if(!canBuild||(now<retryAfter_&&SameBlock(base,failedBase_)))return false;
            plan.stage=PlacementStage::Obsidian;
            if(base.y>targetForecast_.origin.y+1.5)return false;
            // Mio validates crystal space before committing to obsidian, not
            // only after construction. Never build a base already unusable.
            if(!java::CrystalBaseBuildable(base)||!java::CrystalSpaceClear(base)){++rejected_[Occupied];return false;}
            if(!BaseAngle(self,base,reach,plan.angle,plan.support)){++rejected_[AimRay];return false;}
        } else {
            if(!java::CrystalPlaceable(base))return false;
            if(!PlaceAngle(self,base,reach,plan.angle,true)) {
                if(!walls_->get()||!PlaceAngle(self,base,reach,plan.angle))return false;
                plan.stage=PlacementStage::Occluded;
            }
        }
        double liveScore=0;
        if(!Safety(self,target,CrystalPosition(base),liveScore,true,plan.building)){++rejected_[LiveSafety];return false;}
        Vec eye;if(!self.EyePosition(1,eye))return false;
        // Mio uses a fixed extrapolation horizon, not rotation/ack delay summed
        // into an eight-tick forecast that moves the damage window away.
        plan.ticks=PredictionTicks;
        const auto volume=java::CrystalPlacementVolume(base,false);
        const double occupancyTicks=PredictionTicks;
        {
            // Match Mio's horizontal occupancy extrapolation: keep current Y
            // bounds, so a downward world forecast cannot reserve empty air.
            for(int i=1;i<=int(std::ceil(occupancyTicks));++i) {
                auto box=targetForecast_.BoxAt(i);
                box.minY=targetForecast_.bounds.minY;box.maxY=targetForecast_.bounds.maxY;
                if(Overlaps(box,volume)){++rejected_[FutureSpace];return false;}
            }
        }
        if(plan.building) {
            const auto future=targetForecast_.BoxAt(PredictionTicks);
            const auto live=targetForecast_.bounds;
            const java::Box swept{std::min(live.minX,future.minX),std::min(live.minY,future.minY),std::min(live.minZ,future.minZ),
                std::max(live.maxX,future.maxX),std::max(live.maxY,future.maxY),std::max(live.maxZ,future.maxZ)};
            if(Overlaps(swept,java::CrystalPlacementVolume(base,true))){++rejected_[FutureSpace];return false;}
        }
        const auto futureEye=selfForecast_.At(plan.building?1:0).Add(eye.x-selfForecast_.origin.x,
            eye.y-selfForecast_.origin.y,eye.z-selfForecast_.origin.z);
        const java::Box baseBox{double(base.x),double(base.y),double(base.z),double(base.x+1),double(base.y+1),double(base.z+1)};
        if(Distance(futureEye,rotation::ClosestPointOnAabb(baseBox,futureEye))>reach){++rejected_[Reach];return false;}
        if(!PredictDamage(self,target,CrystalPosition(base),plan.ticks,plan.building,plan.targetDamage)){++rejected_[ForecastDamage];return false;}
        ++rejected_[Viable];
        // Damage ranks candidates; self damage and reach are hard constraints.
        return true;
    }
    bool EvaluateAll(const java::EntityHandle& self,const std::vector<java::EntityHandle>& crystals,
                     const java::BlockPos& base,double reach,bool canBuild,std::int64_t now,PlacementPlan& best) {
        // Mio's occupancy loop considers every player, not only the player
        // eventually selected for damage at this cell.
        const auto volume=java::CrystalPlacementVolume(base,false);
        for(const auto& candidate:targets_)for(int i=1;i<=int(PredictionTicks);++i) {
            auto bounds=candidate.forecast.BoxAt(i);
            bounds.minY=candidate.forecast.bounds.minY;bounds.maxY=candidate.forecast.bounds.maxY;
            if(Overlaps(bounds,volume))return false;
        }
        const int previous=targetId_;bool found=false;
        for(const auto& candidate:targets_) {
            UseTarget(candidate);
            PlacementPlan next;
            if(EvaluatePlan(self,candidate.entity,crystals,base,reach,canBuild,now,next)&&
               (!found||next.targetDamage>best.targetDamage)){best=next;found=true;}
        }
        SelectTarget(previous);
        return found;
    }
    double DamageCeiling(const Vec& point)const {
        double maximum=0;
        for(const auto& candidate:targets_) {
            const auto& d=candidate.defense;
            const double damage=Damage(candidate.forecast.NearestDistance(point),1,d.difficulty,d.armor,d.toughness,d.resistance,d.protection);
            if(damage>=Minimum(minimum_->get(),d.health,d.absorption))maximum=std::max(maximum,damage);
        }
        return maximum;
    }
    bool MaximumDamagePlan(const java::EntityHandle& self,const java::EntityHandle& target,
                           const std::vector<java::EntityHandle>& crystals,const Vec& eye,
                           double reach,bool canBuild,std::int64_t now,PlacementPlan& best) {
        // Both preparation and interaction run each tick. Never spend the search
        // budget twice, including when no candidate has been found yet.
        if(searchTick_==tickSerial_)return false;
        searchTick_=tickSerial_;
        if(grid_.empty()||Distance(eye,searchEye_)>4.||Distance(targetForecast_.origin,searchTarget_)>4.||
           (!searchFound_&&Distance(targetMotion_.velocity,searchVelocity_)>.2)||
           std::abs(reach-gridReach_)>.01||canBuild!=searchCanBuild_)searchActive_=false;
        if(!searchActive_) {
            searchStartedTick_=tickSerial_;
            searchCanBuild_=canBuild;
            BuildGrid(eye,targetForecast_.At(ackTicks_+2),reach);
            searchEye_=eye;searchTarget_=targetForecast_.origin;searchVelocity_=targetMotion_.velocity;
            ranked_.clear();ranked_.reserve(grid_.size());searchCursor_=0;searchFound_=false;
            for(const auto& base:grid_) {
                const auto point=CrystalPosition(base);
                // Height is stage-specific and checked in EvaluatePlan. Filtering
                // here also permanently omitted these cells from a ground-started
                // search after the target was subsequently knocked into the air.
                // Full exposure bounds damage independently of action timing.
                const double ceiling=DamageCeiling(point);
                if(ceiling>0)ranked_.push_back({base,ceiling});
            }
            std::stable_sort(ranked_.begin(),ranked_.end(),[](const auto& a,const auto& b){return a.ceiling>b.ceiling;});
            searchActive_=true;
        }
        const auto started=std::chrono::steady_clock::now();
        if(searchFound_) {
            PlacementPlan refreshed;
            searchFound_=EvaluateAll(self,crystals,searchBest_.base,reach,canBuild,now,refreshed);
            if(searchFound_)searchBest_=refreshed;
        }
        int visited=0,detailed=0;
        while(searchCursor_<ranked_.size()&&visited<128&&detailed<4) {
            if(visited&&std::chrono::steady_clock::now()-started>=std::chrono::milliseconds(2))break;
            const auto candidate=ranked_[searchCursor_++];++visited;
            const double upper=DamageCeiling(CrystalPosition(candidate.base));
            if(searchFound_&&searchBest_.stage==PlacementStage::Direct&&upper<searchBest_.targetDamage-1e-6)continue;
            const bool existing=java::CrystalBase(candidate.base);
            if(!existing) {
                if(!canBuild)continue;
                // Most air cells have no support. Reject them with cheap block
                // queries before allocating entity lists or casting damage rays.
                bool support=false;
                for(const auto& d:{java::BlockPos{0,-1,0},{1,0,0},{-1,0,0},{0,0,1},{0,0,-1},{0,1,0}})
                    if(java::CrystalBaseSupport({candidate.base.x+d.x,candidate.base.y+d.y,candidate.base.z+d.z})){support=true;break;}
                if(!support){++rejected_[NoSupport];continue;}
            }
            ++detailed;
            PlacementPlan next;
            if(!EvaluateAll(self,crystals,candidate.base,reach,canBuild,now,next))continue;
            if(!searchFound_||BetterPlan(next,searchBest_)) {
                searchBest_=next;searchFound_=true;
            }
        }
        // Mio publishes a completed search, not the first usable cell. Keep
        // the previous executable plan while the next bounded scan completes.
        if(searchCursor_<ranked_.size())return false;
        searchActive_=false;
        // Scores accumulated across ticks are estimates; never submit a stale hit.
        const bool valid=searchFound_&&EvaluateAll(self,crystals,searchBest_.base,reach,canBuild,now,best);
        return valid;
    }
    void PlaceCrystal(const java::EntityHandle& self,java::EntityHandle& target,
                      const std::vector<java::EntityHandle>& crystals,std::int64_t now,bool preview) {
        stage_="place-disabled/delay/interaction-busy";
        if(!place_->get()||!rotation::CanSubmitItemUse())return;
        // Delay gates the use, not preparation. Keep aiming a revalidated plan
        // before KillAura runs so it cannot pull the look back between uses.
        int selected=-1,slot=-1;
        if(!java::EffectiveSelectedSlot(selected))return;
        if(java::CrystalHotbarSlot(selected))slot=selected;
        else if(autoSwitch_->get())for(int i=0;i<9;++i)if(java::CrystalHotbarSlot(i)){slot=i;break;}
        const bool offhand=java::CrystalOffhand();
        if(slot<0&&!offhand){stage_="no-crystal";return;}
        int obsidian=-1;
        if(autoBase_->get()&&autoSwitch_->get())for(int i=0;i<9;++i)if(java::CrystalObsidianSlot(i)){obsidian=i;break;}
        stage_=autoBase_->get()?(obsidian>=0?"search-base":"no-hotbar-obsidian/switch-disabled"):"auto-base-off";
        Vec eye;double reach=0;
        if(!self.EyePosition(1,eye)||!java::CrystalBlockReach(reach))return;
        reach=std::min(reach,range_->get());PlacementPlan selectedPlan;
        if(autoBase_->get()&&!committedBase_&&haveBase_&&!java::CrystalBase(base_)&&selectedTick_+6<tickSerial_) {
            haveBase_=false;searchActive_=false;
        }
        if(autoBase_->get()&&!committedBase_&&haveBase_&&Distance(selectedTarget_,targetForecast_.origin)>.75) {
            haveBase_=false;searchActive_=false;
        }
        // Give transient occupants a short grace period. After that, a blocked
        // confirmed base must not prevent constructing a usable alternative.
        double committedScore=0;
        const auto waitingBase=base_;
        const bool waitingForSpace=committedBase_&&haveBase_&&java::CrystalBase(base_)&&!java::CrystalPlaceable(base_)&&
           Distance(eye,CrystalPosition(base_))<=reach&&
           Safety(self,target,CrystalPosition(base_),committedScore);
        if(waitingForSpace) {
            if(blockedSince_<0||!SameBlock(blockedBase_,waitingBase)) {
                blockedBase_=waitingBase;blockedSince_=tickSerial_;replacementAllowed_=false;
            }
            if(!replacementAllowed_&&tickSerial_-blockedSince_>=6) {
                replacementAllowed_=true;
                // The previous scan skipped unbuilt cells. Revisit them once.
                searchActive_=false;
            }
            haveBase_=committedBase_=false;
            if(!replacementAllowed_)obsidian=-1;
        } else {blockedSince_=-1;replacementAllowed_=false;}
        if(haveBase_&&!EvaluateAll(self,crystals,base_,reach,obsidian>=0,now,selectedPlan)) {
            haveBase_=false;committedBase_=false;
        }
        // Both Auto Base states use the same stage/damage selection. Continue a
        // budgeted scan beside the retained plan; pin newly built bases for follow-up.
        if(haveBase_&&selectedPlan.building)committedBase_=false;
        if(!committedBase_&&(preview||!aimedTick_)) {
            PlacementPlan next;
            if(MaximumDamagePlan(self,target,crystals,eye,reach,obsidian>=0,now,next)&&
               (!haveBase_||BetterPlan(next,selectedPlan,true))) {
                selectedPlan=next;base_=next.base;haveBase_=true;
                selectedTick_=tickSerial_;selectedTarget_=targetForecast_.origin;
            }
        }
        if(!haveBase_) {
            if(waitingForSpace){stage_=replacementAllowed_?"blocked-base-replan":"blocked-base-grace";base_=waitingBase;haveBase_=committedBase_=true;keepSlot_=slotActive_;placementEsp_.Show(base_);}
            return;
        }
        const bool building=selectedPlan.building;
        target=SelectTarget(selectedPlan.targetId);if(!target)return;
        stage_=building?"aim-base":"aim-crystal";
        keepSlot_=slotActive_;
        placementEsp_.Show(base_);
        offerDamage_=selectedPlan.targetDamage;
        if(!Aim(selectedPlan.angle))return;
        if(preview)return;
        if(now<(building?nextBase_:nextPlace_)){stage_=building?"base-delay":"crystal-delay";return;}
        stage_="rotation-history-wait";
        // Placement is evaluated against the server's preceding look update.
        // The upcoming motion cannot make a currently repeated delta legal.
        if(!rotation::ReadPlacementWire().Ready())return;
        java::BlockHitResultInfo hit;PlacementPlan checked;
        stage_="submission-recheck/ray";
        if(!EvaluatePlan(self,target,crystals,base_,reach,obsidian>=0,now,checked)||checked.building!=building||
           !PlacementHit(self,building?checked.support.blockPos:base_,rotation::GetRotation(),reach,hit)||
           (building&&hit.direction!=checked.support.direction)||
           !rotation::CanSubmitItemUse())return;
        if(building)slot=obsidian;
        stage_="equip-failed";
        if(building||!offhand){if(!Equip(slot))return;}
        else if(!Restore())return;
        if(!rotation::ReadPlacementWire().Ready()){stage_="rotation-history-wait";return;}
        bool entered=false;
        const bool accepted=building?java::PlaceCrystalBase(base_,hit,slot,this,entered):
            offhand?java::PlaceOffhandEndCrystal(hit,this,entered):java::PlaceEndCrystal(hit,slot,this,entered);
        stage_=entered?(building?"base-submitted":"crystal-submitted"):"native-use-not-entered";
        if(entered&&now>=nextSubmitReport_){
            nextSubmitReport_=now+1000;
            LogTo("AutoCrystal: submitted base=%d pos=(%d,%d,%d) slot=%d accepted=%d",int(building),base_.x,base_.y,base_.z,slot,int(accepted));
        }
        if(accepted)java::SwingCrystalHand(!building&&offhand,swing_->get());
        if(entered) {
            pendingAngle_=rotation::GetRotation();
            if(building)nextBase_=now+static_cast<std::int64_t>(baseDelay_->get());
            nextPlace_=now+static_cast<std::int64_t>(placeDelay_->get());pendingPosition_=CrystalPosition(base_);
            pendingBase_=base_;
            committedBase_=building;submittedTick_=tickSerial_;
            confirmation_.Start(building?Confirmation::Kind::Base:Confirmation::Kind::Place,now,static_cast<std::int64_t>((ackTicks_+2)*50));
        }
        if(!entered)keepSlot_=false;
    }
    void Interact(bool preview=false) {
        if(usedTick_)return;
        if(!preview)usedTick_=true;
        keepSlot_=false;
        struct Finish {AutoCrystal* self;bool preview;~Finish(){if(!self->keepSlot_)self->restorePending_=!self->Restore();if(!preview)self->Report();}} finish{this,preview};
        stage_="context-blocked";
        java::EntityHandle self,world;
        if(!Context(self,world)){
            placementEsp_.Clear();selfMotion_.Reset();targetMotion_.Reset();return;
        }
        if(!player_){player_=java::PlayerEntity();world_=java::LevelEntity();}
        if(!self.IsSame(player_)||!world.IsSame(world_)){placementEsp_.Clear();return;}
        if(slotActive_&&!java::OwnsSlot(this))return;
        std::vector<java::EntityHandle> crystals;
        if(!Crystals(self,crystals)){placementEsp_.Clear();return;}
        auto target=Target(self);
        stage_="no-target";
        if(!target){haveBase_=committedBase_=false;grid_.clear();trackedTarget_.Reset();targetMotion_.Reset();placementEsp_.Clear();return;}
        stage_="motion-read-failed";
        if(!SampleMotion(self,selfMotion_,selfForecast_)||!SampleMotion(target,targetMotion_,targetForecast_)) {
            haveBase_=committedBase_=false;placementEsp_.Clear();return;
        }
        const auto now=Now();
        const bool ready=Confirm(crystals,now);
        // Keep the same base through the place/break acknowledgement cycle.
        // Only its discovery is cached; legality and damage are rechecked before use.
        double score=0;bool retainPendingAim=false;
        const bool waitingBaseConfirmation=confirmation_.Action()==Confirmation::Kind::Base;
        if(haveBase_&&place_->get()&&(java::CrystalBase(base_)||
           (autoBase_->get()&&java::CrystalBaseBuildable(base_)))&&
           Safety(self,target,CrystalPosition(base_),score,waitingBaseConfirmation,waitingBaseConfirmation)) {
            placementEsp_.Show(base_);
            bool stock=java::CrystalOffhand();for(int i=0;i<9&&!stock;++i)stock=java::CrystalHotbarSlot(i);
            keepSlot_=slotActive_&&(stock||!crystals.empty());
            retainPendingAim=stock;
        }
        else placementEsp_.Clear();
        stage_="confirmation-wait";
        if(!ready) {
            // The confirmed-world transition is still part of this placement.
            // Hold the submitted direction for the bounded confirmation window;
            // Context above still yields to rescue modules, screens and target loss.
            if(retainPendingAim)Aim(pendingAngle_);
            return;
        }
        placementEsp_.Clear();
        stage_="break-crystal";
        if(!BreakCrystal(self,target,crystals,now,preview))PlaceCrystal(self,target,crystals,now,preview);
    }
    Properties::NumberProp *range_{},*fov_{},*speed_{},*placeDelay_{},*baseDelay_{},*breakDelay_{},*minimum_{},*maximum_{},*remaining_{};
    mutable unsigned rejected_[RejectCount]{};
    const char* stage_="idle";
    std::int64_t nextReport_=0,nextSubmitReport_=0;
    Properties::BoolProperty *place_{},*break_{},*autoSwitch_{},*above_{},*esp_{},*walls_{},*ignoreSelf_{},*autoBase_{},*silent_{},*swing_{};
    PlacementEsp placementEsp_;
    event::Subscription tick_,prepare_,interact_,respawn_;
    rotation::Rotation pendingAngle_{};
    std::atomic<bool> enabled_{false},reset_{false};
    java::EntityHandle player_,world_,oldStack_,pendingCrystal_,trackedTarget_;
    MotionSample selfMotion_,targetMotion_;
    MotionForecast selfForecast_,targetForecast_;
    std::vector<TargetSnapshot> targets_;
    int targetsTick_=-1;
    struct ExposureEntry {int id;Vec point;java::Box box;bool building;double value;};
    mutable std::vector<ExposureEntry> exposureCache_;
    Confirmation confirmation_;
    Vec pendingPosition_;
    java::BlockPos base_,pendingBase_,failedBase_,blockedBase_;
    int blockedSince_=-1;
    bool replacementAllowed_=false;
    std::vector<java::BlockPos> grid_;
    struct SearchCandidate {java::BlockPos base;double ceiling;};
    std::vector<SearchCandidate> ranked_;
    PlacementPlan searchBest_;
    Vec searchEye_,searchTarget_,searchVelocity_,selectedTarget_;
    size_t searchCursor_=0;
    int searchTick_=-1,searchStartedTick_=0;
    bool searchActive_=false,searchFound_=false,searchCanBuild_=false;
    bool usedTick_=false,aimedTick_=false,haveBase_=false,switched_=false,restored_=false,committedBase_=false;
    bool slotActive_=false,slotSilent_=false,keepSlot_=false,restorePending_=false;
    int tickSerial_=0,submittedTick_=0,selectedTick_=0;
    double ackTicks_=1,gridReach_=0;
    double offerDamage_=0;
    int targetId_=-1,oldSlot_=-1,usedSlot_=-1,pendingId_=-1;
    std::int64_t nextPlace_=0,nextBase_=0,nextBreak_=0,retryAfter_=0;
};
}
namespace nova::module {
std::unique_ptr<Module> MakeAutoCrystalModule(){return std::make_unique<autocrystal::AutoCrystal>();}
}
