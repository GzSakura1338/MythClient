#include "minecraft/modules/Module.h"
#include "core/RuntimeControl.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_anchor.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_freelook.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/modules/combat/shared/BlockAim.h"
#include "minecraft/modules/combat/shared/HotbarSession.h"
#include "minecraft/modules/combat/shared/ExplosionCoordinator.h"
#include "minecraft/modules/combat/legitcrystalaura/CrystalRules.h"
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
#include "minecraft/modules/misc/teams/Teams.h"
#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/PlacementWire.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include "ui/clickgui/ClickGui.h"
#include "ui/notifications/Notifications.h"
#include "utils/Keyboard.h"
#include <atomic>
#include <chrono>
#include <format>
#include <vector>

namespace nova::module::anchoraura {
using namespace explosive;
using Vec=rotation::Vec3;
using Action=java::AnchorAction;
using Item=java::AnchorItem;
static std::int64_t Now(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}

class AnchorAura final:public Module {
public:
    AnchorAura():Module("AnchorAura",Category::Combat,"General","General"),slots_(this){
        enableOverlay();
        const auto integer=[](double v){return std::format("{:.0f}",v);};
        const auto decimal=[](double v){return std::format("{:.1f}",v);};
        auto range=subNumber("Range",1,6,.1,4.5,decimal),walls=subBool("Through Walls",false);
        range_=N(range);walls_=B(walls);addSettings({Properties::PropDesc("Target",nullptr,{range,walls})});
        auto delay=subNumber("Delay",0,1000,5,0,integer);delay_=N(delay);addSettings({delay});
        silent_=&addBool("Silent",false);swing_=&addBool("Swing",true);
        auto speed=subNumber("Speed",1,180,1,90,integer);speed_=N(speed);
        addSettings({Properties::PropDesc("Rotation",nullptr,{speed})});
        auto minimum=subNumber("Min Damage",.5,36,.5,4,decimal),maximum=subNumber("Max Self Damage",0,36,.5,6,decimal);
        minimum_=N(minimum);maximum_=N(maximum);addSettings({Properties::PropDesc("Safety",nullptr,{minimum,maximum})});
        esp_=&addBool("Place ESP",true);
        auto& bus=event::EventBus::Get();
        begin_=bus.Subscribe<event::GameTickEvent>([this](auto& e){if(e.type==event::EventType::Pre)Begin();},event::Priority::Highest-2);
        prepare_=bus.Subscribe<event::GameTickEvent>([this](auto& e){if(e.type==event::EventType::Pre)Prepare();},event::Priority::Highest);
        aim_=bus.Subscribe<event::GameTickEvent>([this](auto& e){if(e.type==event::EventType::Pre)Aim();},event::Priority::Highest+2);
        interact_=bus.Subscribe<event::PrePlayerInteractEvent>([this](auto&){Use();},event::Priority::High+1);
        respawn_=bus.Subscribe<event::RespawnEvent>([this](auto&){reset_=true;espView_.Clear();},event::Priority::Highest);
    }
    ~AnchorAura()override{
        begin_.Reset();prepare_.Reset();aim_.Reset();interact_.Reset();respawn_.Reset();
        Release();if(enabled_)anchorAuraEnabled=false;
    }
private:
    static Properties::NumberProp* N(const Properties::PropDesc& p){return static_cast<Properties::NumberProp*>(p.getProp());}
    static Properties::BoolProperty* B(const Properties::PropDesc& p){return static_cast<Properties::BoolProperty*>(p.getProp());}
    struct Plan{
        java::BlockPos pos{};java::BlockHitResultInfo hit{};rotation::Rotation angle{};
        Action action=Action::Place;int slot=-1,target=-1;double damage=0;bool existing=false,offhand=false;
    };
    struct DamageEntry{int id;java::BlockPos pos;double damage;};
    void onEnable()override{enabled_=anchorAuraEnabled=true;reset_=true;}
    void onDisable()override{enabled_=anchorAuraEnabled=false;reset_=true;espView_.Clear();}
    void onRender()override{if(enabled_&&esp_->get())espView_.Draw();}
    void Release(){rotation::ReleaseExclusiveRotations(this);rotation::ReleaseAttack(this);}
    void Clear(){
        slots_.Restore();Release();pending_=have_=ready_=candidate_=bestValid_=false;
        grid_.clear();world_.Reset();player_.Reset();espView_.Clear();nextUse_=retryAt_=0;
    }
    void Begin(){
        const bool reset=reset_.exchange(false);
        if(!enabled_){if(reset){Clear();ResetAnchor();anchorConfirming=false;}return;}
        ResetAnchor();Release();ready_=used_=false;damageCache_.clear();stage_="idle";
        for(auto& slot:slotCache_)slot=-2;
        if(reset)Clear();anchorConfirming=pending_;
        if(slots_.Active()&&!slots_.Restore())used_=true;
        if(!slots_.Prepare(silent_->get())){slots_.Restore();used_=true;}
        if(!java::CrystalSupported()){
            notifications::Post("AnchorAura: requires Lunar 1.21.11 or Minecraft 26.3",notifications::Type::Warning);setEnabled(false);
        }
    }
    bool Context(java::EntityHandle& self){
        self=java::PlayerEntity();auto world=java::LevelEntity();float y=0,p=0;
        if(!enabled_||reset_||!self||!world||!self.IsAlive()||self.IsSpectator())return false;
        if(world_&&(!world.IsSame(world_)||!self.IsSame(player_))){Clear();return false;}
        if(!world_){world_=java::LevelEntity();player_=java::PlayerEntity();}
        return input::IsProcessForeground()&&!java::ScreenOpen()&&!clickgui::IsOpen()&&!java::FreeCamActive()&&!java::FreeLookAngles(y,p)&&
            java::CrystalContextReady()&&!java::KeyShiftDown()&&!scaffold::IsEnabled()&&!automace::IsWorking()&&!automlg::IsWorking()&&
            !autocharge::IsWorking()&&!refill::IsWorking()&&!autoblock::OwnsItemUse()&&!autorod::OwnsItemUse()&&
            !velocity::IsDelaying()&&!antikb::IsDelaying()&&!backtrack::IsLagging();
    }
    bool Allowed(const java::EntityHandle& self,const java::EntityHandle& e){
        if(!e||e.IsSame(self)||!e.IsPlayer()||!e.IsAlive()||e.IsSpectator()||teams::IsTeammate(self,e)||antibot::IsBot(e))return false;
        bool creative=false;Vec a,b;
        return java::CrystalCreative(e,creative)&&!creative&&self.Position(a)&&e.Position(b)&&a.DistanceTo(b)<=12;
    }
    int Slot(Item kind){
        auto& cached=slotCache_[int(kind)];if(cached!=-2)return cached;
        int selected=-1;cached=-1;if(!java::EffectiveSelectedSlot(selected))return cached;
        if(java::AnchorSlot(selected,kind))return cached=selected;
        for(int i=0;i<9;++i)if(java::AnchorSlot(i,kind))return cached=i;return cached;
    }
    // Mio X8, nearest reachable interior corner. The actual ray supplies the
    // side/hit vector; no synthetic air/liquid placement or fabricated side.
    bool Point(const java::EntityHandle& self,const java::BlockPos& p,double reach,int face,rotation::Rotation& angle,java::BlockHitResultInfo& hit){
        Vec eye;if(!self.EyePosition(1,eye))return false;
        std::vector<Vec> points;points.reserve(8);
        for(double x:{.05,.95})for(double y:{.05,.95})for(double z:{.05,.95})points.push_back({p.x+x,p.y+y,p.z+z});
        std::stable_sort(points.begin(),points.end(),[&](auto a,auto b){return eye.DistanceTo(a)<eye.DistanceTo(b);});
        const auto current=rotation::GetRotation();const bool change=!rotation::ReadPlacementWire().Ready();
        for(auto point:points){
            auto a=rotation::Calculate(eye,point);
            if(change&&std::abs(std::remainder(double(a.yaw-current.yaw),360.))<.0001&&std::abs(a.pitch-current.pitch)<.0001)continue;
            if(BlockHit(self,p,a,reach,walls_->get(),hit)&&(face<0||hit.direction==face)){angle=a;return true;}
        }
        // Center fallback is in Mio's getVec3d2871; still validate it here.
        auto a=rotation::Calculate(eye,Center(p));
        if(BlockHit(self,p,a,reach,walls_->get(),hit)&&(face<0||hit.direction==face)){angle=a;return true;}
        return false;
    }
    bool Geometry(const java::EntityHandle& self,const java::BlockPos& p,Plan& out){
        java::AnchorState state;Vec eye;double reach=0;bool explodes=false;
        if(!java::ReadAnchor(p,state)||(!state.anchor&&!state.replaceable)||!self.EyePosition(1,eye)||!java::CrystalBlockReach(reach))return false;
        reach=std::min(reach,range_->get());if(eye.DistanceTo(Center(p))>reach)return false;
        if(!java::AnchorExplodes(p,explodes)||!explodes||(Now()<retryAt_&&SameBlock(p,failed_)))return false;
        out={};out.pos=p;out.existing=state.anchor;
        if(state.anchor){
            out.action=state.charges?Action::Detonate:Action::Charge;out.slot=Slot(state.charges?Item::Detonator:Item::Glowstone);
            return out.slot>=0&&(state.charges||Slot(Item::Detonator)>=0)&&Point(self,p,reach,-1,out.angle,out.hit);
        }
        out.slot=Slot(Item::Anchor);out.offhand=out.slot<0&&java::AnchorOffhand();
        if(out.offhand&&!java::EffectiveSelectedSlot(out.slot))return false;
        if(out.slot<0||Slot(Item::Glowstone)<0||Slot(Item::Detonator)<0||!java::AnchorBuildable(p))return false;
        // PhaseESPSearchHelper4_2.directionArr: DOWN,NORTH,EAST,SOUTH,WEST,UP.
        const java::BlockPos offsets[]={{0,-1,0},{0,0,-1},{1,0,0},{0,0,1},{-1,0,0},{0,1,0}};
        const int faces[]={1,3,4,2,5,0};
        for(int i=0;i<6;++i){const auto d=offsets[i];java::BlockPos support{p.x+d.x,p.y+d.y,p.z+d.z};
            if(java::CrystalBaseSupport(support)&&Point(self,support,reach,faces[i],out.angle,out.hit))return true;
        }return false;
    }
    bool Damage(const java::EntityHandle& e,const java::BlockPos& p,double& damage){
        int id=-1;if(!java::EntityId(e,id))return false;
        for(auto& entry:damageCache_)if(entry.id==id&&SameBlock(entry.pos,p)){damage=entry.damage;return true;}
        Vec at;java::Box box;java::CrystalDefense defense;double exposure=0;
        if(!e.Position(at)||!e.BoundingBox(box)||!java::ReadCrystalDefense(e,defense)||!java::MioAnchorExposure(e,p,box,exposure))return false;
        damage=crystal::Damage(at.DistanceTo(Center(p)),exposure,defense.difficulty,defense.armor,defense.toughness,defense.resistance,defense.protection,5);
        if(!std::isfinite(damage))return false;damageCache_.push_back({id,p,damage});return true;
    }
    bool Safe(const java::EntityHandle& self,const Plan& plan){
        auto target=java::EntityById(plan.target);java::CrystalDefense enemy,own;double damage=0;
        if(!java::ReadCrystalDefense(target,enemy)||!java::ReadCrystalDefense(self,own)||!Damage(self,plan.pos,damage))return false;
        // Mio is2874: lethal target bypasses maxSelf, never antiSuicide.
        if(damage>=own.health+own.absorption||(plan.damage<enemy.health+enemy.absorption&&damage>=maximum_->get()))return false;
        return true;
    }
    bool Refresh(const java::EntityHandle& self,Plan& plan){
        Plan next;if(!Geometry(self,plan.pos,next))return false;
        std::vector<java::EntityHandle> players;if(!java::WorldPlayers(players))return false;
        for(const auto& e:players){double damage=0;
            if(Allowed(self,e)&&Damage(e,plan.pos,damage)&&damage>=minimum_->get()&&damage>=.5&&damage>next.damage){
                next.damage=damage;java::EntityId(e,next.target);
            }
        }
        if(next.target<0||!Safe(self,next))return false;plan=next;return true;
    }
    void Search(const java::EntityHandle& self){
        Vec eye,feet;if(!self.EyePosition(1,eye)||!self.Position(feet))return;
        if(grid_.empty()||eye.DistanceTo(searchEye_)>range_->get()||searchRange_!=range_->get()||searchWalls_!=walls_->get()){
            grid_.clear();cursor_=0;candidate_=bestValid_=false;searchEye_=eye;searchRange_=range_->get();searchWalls_=walls_->get();
            for(int x=-int(std::floor(searchRange_));x<int(std::ceil(searchRange_));++x)
            for(int y=-int(std::floor(searchRange_));y<int(std::ceil(searchRange_));++y)
            for(int z=-int(std::floor(searchRange_));z<int(std::ceil(searchRange_));++z){
                java::BlockPos p{int(std::floor(feet.x+x)),int(std::floor(eye.y+y)),int(std::floor(feet.z+z))};
                if(eye.DistanceTo(Center(p))<=searchRange_)grid_.push_back(p);
            }
        }
        const auto start=std::chrono::steady_clock::now();int cells=0,details=0;
        while(cursor_<grid_.size()&&cells<128&&details<8){
            if(cells&&std::chrono::steady_clock::now()-start>std::chrono::milliseconds(2))break;
            if(!candidate_){
                ++cells;if(!Geometry(self,grid_[cursor_],candidatePlan_)){++cursor_;continue;}
                std::vector<java::EntityHandle> players;if(!java::WorldPlayers(players)){++cursor_;continue;}
                targets_.clear();targetCursor_=0;
                for(const auto& e:players){int id=-1;if(Allowed(self,e)&&java::EntityId(e,id))targets_.push_back(id);}
                candidate_=true;
            }
            while(targetCursor_<targets_.size()&&details<8){
                auto e=java::EntityById(targets_[targetCursor_++]);double damage=0;++details;
                if(Allowed(self,e)&&Damage(e,candidatePlan_.pos,damage)&&damage>=minimum_->get()&&damage>=.5&&damage>candidatePlan_.damage){
                    candidatePlan_.damage=damage;java::EntityId(e,candidatePlan_.target);
                }
                if(std::chrono::steady_clock::now()-start>std::chrono::milliseconds(2))break;
            }
            if(targetCursor_<targets_.size())break;
            candidate_=false;++cursor_;
            if(candidatePlan_.target<0||!Safe(self,candidatePlan_))continue;
            if(candidatePlan_.existing&&Refresh(self,candidatePlan_)){plan_=candidatePlan_;have_=true;grid_.clear();return;}
            if(!bestValid_||candidatePlan_.damage>=best_.damage){best_=candidatePlan_;bestValid_=true;}
        }
        if(cursor_==grid_.size()){
            if(bestValid_&&Refresh(self,best_)){plan_=best_;have_=true;}
            grid_.clear();bestValid_=candidate_=false;
        }
    }
    void Prepare(){
        if(!enabled_)return;stage_="context";java::EntityHandle self;
        if(used_||!Context(self)){slots_.Restore();espView_.Clear();return;}
        Vec at;bool explodes=false;if(!self.Position(at)||!java::AnchorExplodes({int(std::floor(at.x)),int(std::floor(at.y)),int(std::floor(at.z))},explodes))return;
        if(!explodes){setEnabled(false);slots_.Restore();notifications::Post("AnchorAura: anchors do not explode here",notifications::Type::Warning);return;}
        if(pending_){
            stage_="confirmation";java::AnchorState state;if(!java::ReadAnchor(sent_.pos,state))return;
            const bool confirmed=sent_.action==Action::Place?state.anchor:sent_.action==Action::Charge?state.anchor&&state.charges>0:!state.anchor;
            if(!confirmed&&Now()<deadline_){
                auto e=java::EntityById(sent_.target);double damage=0;
                if(Allowed(self,e)&&Damage(e,sent_.pos,damage)&&damage>=minimum_->get()){
                    auto held=sent_;held.damage=damage;
                    if(Safe(self,held)){plan_=held;ready_=true;OfferAnchor(damage,true);espView_.Show(plan_.pos);}
                }return;
            }
            pending_=anchorConfirming=false;
            if(!confirmed){failed_=sent_.pos;retryAt_=Now()+500;have_=false;grid_.clear();}
            // Keep the location after a confirmed explosion, as Mio sequential
            // does, but re-evaluate target, stock, damage and support before reuse.
        }
        stage_="search";
        if(have_&&!Refresh(self,plan_))have_=false;
        if(!have_)Search(self);
        if(!have_){slots_.Restore();espView_.Clear();return;}
        ready_=true;stage_="ready";OfferAnchor(plan_.damage,false);espView_.Show(plan_.pos);
    }
    void Aim(){
        if(!enabled_||!ready_||!ChooseAnchor()){if(enabled_)slots_.Restore();return;}
        java::EntityHandle self;if(!Context(self))return;
        const auto from=rotation::GetRotation(),wanted=pending_?sentAngle_:plan_.angle;
        const double d=std::hypot(std::remainder(double(wanted.yaw-from.yaw),360.),double(wanted.pitch-from.pitch));
        auto next=rotation::LinearStep(from,wanted,std::min(speed_->get(),d>1?d*.5:d));double sensitivity=0;
        if(java::PlayerSensitivity(sensitivity))next=rotation::ApplySensitivityPatch(next,from,sensitivity);
        anchorWorking=rotation::SetExclusiveRotations(this,next);stage_=anchorWorking?"aim":"rotation-busy";
    }
    void Use(){
        struct Finish{AnchorAura* self;~Finish(){self->Report();}} finish{this};
        if(!enabled_||used_)return;used_=true;
        if(!ready_||!anchorWorking||pending_||!rotation::OwnsExclusiveRotations(this))return;
        stage_="interaction-busy";java::EntityHandle self;if(!Context(self)||!rotation::CanSubmitItemUse())return;
        stage_="delay";if(Now()<nextUse_)return;
        stage_="recheck";Plan checked=plan_;damageCache_.clear();
        if(!Refresh(self,checked)||checked.action!=plan_.action)return;
        stage_="rotation-history";if(!rotation::ReadPlacementWire().Ready())return;
        stage_="ray";java::BlockHitResultInfo hit;double reach=0;
        if(!java::CrystalBlockReach(reach)||!BlockHit(self,checked.hit.blockPos,rotation::GetRotation(),std::min(reach,range_->get()),walls_->get(),hit)||
            (checked.action==Action::Place&&hit.direction!=checked.hit.direction))return;
        stage_="equip";if(!slots_.Equip(checked.slot,silent_->get()))return;
        if(!rotation::ReadPlacementWire().Ready()){slots_.Restore();return;}
        bool entered=false;
        const bool accepted=checked.offhand?java::UseOffhandAnchor(checked.pos,hit,checked.slot,this,entered):
            java::UseAnchor(checked.pos,hit,checked.slot,checked.action,this,entered);
        stage_=entered?"submitted":"native-rejected";
        if(accepted)java::SwingCrystalHand(checked.offhand,swing_->get());
        if(entered){
            sent_=checked;sentAngle_=rotation::GetRotation();pending_=anchorConfirming=true;deadline_=Now()+500;
            nextUse_=Now()+std::int64_t(delay_->get());
        }
        // Mio swaps back after each operation. The native lease preserves
        // manual scrolling and retries a failed slot synchronization next tick.
        slots_.Restore();
    }
    void Report(){
        if(!enabled_||Now()<nextReport_)return;nextReport_=Now()+1000;
        const auto wire=rotation::ReadPlacementWire();
        LogTo("AnchorAura: stage=%s plan=%d action=%d pending=%d search=%zu/%zu damage=%.2f slots=%d,%d,%d wire=%d delta=%.6f placed=%.6f",
            stage_,int(have_),int(plan_.action),int(pending_),cursor_,grid_.size(),plan_.damage,slotCache_[0],slotCache_[1],slotCache_[2],int(wire.Ready()),wire.delta,wire.placedDelta);
    }
    Properties::NumberProp *range_{},*delay_{},*speed_{},*minimum_{},*maximum_{};
    Properties::BoolProperty *walls_{},*silent_{},*swing_{},*esp_{};
    event::Subscription begin_,prepare_,aim_,interact_,respawn_;
    std::atomic<bool> enabled_{false},reset_{false};
    HotbarSession slots_;crystal::PlacementEsp espView_;java::EntityHandle world_,player_;
    Plan plan_,sent_,best_,candidatePlan_;rotation::Rotation sentAngle_{};java::BlockPos failed_{};Vec searchEye_{};
    std::vector<java::BlockPos> grid_;std::vector<int> targets_;std::vector<DamageEntry> damageCache_;
    size_t cursor_=0,targetCursor_=0;double searchRange_=0;bool searchWalls_=false;
    bool have_=false,ready_=false,pending_=false,used_=false,candidate_=false,bestValid_=false;
    int slotCache_[3]{-2,-2,-2};std::int64_t nextUse_=0,deadline_=0,retryAt_=0,nextReport_=0;const char* stage_="idle";
};
}
namespace nova::module {std::unique_ptr<Module> MakeAnchorAuraModule(){return std::make_unique<anchoraura::AnchorAura>();}}
