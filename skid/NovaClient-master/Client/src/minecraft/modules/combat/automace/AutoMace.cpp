#include "minecraft/modules/combat/automace/AutoMace.h"
#include "minecraft/modules/Module.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_mace.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_itemuse.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_freelook.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/RotationUtils.h"
#include "minecraft/utils/rotation/RayCastUtil.h"
#include "minecraft/utils/rotation/AimUtils.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include "minecraft/modules/misc/teams/Teams.h"
#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/modules/player/refill/Refill.h"
#include "minecraft/modules/combat/autoblock/AutoBlockRuntime.h"
#include "minecraft/modules/combat/autorod/AutoRod.h"
#include "minecraft/modules/combat/velocity/Velocity.h"
#include "minecraft/modules/combat/antikb/AntiKB.h"
#include "minecraft/modules/combat/backtrack/Backtrack.h"
#include "ui/clickgui/ClickGui.h"
#include "core/RuntimeControl.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <format>
#include <vector>

namespace nova::module::automace {
namespace { std::atomic<bool> working{false}; }

// AutoWeapon (LiquidBounce): OnTarget / OnAttack switching and no full-cooldown
// gate on native mace smash. Targeting and automatic submission are Nova's own.
class AutoMace final : public Module {
    enum class Phase { Idle, Aiming, MaceFollowUp, Restore };
public:
    AutoMace() : Module("AutoMace",Category::Combat,"General","General") {
        mode_=&addMode("Mode",1,{"Quick Switch","Normal"});
        range_=&addNumber("Range",1,6,.05,3,{},[](double v){return std::format("{:.2f}",v);});
        fov_=&addNumber("FOV",0,360,1,180,{},[](double v){return std::format("{:.0f}",v);});
        speed_=&addNumber("Rotation speed",1,180,1,180,{},[](double v){return std::format("{:.0f}",v);});
        axe_=&addBool("Axe",false,{},[this]{return mode_->get()==0;});
        auto player=subBool("Player",true), mobs=subBool("Mobs",false), animals=subBool("Animals",false);
        targetPlayer_=static_cast<Properties::BoolProperty*>(player.getProp());
        targetMobs_=static_cast<Properties::BoolProperty*>(mobs.getProp());
        targetAnimals_=static_cast<Properties::BoolProperty*>(animals.getProp());
        // Append the group without dropping the base Enabled/Bind rows or any
        // registered settings (also used by config serialization).
        addSettings({Properties::PropDesc("Target",nullptr,{player,mobs,animals})});
        auto& bus=event::EventBus::Get();
        // Decide before MLG, independent of module registration order.
        tick_=bus.Subscribe<event::GameTickEvent>([this](auto& e){if(e.type==event::EventType::Pre)Tick();},event::Priority::Highest-1);
        interact_=bus.Subscribe<event::PrePlayerInteractEvent>([this](auto&){Interact();},event::Priority::Highest-1);
        respawn_=bus.Subscribe<event::RespawnEvent>([this](auto&){reset_=true;},event::Priority::Highest);
    }
    ~AutoMace() override {
        tick_.Reset();interact_.Reset();respawn_.Reset();
        // Destruction may be off-thread. Never send inventory packets here.
        java::ReleaseSilentSlot(this);rotation::ReleaseExclusiveRotations(this);rotation::ReleaseAttack(this);working=false;
    }
private:
    void onEnable() override {enabled_=true;reset_=true;}
    void onDisable() override {enabled_=false;reset_=true;}
    static bool Smash(const java::MaceState& s) {
        return !s.ground&&!s.gliding&&!s.obstructed&&!s.interacting&&s.fall>1.5&&s.verticalSpeed<0;
    }
    bool Context(java::EntityHandle& self,java::EntityHandle& world) {
        self=java::PlayerEntity();world=java::LevelEntity();float yaw=0,pitch=0;
        return self&&world&&self.IsAlive()&&!self.IsSpectator()&&!java::ScreenOpen()&&!clickgui::IsOpen()&&
            !java::FreeCamActive()&&!java::FreeLookAngles(yaw,pitch)&&!scaffold::IsEnabled()&&
            !autocharge::IsWorking()&&!refill::IsWorking()&&!autoblock::OwnsItemUse()&&!autorod::OwnsItemUse()&&
            !velocity::IsDelaying()&&!antikb::IsDelaying()&&!backtrack::IsLagging();
    }
    bool SameWorld() const {
        auto self=java::PlayerEntity(),world=java::LevelEntity();
        return self&&world&&self.IsSame(player_)&&world.IsSame(world_);
    }
    bool Allowed(const java::EntityHandle& self,const java::EntityHandle& e) const {
        if(!e||e.IsSame(self)||!e.IsAlive()||e.IsSpectator())return false;
        unsigned types=0;if(!java::EntityTypes(e,types)||!(types&java::kTypeLiving))return false;
        if(types&java::kTypePlayer)return targetPlayer_->get()&&!teams::IsTeammate(self,e)&&!antibot::IsBot(e);
        return (targetMobs_->get()&&(types&java::kTypeMonster))||
               (targetAnimals_->get()&&(types&(java::kTypeAnimal|java::kTypeVillager)));
    }
    bool AimPoint(const java::EntityHandle& self,const java::EntityHandle& e,int slot,
                  rotation::Rotation& angle,double& distance) const {
        if(!Allowed(self,e)||!java::MaceCanReach(slot,e))return false;
        double reach=0;if(!java::MaceReach(slot,reach))return false;
        reach=std::min(reach,range_->get());
        java::Box box;rotation::Vec3 eye;rotation::Rotation view;
        if(!self.EyePosition(1,eye)||!e.BoundingBox(box)||!java::PlayerRotation(view.yaw,view.pitch))return false;
        const auto closest=rotation::ClosestPointOnAabb(box,eye);
        const auto delta=closest.Subtract(eye);distance=std::sqrt(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z);
        if(distance>reach)return false;
        const rotation::Vec3 center{(box.minX+box.maxX)*.5,(box.minY+box.maxY)*.5,(box.minZ+box.maxZ)*.5};
        // Aim inside the nearest surface rather than measuring feet distance.
        const auto inset=closest.Add((center.x-closest.x)*.08,(center.y-closest.y)*.08,(center.z-closest.z)*.08);
        for(const auto point:{inset,center}) {
            const auto candidate=rotation::Calculate(eye,point);
            if(std::abs(std::remainder(double(candidate.yaw-view.yaw),360.))>fov_->get()*.5)continue;
            const auto hit=rotation::InteractionRayCast(candidate,reach);
            if(hit.entity&&hit.entity.IsSame(e)){angle=candidate;return true;}
        }
        return false;
    }
    bool Select(const java::EntityHandle& self,int slot,java::EntityHandle& target,rotation::Rotation& angle) const {
        rotation::Vec3 p;if(!self.Position(p))return false;
        std::vector<java::EntityHandle> entities;
        if(!java::WorldEntities(self,{p.x-7,p.y-7,p.z-7,p.x+7,p.y+7,p.z+7},entities))return false;
        double best=1e9;
        for(auto& entity:entities) {
            double d=0;rotation::Rotation candidate;
            if(AimPoint(self,entity,slot,candidate,d)&&d<best){best=d;target=std::move(entity);angle=candidate;}
        }
        return bool(target);
    }
    bool SlotIntact() const {
        int selected=-1;
        return java::OwnsPhysicalSlot(this)&&java::SelectedSlot(selected)&&selected==heldSlot_&&
            java::MaceSlotAvailable(maceSlot_)&&java::MaceSlotMatches(maceSlot_,maceIdentity_)&&
            (axeSlot_<0||java::MaceSlotMatches(axeSlot_,axeIdentity_))&&
            java::MaceSlotMatches(oldSlot_,oldIdentity_);
    }
    void Finish(bool restore=true) {
        if(restore&&equipped_&&SameWorld()&&SlotIntact()) {
            if(java::SetOwnedPhysicalSlot(this,oldSlot_))java::SyncSelectedItem();
        }
        java::ReleaseSilentSlot(this);rotation::ReleaseExclusiveRotations(this);rotation::ReleaseAttack(this);
        phase_=Phase::Idle;working=false;equipped_=false;age_=0;oldSlot_=maceSlot_=axeSlot_=heldSlot_=targetId_=-1;
        target_.Reset();player_.Reset();world_.Reset();oldIdentity_.Reset();maceIdentity_.Reset();axeIdentity_.Reset();
    }
    void Abort(const char* reason) {
        // Do not repeatedly reacquire the same failed window and starve MLG.
        LogTo("AutoMace: yield to MLG reason=%s age=%d",reason,age_);
        spentFall_=true;Finish();
    }
    bool Equip(int slot) {
        if(!SlotIntact())return false;
        if(heldSlot_!=slot) {
            if(!java::SetOwnedPhysicalSlot(this,slot))return false;
            heldSlot_=slot;
            equipped_=true;
        }
        return java::SyncSelectedItem();
    }
    bool Aim(const rotation::Rotation& wanted) {
        const auto from=rotation::GetRotation();
        auto next=rotation::LinearStep(from,wanted,speed_->get());double sensitivity=0;
        if(java::PlayerSensitivity(sensitivity))next=rotation::ApplySensitivityPatch(next,from,sensitivity);
        return rotation::SetExclusiveRotations(this,next);
    }
    void Tick() {
        if(reset_.exchange(false)){Finish();spentFall_=false;}
        if(phase_==Phase::Restore){Finish();return;} // Restore on the next game tick.
        if(!enabled_)return;
        java::MaceState state;java::EntityHandle self,world;
        if(!java::ReadMaceState(state)||!Context(self,world)){
            if(phase_!=Phase::Idle)Abort("context unavailable");
            return;
        }
        if(phase_!=Phase::Idle&&(!self.IsSame(player_)||!world.IsSame(world_))){Finish(false);spentFall_=false;return;}
        if(state.ground||state.gliding||state.obstructed||state.fall<=.1){spentFall_=false;Finish();return;}
        if(spentFall_||state.verticalSpeed>=0||state.interacting){Finish();return;}
        if(phase_==Phase::Idle) {
            // Normal prepares on a reachable target; Quick waits for smash eligibility.
            if(mode_->get()==0&&!Smash(state))return;
            int slot=-1;for(int i=0;i<9;++i)if(java::MaceSlotAvailable(i)){slot=i;break;}
            if(slot<0)return;
            java::EntityHandle candidate;rotation::Rotation desired;
            if(!Select(self,slot,candidate,desired))return;
            // Acquire first: BeginAttackSubmission cannot grant ownership itself.
            if(!rotation::AcquireAttack(this))return;
            if(!automlg::YieldForMace()||!java::AcquirePhysicalSlot(this)){Finish();return;}
            player_=std::move(self);world_=std::move(world);target_=std::move(candidate);maceSlot_=slot;
            if(!java::SelectedSlot(oldSlot_)||!java::EntityId(target_,targetId_)){Finish();return;}
            heldSlot_=oldSlot_;
            oldIdentity_=java::MaceSlotIdentity(oldSlot_);maceIdentity_=java::MaceSlotIdentity(maceSlot_);
            if(!oldIdentity_||!maceIdentity_||!Aim(desired)){Finish();return;}
            phase_=Phase::Aiming;working=true;age_=1;
            LogTo("AutoMace: prepare mode=%d target=%d fall=%.2f slot=%d",mode_->get(),targetId_,state.fall,maceSlot_);
        } else {
            auto live=java::EntityById(targetId_);rotation::Rotation desired;double d=0;
            if(!live||!live.IsSame(target_)||!SlotIntact()||!AimPoint(self,live,maceSlot_,desired,d)){Abort("target/slot unavailable");return;}
            // The axe grants one follow-up tick, never a second attack in its tick.
            const bool followUp=phase_==Phase::MaceFollowUp;
            if(followUp&&(mode_->get()!=0||!axe_->get())){Abort("axe option changed");return;}
            if(++age_>(followUp?1:3)||!rotation::AcquireAttack(this)||!Aim(desired)){Abort("aim window expired/busy");return;}
        }
        if(mode_->get()==1&&!equipped_&&!Equip(maceSlot_))Abort("normal switch failed");
    }
    void Interact() {
        if(phase_!=Phase::Aiming&&phase_!=Phase::MaceFollowUp)return;
        java::MaceState s;java::EntityHandle self,world;
        if(!enabled_||reset_||!Context(self,world)||!SameWorld()||!SlotIntact()||!java::ReadMaceState(s)) {Abort("interaction context");return;}
        if(!Smash(s))return;
        auto live=java::EntityById(targetId_);
        if(!live||!live.IsSame(target_)||!Allowed(self,live)||!java::MaceCanReach(maceSlot_,live)){Abort("target left reach");return;}
        double reach=0;if(!java::MaceReach(maceSlot_,reach)){Abort("reach unavailable");return;}
        const auto hit=rotation::InteractionRayCast(rotation::GetRotation(),std::min(reach,range_->get()));
        if(!hit.entity||!hit.entity.IsSame(live))return;
        if(!rotation::CanSubmitAttack(this))return;
        int attackSlot=maceSlot_;
        if(phase_==Phase::Aiming&&mode_->get()==0&&axe_->get()&&java::MaceTargetBlockingShield(live)) {
            for(int i=0;i<9;++i) {
                if(!java::MaceAxeSlotAvailable(i)||!java::MaceCanReach(i,live))continue;
                double axeReach=0;if(!java::MaceReach(i,axeReach))continue;
                const auto axeHit=rotation::InteractionRayCast(rotation::GetRotation(),std::min(axeReach,range_->get()));
                if(!axeHit.entity||!axeHit.entity.IsSame(live))continue;
                auto identity=java::MaceSlotIdentity(i);if(!identity)continue;
                axeSlot_=attackSlot=i;axeIdentity_=std::move(identity);break;
            }
        }
        if(!Equip(attackSlot)){Abort("attack switch failed");return;}
        if(!rotation::BeginAttackSubmission(this))return;
        const bool sent=java::AttackAndSwing(live);
        const bool entered=rotation::EndAttackSubmission();
        if(attackSlot==axeSlot_&&sent&&entered) {
            phase_=Phase::MaceFollowUp;age_=0;
            LogTo("AutoMace: axe attack target=%d slot=%d; mace next tick",targetId_,axeSlot_);
            return;
        }
        // A JNI exception after controller entry cannot prove no packet was sent.
        spentFall_=true;phase_=Phase::Restore;
        LogTo("AutoMace: attack target=%d sent=%d native=%d fall=%.2f",targetId_,int(sent),int(entered),s.fall);
    }
    Properties::ModeProperty* mode_{};
    Properties::NumberProp *range_{},*fov_{},*speed_{};
    Properties::BoolProperty *targetPlayer_{},*targetMobs_{},*targetAnimals_{},*axe_{};
    std::atomic<bool> enabled_{false},reset_{false};
    event::Subscription tick_,interact_,respawn_;
    Phase phase_=Phase::Idle;
    bool spentFall_=false,equipped_=false;
    int age_=0,oldSlot_=-1,maceSlot_=-1,axeSlot_=-1,heldSlot_=-1,targetId_=-1;
    java::EntityHandle player_,world_,target_,oldIdentity_,maceIdentity_,axeIdentity_;
};
bool IsWorking(){return working.load();}
}
namespace nova::module {
std::unique_ptr<Module> MakeAutoMaceModule(){return std::make_unique<automace::AutoMace>();}
}
