#include "minecraft/modules/Module.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_blockplace.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_freelook.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/modules/combat/feetplace/PlacementPlan.h"
#include "minecraft/modules/combat/shared/BlockAim.h"
#include "minecraft/modules/combat/shared/HotbarSession.h"
#include "minecraft/modules/combat/shared/ExplosionCoordinator.h"
#include "minecraft/modules/combat/legitcrystalaura/PlacementPrediction.h"
#include "minecraft/modules/combat/legitcrystalaura/PlacementEsp.h"
#include "minecraft/modules/combat/automace/AutoMace.h"
#include "minecraft/modules/combat/autoblock/AutoBlockRuntime.h"
#include "minecraft/modules/combat/autorod/AutoRod.h"
#include "minecraft/modules/combat/velocity/Velocity.h"
#include "minecraft/modules/combat/antikb/AntiKB.h"
#include "minecraft/modules/combat/backtrack/Backtrack.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/modules/player/refill/Refill.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include "ui/clickgui/ClickGui.h"
#include "ui/notifications/Notifications.h"
#include "utils/Keyboard.h"
#include <atomic>
#include <chrono>
#include <format>

namespace nova::module::feetplace {
using namespace explosive;
using Vec=rotation::Vec3;
static std::int64_t Now(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
class FeetPlace final:public Module {
public:
    FeetPlace():Module("FeetPlace",Category::Combat,"General","General"),slots_(this){
        enableOverlay();
        auto integer=[](double v){return std::format("{:.0f}",v);};
        auto delay=subNumber("Place Delay",0,250,5,50,integer);delay_=static_cast<Properties::NumberProp*>(delay.getProp());addSettings({delay});
        silent_=&addBool("Silent",true);swing_=&addBool("Swing",false);
        auto speed=subNumber("Speed",1,180,1,45,integer);speed_=static_cast<Properties::NumberProp*>(speed.getProp());
        addSettings({Properties::PropDesc("Rotation",nullptr,{speed})});
        autoDisable_=&addBool("Auto Disable",true);esp_=&addBool("Place ESP",true);
        auto& bus=event::EventBus::Get();
        begin_=bus.Subscribe<event::GameTickEvent>([this](auto& e){if(e.type==event::EventType::Pre)Begin();},event::Priority::Highest-4);
        prepare_=bus.Subscribe<event::GameTickEvent>([this](auto& e){if(e.type==event::EventType::Pre)Prepare();},event::Priority::Highest-1);
        aim_=bus.Subscribe<event::GameTickEvent>([this](auto& e){if(e.type==event::EventType::Pre)Aim();},event::Priority::Highest+3);
        use_=bus.Subscribe<event::PrePlayerInteractEvent>([this](auto&){Use();},event::Priority::High-1);
        motion_=bus.Subscribe<event::MotionEvent>([this](auto& e){if(e.type==event::EventType::Post)turn_.Observe({e.yaw,e.pitch});},event::Priority::Lowest);
        respawn_=bus.Subscribe<event::RespawnEvent>([this](auto&){reset_=true;espView_.Clear();},event::Priority::Highest);
    }
    ~FeetPlace()override{begin_.Reset();prepare_.Reset();aim_.Reset();use_.Reset();motion_.Reset();respawn_.Reset();Release();}
private:
    struct Plan{Pos pos;java::BlockHitResultInfo hit;rotation::Rotation angle;};
    struct Retry{Pos pos;std::int64_t until;};
    void Release(){rotation::ReleaseExclusiveRotations(this);rotation::ReleaseAttack(this);feetWorking=false;}
    void Pause(){Release();slots_.Restore();ready_=false;espView_.Clear();}
    void Clear(){Pause();pending_=have_=false;footprint_.clear();goals_.clear();retries_.clear();cursor_=0;world_.Reset();player_.Reset();turn_.Reset();nextUse_=nextScan_=0;}
    void onEnable()override{enabled_=true;reset_=true;}
    void onDisable()override{enabled_=false;reset_=true;feetWorking=false;espView_.Clear();}
    void onRender()override{if(enabled_&&esp_->get())espView_.Draw();}
    void Begin(){
        Release();ready_=used_=false;
        if(reset_.exchange(false))Clear();
        if(!slots_.Prepare(silent_->get())){used_=true;have_=false;}
        if(!enabled_){slots_.Restore();return;}
        if(!java::CrystalSupported()){
            notifications::Post("FeetPlace: requires Lunar 1.21.11 or Minecraft 26.3",notifications::Type::Warning);setEnabled(false);return;
        }
        turn_.Seed(rotation::GetRotation());
    }
    bool Context(java::EntityHandle& self){
        self=java::PlayerEntity();auto world=java::LevelEntity();float yaw=0,pitch=0;
        if(!enabled_||reset_||!self||!world||!self.IsAlive()||self.IsSpectator())return false;
        if(world_&&(!world.IsSame(world_)||!self.IsSame(player_))){Clear();return false;}
        if(!world_){world_=java::LevelEntity();player_=java::PlayerEntity();}
        return input::IsProcessForeground()&&!java::ScreenOpen()&&!clickgui::IsOpen()&&!java::FreeCamActive()&&
            !java::FreeLookAngles(yaw,pitch)&&java::ObsidianContextReady()&&!scaffold::IsEnabled()&&
            !automace::IsWorking()&&!automlg::IsWorking()&&!autocharge::IsWorking()&&!refill::IsWorking()&&
            !autoblock::OwnsItemUse()&&!autorod::OwnsItemUse()&&!velocity::IsDelaying()&&!antikb::IsDelaying()&&!backtrack::IsLagging();
    }
    int Slot(){int slot=-1;if(!java::EffectiveSelectedSlot(slot))return -1;
        if(java::CrystalObsidianSlot(slot))return slot;
        for(int i=0;i<9;++i)if(java::CrystalObsidianSlot(i))return i;return -1;}
    bool Cooling(Pos p){for(const auto& r:retries_)if(Same(r.pos,p)&&Now()<r.until)return true;return false;}
    bool Evaluate(const java::EntityHandle& self,Pos p,Plan& out){
        double reach=0;if(Cooling(p)||!java::CrystalBlockReach(reach)||!java::ObsidianBuildable(p))return false;
        out.pos=p;return AimPlacement(self,p,reach,false,rotation::GetRotation(),out.angle,out.hit,!turn_.Ready(rotation::GetRotation()));
    }
    void Rebuild(const java::EntityHandle& self,const std::vector<Pos>& cells){
        goals_=Ring(cells);cursor_=0;
        // Bound Mio's occupied-cell expansion; only living collisions expand the ring.
        java::Box box;if(!self.BoundingBox(box))return;
        std::vector<java::EntityHandle> entities;
        if(java::WorldEntities(self,{box.minX-4,box.minY-1,box.minZ-4,box.maxX+4,box.maxY+2,box.maxZ+4},entities)){
            std::vector<java::Box> living;
            for(const auto& e:entities){unsigned types=0;java::Box b;
                if(e.IsAlive()&&!e.IsSpectator()&&java::EntityTypes(e,types)&&(types&java::kTypeLiving)&&e.BoundingBox(b))living.push_back(b);}
            for(size_t i=0;i<goals_.size()&&goals_.size()<32;++i){
                auto p=goals_[i];if(p.y!=cells.front().y)continue;
                java::Box cube{double(p.x),double(p.y),double(p.z),double(p.x+1),double(p.y+1),double(p.z+1)};
                if(std::any_of(living.begin(),living.end(),[&](auto b){return crystal::Overlaps(cube,b);}))
                    for(auto d:Sides){auto n=Offset(p,d);if(!Contains(cells,n))Add(goals_,n);}
            }
        }
        Vec eye;if(!self.EyePosition(1,eye))return;const auto current=rotation::GetRotation();
        std::stable_sort(goals_.begin(),goals_.end(),[&](Pos a,Pos b){
            auto score=[&](Pos p){const auto angle=rotation::Calculate(eye,Center(p));return std::hypot(std::remainder(double(angle.yaw-current.yaw),360.),double(angle.pitch-current.pitch));};
            const double da=score(a),db=score(b);return std::abs(da-db)>1e-6?da<db:eye.DistanceTo(Center(a))<eye.DistanceTo(Center(b));
        });
    }
    void Prepare(){
        java::EntityHandle self;if(used_||!Context(self)){Pause();return;}
        java::Box box;bool ground=false;
        if(!self.BoundingBox(box)||!self.OnGround(ground)){Pause();return;}
        auto cells=Footprint(box);if(cells.empty()){Pause();return;}
        if(!footprint_.empty()&&autoDisable_->get()&&(java::KeyJumpDown()||!SameFootprint(cells,footprint_))){setEnabled(false);Clear();return;}
        if(!ground){Pause();return;}
        if(footprint_.empty()||!SameFootprint(cells,footprint_)){footprint_=cells;have_=pending_=false;goals_.clear();}
        if(pending_){
            bool placed=false;
            if(java::ReadObsidian(plan_.pos,placed)&&placed){pending_=have_=false;goals_.clear();nextScan_=0;}
            else if(Now()>=deadline_){
                retries_.push_back({plan_.pos,Now()+500});pending_=have_=false;goals_.clear();
            }else {espView_.Show(plan_.pos);return;}
        }
        std::erase_if(retries_,[](const auto& r){return Now()>=r.until;});
        if(anchorConfirming||crystalConfirming||autoCrystalConfirming||Now()<nextUse_){Pause();return;}
        slot_=Slot();if(slot_<0){Pause();if(Now()>=nextNotice_){nextNotice_=Now()+5000;notifications::Post("FeetPlace: no obsidian in hotbar",notifications::Type::Warning);}return;}
        if(have_&&!Evaluate(self,plan_.pos,plan_))have_=false;
        if(!have_){
            if(goals_.empty()){if(Now()<nextScan_){Pause();return;}Rebuild(self,cells);}
            const auto start=std::chrono::steady_clock::now();int queries=0;
            while(cursor_<goals_.size()&&queries<48){
                if(queries&&std::chrono::steady_clock::now()-start>std::chrono::milliseconds(2))break;
                auto goal=goals_[cursor_++];std::vector<Pos> visited;Pos first;
                auto buildable=[&](Pos p){Vec eye;double reach=0;
                    return ++queries<=48&&!Contains(cells,p)&&!Cooling(p)&&self.EyePosition(1,eye)&&java::CrystalBlockReach(reach)&&
                        eye.DistanceTo(Center(p))<=reach&&java::ObsidianBuildable(p);};
                auto aim=[&](Pos p){return Evaluate(self,p,plan_);};
                if(FirstStep(goal,2,visited,buildable,aim,first)){have_=true;break;}
            }
            if(cursor_>=goals_.size()){goals_.clear();nextScan_=Now()+100;}
        }
        if(!have_){Pause();return;}
        // Publish a defense offer before crystal/anchor planning, but acquire
        // rotation only after this tick's emergency modules have had their turn.
        ready_=feetWorking=true;
    }
    void Aim(){
        if(!ready_)return;
        java::EntityHandle self;if(!Context(self)){Pause();return;}
        const auto from=rotation::GetRotation();const auto wanted=plan_.angle;
        const double distance=std::hypot(std::remainder(double(wanted.yaw-from.yaw),360.),double(wanted.pitch-from.pitch));
        auto next=rotation::LinearStep(from,wanted,std::min(speed_->get(),distance>1?distance*.5:distance));double sensitivity=0;
        if(java::PlayerSensitivity(sensitivity))next=rotation::ApplySensitivityPatch(next,from,sensitivity);
        ready_=feetWorking=rotation::SetExclusiveRotations(this,next);
        if(ready_)espView_.Show(plan_.pos);else Pause();
    }
    void Use(){
        if(used_)return;used_=true;
        java::EntityHandle self;
        if(!ready_||!Context(self)||!rotation::OwnsExclusiveRotations(this)||!rotation::CanSubmitItemUse()||!turn_.Ready(rotation::GetRotation()))return;
        Plan checked;if(!Evaluate(self,plan_.pos,checked))return;
        double reach=0;java::BlockHitResultInfo hit;
        if(!java::CrystalBlockReach(reach)||!BlockHit(self,checked.hit.blockPos,rotation::GetRotation(),reach,false,hit)||hit.direction!=checked.hit.direction)return;
        slot_=Slot();if(slot_<0||!slots_.Equip(slot_,silent_->get()))return;
        bool entered=false;const bool accepted=java::PlaceObsidian(checked.pos,hit,slot_,this,entered);
        if(accepted){if(swing_->get())java::SwingMainHand();else java::SendSwingPacket();}
        if(entered){plan_=checked;pending_=true;deadline_=Now()+300;nextUse_=Now()+std::int64_t(delay_->get());turn_.Submitted();}
        else {retries_.push_back({plan_.pos,Now()+500});have_=false;Pause();}
    }
    Properties::NumberProp *delay_{},*speed_{};
    Properties::BoolProperty *silent_{},*swing_{},*autoDisable_{},*esp_{};
    std::atomic<bool> enabled_{false},reset_{false};
    event::Subscription begin_,prepare_,aim_,use_,motion_,respawn_;
    HotbarSession slots_;crystal::PlacementTurnGate turn_;crystal::PlacementEsp espView_;
    java::EntityHandle world_,player_;Plan plan_;
    std::vector<Pos> footprint_,goals_;std::vector<Retry> retries_;
    size_t cursor_=0;int slot_=-1;bool ready_=false,used_=false,have_=false,pending_=false;
    std::int64_t deadline_=0,nextUse_=0,nextScan_=0,nextNotice_=0;
};
}
namespace nova::module {std::unique_ptr<Module> MakeFeetPlaceModule(){return std::make_unique<feetplace::FeetPlace>();}}
