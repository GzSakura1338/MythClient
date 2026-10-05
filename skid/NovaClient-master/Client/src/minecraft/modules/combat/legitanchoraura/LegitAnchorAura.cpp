#include "minecraft/modules/Module.h"
#include "core/RuntimeControl.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_anchor.h"
#include "minecraft/java/api/game_blockplace.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_freelook.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/modules/combat/shared/BlockAim.h"
#include "minecraft/modules/combat/shared/HotbarSession.h"
#include "minecraft/modules/combat/shared/ExplosionCoordinator.h"
#include "minecraft/modules/combat/legitcrystalaura/CrystalRules.h"
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
#include "minecraft/modules/misc/teams/Teams.h"
#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include "minecraft/utils/rotation/AimUtils.h"
#include "ui/clickgui/ClickGui.h"
#include "ui/notifications/Notifications.h"
#include "utils/Keyboard.h"
#include <atomic>
#include <chrono>
#include <format>
#include <vector>
namespace nova::module::anchor {
using Vec=rotation::Vec3;
using namespace explosive;
using Action=java::AnchorAction;
using Item=java::AnchorItem;
static std::int64_t Now(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
class LegitAnchorAura final:public Module {
public:
    LegitAnchorAura():Module("LegitAnchorAura",Category::Combat,"General","General"),slots_(this){
        enableOverlay();
        const auto integer=[](double v){return std::format("{:.0f}",v);};
        const auto decimal=[](double v){return std::format("{:.1f}",v);};
        auto range=subNumber("Range",1,6,.1,4.5,decimal),fov=subNumber("FOV",1,360,1,180,integer),walls=subBool("Through Walls",false);
        range_=N(range);fov_=N(fov);walls_=B(walls);addSettings({Properties::PropDesc("Target",nullptr,{range,fov,walls})});
        auto delay=subNumber("Delay",0,1000,5,0,integer),sw=subBool("Auto Switch",true),silent=subBool("Silent",false);
        placeDelay_=N(delay);switch_=B(sw);silent_=B(silent);place_=&addBool("Place",true,{delay,sw,silent});
        behind_=&addBool("Prefer Behind",false);
        safeAnchor_=&addBool("SafeAnchor",false);
        auto boomDelay=subNumber("Delay",0,1000,5,0,integer);detonateDelay_=N(boomDelay);detonate_=&addBool("Detonate",true,{boomDelay});
        swing_=&addBool("Swing",true);auto speed=subNumber("Speed",1,180,1,45,integer);speed_=N(speed);
        addSettings({Properties::PropDesc("Rotation",nullptr,{speed})});
        auto ignore=subBool("Ignore Self Damage",false),minimum=subNumber("Min Target Damage",0,36,.5,4,decimal);
        auto maximum=subNumber("Max Self Damage",0,36,.5,6,decimal),remaining=subNumber("Min Remaining Health",1,20,.5,4,decimal);
        ignore_=B(ignore);minimum_=N(minimum);maximum_=N(maximum);remaining_=N(remaining);
        addSettings({Properties::PropDesc("Safety",nullptr,{ignore,minimum,maximum,remaining})});esp_=&addBool("Place ESP",true);
        auto& bus=event::EventBus::Get();
        begin_=bus.Subscribe<event::GameTickEvent>([this](auto& e){if(e.type==event::EventType::Pre)Begin();},event::Priority::Highest-3);
        planEvent_=bus.Subscribe<event::GameTickEvent>([this](auto& e){if(e.type==event::EventType::Pre)Prepare();},event::Priority::Highest);
        aimEvent_=bus.Subscribe<event::GameTickEvent>([this](auto& e){if(e.type==event::EventType::Pre)Aim();},event::Priority::Highest+2);
        use_=bus.Subscribe<event::PrePlayerInteractEvent>([this](auto&){Use();},event::Priority::High+1);
        motion_=bus.Subscribe<event::MotionEvent>([this](auto& e){if(e.type==event::EventType::Post)turn_.Observe({e.yaw,e.pitch});},event::Priority::Lowest);
        respawn_=bus.Subscribe<event::RespawnEvent>([this](auto&){reset_=true;espView_.Clear();},event::Priority::Highest);
    }
    ~LegitAnchorAura()override{begin_.Reset();planEvent_.Reset();aimEvent_.Reset();use_.Reset();motion_.Reset();respawn_.Reset();Release();}
private:
    static Properties::NumberProp* N(const Properties::PropDesc& p){return static_cast<Properties::NumberProp*>(p.getProp());}
    static Properties::BoolProperty* B(const Properties::PropDesc& p){return static_cast<Properties::BoolProperty*>(p.getProp());}
    struct Plan {java::BlockPos pos,shield;java::BlockHitResultInfo hit;rotation::Rotation angle;Action action=Action::Place;int slot=-1,target=-1;double damage=0;bool existing=false,shieldMissing=false;};
    static java::BlockPos ActionPos(const Plan& p){return p.action==Action::Shield?p.shield:p.pos;}
    enum Reject {State,Range,Dimension,Space,Stock,Ray,Target,DamageRead,DamageLow,Safety,Count};
    void Report(){
        if(!enabled_||Now()<nextReport_)return;nextReport_=Now()+1000;
        LogTo("Anchor: stage=%s selected=%d pending=%d search=%zu/%zu slots[anchor=%d glow=%d det=%d] reject[state=%u range=%u dim=%u space=%u stock=%u ray=%u target=%u damageRead=%u damageLow=%u safety=%u]",
            stage_,int(have_),int(pending_),cursor_,grid_.size(),slotCache_[0],slotCache_[1],slotCache_[2],
            rejects_[State],rejects_[Range],rejects_[Dimension],rejects_[Space],rejects_[Stock],rejects_[Ray],rejects_[Target],rejects_[DamageRead],rejects_[DamageLow],rejects_[Safety]);
        for(auto& n:rejects_)n=0;
    }
    void onEnable()override{enabled_=true;reset_=true;}
    void onDisable()override{enabled_=false;reset_=true;if(!anchorAuraEnabled)anchorWorking=false;espView_.Clear();}
    void onRender()override{if(enabled_&&esp_->get())espView_.Draw();}
    void Release(){rotation::ReleaseExclusiveRotations(this);rotation::ReleaseAttack(this);if(!anchorAuraEnabled)anchorWorking=false;}
    void Clear(){slots_.Restore();Release();pending_=have_=ready_=false;aimTicks_=0;resumeAimTick_=0;grid_.clear();cursor_=0;candidate_=false;bestValid_=rearValid_=false;world_.Reset();player_.Reset();espView_.Clear();turn_.Reset();}
    void YieldForAura(const char* reason){
        stage_=reason;ready_=false;aimTicks_=0;resumeAimTick_=tick_+2;
        Release();slots_.Restore();
    }
    void Begin(){
        if(anchorAuraEnabled){Clear();anchorConfirming=false;return;}
        ++tick_;ResetAnchor();Release();ready_=false;used_=false;damageCache_.clear();
        for(auto& slot:slotCache_)slot=-2;glowCount_=-1;stage_="idle";
        if(reset_.exchange(false)){Clear();nextPlace_=nextBoom_=retryAt_=0;}
        anchorConfirming=enabled_&&pending_;
        if(searchBehind_!=behind_->get()||searchSafe_!=safeAnchor_->get()){
            searchBehind_=behind_->get();searchSafe_=safeAnchor_->get();grid_.clear();candidate_=bestValid_=rearValid_=false;
            if(!pending_&&have_&&!plan_.existing)have_=false;
        }
        if(!slots_.Prepare(silent_->get())||(!switch_->get()&&slots_.Active())){slots_.Restore();used_=true;}
        if(!enabled_){slots_.Restore();return;}
        if(!java::CrystalSupported()){notifications::Post("LegitAnchorAura: requires Lunar 1.21.11 or Minecraft 26.3",notifications::Type::Warning);setEnabled(false);return;}
        turn_.Seed(rotation::GetRotation());
    }
    bool Context(java::EntityHandle& self){
        self=java::PlayerEntity();auto world=java::LevelEntity();float y=0,p=0;
        if(anchorAuraEnabled||!enabled_||reset_||!self||!world||!self.IsAlive()||self.IsSpectator())return false;
        if(world_&&(!world.IsSame(world_)||!self.IsSame(player_))){Clear();return false;}
        if(!world_){world_=java::LevelEntity();player_=java::PlayerEntity();}
        return input::IsProcessForeground()&&!java::ScreenOpen()&&!clickgui::IsOpen()&&!java::FreeCamActive()&&!java::FreeLookAngles(y,p)&&
            java::CrystalContextReady()&&!java::KeyShiftDown()&&!scaffold::IsEnabled()&&!automace::IsWorking()&&!automlg::IsWorking()&&!autocharge::IsWorking()&&!refill::IsWorking()&&
            !autoblock::OwnsItemUse()&&!autorod::OwnsItemUse()&&!velocity::IsDelaying()&&!antikb::IsDelaying()&&!backtrack::IsLagging();
    }
    bool Allowed(const java::EntityHandle& self,const java::EntityHandle& e){
        if(!e||e.IsSame(self)||!e.IsPlayer()||!e.IsAlive()||e.IsSpectator()||teams::IsTeammate(self,e)||antibot::IsBot(e))return false;
        Vec eye,p;java::Box box;float yaw=0,pitch=0;
        return self.EyePosition(1,eye)&&e.EyePosition(1,p)&&e.BoundingBox(box)&&java::PlayerRotation(yaw,pitch)&&
            eye.DistanceTo(rotation::ClosestPointOnAabb(box,eye))<=range_->get()&&
            std::abs(std::remainder(double(rotation::Calculate(eye,p).yaw-yaw),360.))<=fov_->get()*.5;
    }
    int Slot(Item kind){
        auto& cached=slotCache_[int(kind)];if(cached!=-2)return cached;
        int selected=-1;cached=-1;if(!java::EffectiveSelectedSlot(selected))return cached;
        if(java::AnchorSlot(selected,kind))return cached=selected;if(!switch_->get())return cached;
        for(int i=0;i<9;++i)if(java::AnchorSlot(i,kind))return cached=i;return cached;
    }
    bool AnchorGeometry(const java::EntityHandle& self,const java::BlockPos& p,Plan& out){
        java::AnchorState state;bool explodes=false;Vec eye;double reach=0;
        if(!java::ReadAnchor(p,state)||!self.EyePosition(1,eye)||!java::CrystalBlockReach(reach)){++rejects_[State];return false;}
        reach=std::min(reach,range_->get());
        // Mio measures the candidate block center, not the nearest AABB point.
        if(eye.DistanceTo(Center(p))>reach){++rejects_[Range];return false;}
        // Reject solid cells before environment, inventory, entity and ray queries.
        if(!state.anchor&&(!place_->get()||!state.replaceable)){++rejects_[Space];return false;}
        if(!java::AnchorExplodes(p,explodes)||!explodes){++rejects_[Dimension];return false;}
        if(Now()<retryAt_&&SameBlock(p,failed_))return false;
        out={};out.pos=p;out.existing=state.anchor;
        if(state.anchor){
            if(!detonate_->get()&&!safeAnchor_->get())return false;
            out.action=state.charges?Action::Detonate:Action::Charge;
            out.slot=Slot(state.charges?Item::Detonator:Item::Glowstone);
            if(!safeAnchor_->get()&&(out.slot<0||(!state.charges&&switch_->get()&&Slot(Item::Detonator)<0))){++rejects_[Stock];return false;}
            const bool hit=AimBlock(self,p,reach,walls_->get(),rotation::GetRotation(),out.angle,out.hit,-1,!turn_.Ready(rotation::GetRotation()));
            if(!hit)++rejects_[Ray];return hit;
        }
        out.slot=Slot(Item::Anchor);
        if(out.slot<0||(switch_->get()&&detonate_->get()&&(Slot(Item::Glowstone)<0||Slot(Item::Detonator)<0))){++rejects_[Stock];return false;}
        if(!AimPlacement(self,p,reach,walls_->get(),rotation::GetRotation(),out.angle,out.hit,!turn_.Ready(rotation::GetRotation()))){++rejects_[Ray];return false;}
        if(!java::AnchorBuildable(p)){++rejects_[Space];return false;}return true;
    }
    bool ShieldHit(const java::EntityHandle& self,java::BlockPos p,double reach,rotation::Rotation& angle,java::BlockHitResultInfo& hit){
        const java::BlockPos steps[]={{0,1,0},{1,0,0},{-1,0,0},{0,0,1},{0,0,-1},{0,-1,0}};
        const int faces[]={1,5,4,3,2,0};
        for(int i=0;i<6;++i){const auto d=steps[i];java::BlockPos b{p.x-d.x,p.y-d.y,p.z-d.z};
            if(java::GlowstoneSupport(b)&&AimBlock(self,b,reach,walls_->get(),rotation::GetRotation(),angle,hit,faces[i],!turn_.Ready(rotation::GetRotation())))return true;
        }return false;
    }
    bool Geometry(const java::EntityHandle& self,const java::BlockPos& p,Plan& out){
        if(!AnchorGeometry(self,p,out))return false;
        if(!safeAnchor_->get())return true;
        Vec at;double reach=0;if(!self.Position(at)||!java::CrystalBlockReach(reach))return false;
        reach=std::min(reach,range_->get());
        const double dx=at.x-(p.x+.5),dz=at.z-(p.z+.5),length=std::hypot(dx,dz);
        if(length<.001){++rejects_[Safety];return false;}
        java::BlockPos directions[]={{1,0,0},{-1,0,0},{0,0,1},{0,0,-1}};
        auto alignment=[&](auto d){return (d.x*dx+d.z*dz)/length;};
        std::stable_sort(std::begin(directions),std::end(directions),[&](auto a,auto b){return alignment(a)>alignment(b);});
        for(auto d:directions){
            if(alignment(d)<.5)continue;
            java::BlockPos shield{p.x+d.x,p.y,p.z+d.z};bool exists=false;
            if(!java::ReadGlowstone(shield,exists))continue;
            rotation::Rotation angle;java::BlockHitResultInfo hit;
            if(!exists){
                if(glowCount_<0)glowCount_=java::AnchorGlowstoneCount();
                if(glowCount_<(!detonate_->get()||out.action==Action::Detonate?1:2)||!java::AnchorBuildable(shield)||!ShieldHit(self,shield,reach,angle,hit))continue;
            }
            if(exists&&out.existing&&!detonate_->get())return false;
            out.shield=shield;out.shieldMissing=!exists;
            if(out.existing&&!exists){out.action=Action::Shield;out.slot=Slot(Item::Glowstone);out.angle=angle;out.hit=hit;}
            if(out.slot<0){++rejects_[Stock];return false;}
            return true;
        }
        ++rejects_[Safety];return false;
    }
    struct DamageEntry{int id;java::BlockPos p,shield;bool projected;double value;};
    bool Damage(const java::EntityHandle& e,const java::BlockPos& p,double& damage,const java::BlockPos* shield=nullptr){
        int id=-1;if(!java::EntityId(e,id))return false;
        for(const auto& v:damageCache_)if(v.id==id&&SameBlock(v.p,p)&&v.projected==bool(shield)&&(!shield||SameBlock(v.shield,*shield))){damage=v.value;return true;}
        java::Box box;Vec at;java::CrystalDefense defense;double exposure=0;
        if(!e.Position(at)||!e.BoundingBox(box)||!java::ReadCrystalDefense(e,defense)||
           !(shield?java::AnchorExposureWithShield(e,p,box,shield,exposure):java::AnchorExposure(e,p,box,exposure))){++rejects_[DamageRead];return false;}
        damage=crystal::Damage(at.DistanceTo(Center(p)),exposure,defense.difficulty,defense.armor,defense.toughness,defense.resistance,defense.protection,5);
        if(!std::isfinite(damage))return false;damageCache_.push_back({id,p,shield?*shield:java::BlockPos{},bool(shield),damage});return true;
    }
    bool SafePlan(const java::EntityHandle& self,const Plan& p){
        std::vector<java::EntityHandle> players;if(!java::WorldPlayers(players))return false;
        for(const auto& e:players){
            if(!e.IsAlive()||e.IsSpectator())continue;
            const bool own=e.IsSame(self);if(own?(ignore_->get()&&!safeAnchor_->get()):!teams::IsTeammate(self,e))continue;
            Vec at;if(!e.Position(at))return false;if(at.DistanceTo(Center(p.pos))>10)continue;
            java::CrystalDefense defense;double damage=0;
            if(!java::ReadCrystalDefense(e,defense)||!Damage(e,p.pos,damage,safeAnchor_->get()&&p.shieldMissing?&p.shield:nullptr)||!crystal::Safe(damage,defense.health,defense.absorption,maximum_->get(),remaining_->get()))return false;
        }return true;
    }
    bool Refresh(const java::EntityHandle& self,Plan& p){
        auto target=java::EntityById(p.target);if(!Allowed(self,target)){++rejects_[Target];return false;}
        Plan next;if(!Geometry(self,p.pos,next)||!Damage(target,p.pos,next.damage,safeAnchor_->get()&&next.shieldMissing?&next.shield:nullptr))return false;
        if(next.damage<minimum_->get()||next.damage<.5){++rejects_[DamageLow];return false;}
        next.target=p.target;if(!SafePlan(self,next)){++rejects_[Safety];return false;}p=next;return true;
    }
    // Target facing, not our view or the target's travel direction. Rear cone
    // tolerates the block grid; alignment then damage breaks placement ties.
    double RearAlignment(const Plan& p){
        if(!behind_->get()||p.existing)return -1;
        auto target=java::EntityById(p.target);Vec at;float yaw=0;
        if(!target||!target.Position(at)||!java::AnchorTargetYaw(target,yaw))return -1;
        const double x=p.pos.x+.5-at.x,z=p.pos.z+.5-at.z,length=std::hypot(x,z);
        if(!std::isfinite(length)||length<.001)return -1;
        const double radians=std::remainder(double(yaw),360.)*.017453292519943295;
        const double alignment=(x*std::sin(radians)-z*std::cos(radians))/length;
        return alignment>=.7071067811865475?alignment:-1;
    }
    bool Prefer(const Plan& next,const Plan& previous,double damageMargin){
        const double a=RearAlignment(next),b=RearAlignment(previous);
        if((a>=0)!=(b>=0))return a>=0;
        if(a>=0&&std::abs(a-b)>.02)return a>b;
        return damageMargin==0?next.damage>=previous.damage:next.damage>previous.damage+damageMargin;
    }
    void Search(const java::EntityHandle& self){
        Vec eye;if(!self.EyePosition(1,eye))return;
        // Ordinary movement must not continually restart a budgeted full scan.
        // Live Geometry/Refresh reject cells that have moved out of reach.
        if(grid_.empty()||eye.DistanceTo(searchEye_)>range_->get()||std::abs(searchRange_-range_->get())>.01){
            grid_.clear();cursor_=0;candidate_=bestValid_=rearValid_=false;searchEye_=eye;
            searchRange_=range_->get();
            Vec feet;if(!self.Position(feet))return;const double range=range_->get();
            // Same x/y/z nesting and floor/ceil offset bounds as Mio.
            for(int x=-int(std::floor(range));x<int(std::ceil(range));++x)
            for(int y=-int(std::floor(range));y<int(std::ceil(range));++y)
            for(int z=-int(std::floor(range));z<int(std::ceil(range));++z){
                java::BlockPos p{int(std::floor(feet.x+x)),int(std::floor(eye.y+y)),int(std::floor(feet.z+z))};
                if(eye.DistanceTo(Center(p))<=range)grid_.push_back(p);
            }
        }
        const auto start=std::chrono::steady_clock::now();int visited=0,evaluated=0;
        // Read eligible IDs once per search slice, not once per candidate cell.
        // Resuming an old candidate still rechecks Allowed before its damage.
        std::vector<java::EntityHandle> players;if(!java::WorldPlayers(players))return;
        std::vector<int> allowedPlayers;
        for(const auto& e:players){int id=-1;if(Allowed(self,e)&&java::EntityId(e,id))allowedPlayers.push_back(id);}
        while(cursor_<grid_.size()&&visited<256&&evaluated<24){
            if(visited&&std::chrono::steady_clock::now()-start>std::chrono::milliseconds(2))break;
            if(!candidate_){
                ++visited;if(!Geometry(self,grid_[cursor_],candidatePlan_)){++cursor_;continue;}
                candidatePlayers_=allowedPlayers;candidateCursor_=0;candidatePlan_.damage=0;
                if(candidatePlayers_.empty())++rejects_[Target];
                candidate_=true;
            }
            while(candidateCursor_<candidatePlayers_.size()&&evaluated<24){
                auto e=java::EntityById(candidatePlayers_[candidateCursor_++]);double damage=0;++evaluated;
                if(Allowed(self,e)&&Damage(e,candidatePlan_.pos,damage,safeAnchor_->get()&&candidatePlan_.shieldMissing?&candidatePlan_.shield:nullptr)&&damage>=minimum_->get()&&damage>=.5&&damage>candidatePlan_.damage){
                    candidatePlan_.damage=damage;java::EntityId(e,candidatePlan_.target);
                }
                if(std::chrono::steady_clock::now()-start>std::chrono::milliseconds(2))break;
            }
            if(candidateCursor_<candidatePlayers_.size())break;
            candidate_=false;++cursor_;
            if(candidatePlan_.damage<=0||!Refresh(self,candidatePlan_))continue;
            // Mio returns the first valid existing anchor immediately; new
            // positions compete by highest target damage over the full scan.
            if(candidatePlan_.existing){plan_=candidatePlan_;have_=true;grid_.clear();return;}
            if(RearAlignment(candidatePlan_)>=0&&(!rearValid_||Prefer(candidatePlan_,rearBest_,0))){rearBest_=candidatePlan_;rearValid_=true;}
            if(!bestValid_||candidatePlan_.damage>=best_.damage){best_=candidatePlan_;bestValid_=true;}
        }
        if(cursor_==grid_.size()){
            const bool rear=rearValid_&&Refresh(self,rearBest_)&&RearAlignment(rearBest_)>=0;
            Plan* chosen=rear?&rearBest_:(bestValid_&&Refresh(self,best_)?&best_:nullptr);
            if(chosen&&(!have_||Prefer(*chosen,plan_,.5))){plan_=*chosen;have_=true;}
            grid_.clear();bestValid_=rearValid_=candidate_=false;
        }
    }
    void Prepare(){
        stage_="context-blocked";
        java::EntityHandle self;if(used_||!Context(self)){slots_.Restore();espView_.Clear();return;}
        Vec at;bool explodes=false;
        stage_="dimension-read";
        if(!self.Position(at)||!java::AnchorExplodes({int(std::floor(at.x)),int(std::floor(at.y)),int(std::floor(at.z))},explodes))return;
        if(!explodes){slots_.Restore();setEnabled(false);notifications::Post("LegitAnchorAura: anchors do not explode here",notifications::Type::Warning);return;}
        if(pending_){
            stage_="confirmation-wait";
            java::AnchorState state;
            if(!java::ReadAnchor(pendingPlan_.pos,state)){slots_.Restore();return;}
            bool shieldExists=false;
            const bool confirmed=pendingPlan_.action==Action::Shield?(java::ReadGlowstone(pendingPlan_.shield,shieldExists)&&shieldExists):
                pendingPlan_.action==Action::Place?state.anchor:pendingPlan_.action==Action::Charge?state.anchor&&state.charges>0:!state.anchor;
            if(!confirmed&&Now()<deadline_){
                // Maintain the transaction only while its original target remains safe.
                auto e=java::EntityById(pendingPlan_.target);double damage=0;
                if(Allowed(self,e)&&Damage(e,pendingPlan_.pos,damage)&&damage>=minimum_->get()&&SafePlan(self,pendingPlan_)){
                    plan_=pendingPlan_;OfferAnchor(damage,true);espView_.Show(ActionPos(plan_));
                }else {slots_.Restore();espView_.Clear();}return;
            }
            pending_=false;
            if(!confirmed){failed_=pendingPlan_.pos;retryAt_=Now()+500;have_=false;grid_.clear();}
            if(pendingPlan_.action==Action::Detonate)have_=false;
        }
        stage_="search";
        if(have_&&!Refresh(self,plan_))have_=false;
        if(!have_||!plan_.existing)Search(self);
        if(!have_||!Refresh(self,plan_)){have_=false;slots_.Restore();espView_.Clear();return;}
        stage_="plan-ready";ready_=true;OfferAnchor(plan_.damage,false);espView_.Show(ActionPos(plan_));
    }
    void Aim(){
        if(!ready_||!ChooseAnchor()){
            if(ready_)stage_="crystal-priority";aimTicks_=0;slots_.Restore();
            // Keep confirmation ESP, but do not hold a weapon or turn for a
            // server update that requires no new use this tick.
            if(!pending_)espView_.Clear();return;
        }
        if(pending_){stage_="confirmation-wait";aimTicks_=0;slots_.Restore();return;}
        if(tick_<resumeAimTick_){stage_="aura-yield";ready_=false;slots_.Restore();return;}
        if(Now()<((plan_.action==Action::Place||plan_.action==Action::Shield)?nextPlace_:nextBoom_)){
            stage_="delay";ready_=false;aimTicks_=0;slots_.Restore();return;
        }
        java::EntityHandle self;
        if(!Context(self)||!rotation::CanSubmitItemUse()){YieldForAura("interaction-busy");return;}
        // Six ticks cover a normal 180-degree turn at the default 45 deg/tick.
        // A persistent miss must allow at least the next full aura tick through.
        if(aimTicks_>=6){YieldForAura("aim-timeout");return;}
        if(!slots_.Equip(plan_.slot,silent_->get())){YieldForAura("equip");return;}
        const auto from=rotation::GetRotation();const auto wanted=plan_.angle;
        double d=std::hypot(std::remainder(double(wanted.yaw-from.yaw),360.),double(wanted.pitch-from.pitch));
        auto next=rotation::LinearStep(from,wanted,std::min(speed_->get(),d));double sensitivity=0;
        if(java::PlayerSensitivity(sensitivity))next=rotation::ApplySensitivityPatch(next,from,sensitivity);
        anchorWorking=rotation::SetExclusiveRotations(this,next);
        if(anchorWorking)++aimTicks_;else {YieldForAura("rotation-busy");return;}
        stage_=anchorWorking?"aiming":"rotation-busy";
    }
    void Use(){
        struct Finish{LegitAnchorAura* self;~Finish(){self->Report();}} finish{this};
        if(used_)return;used_=true;
        if(!ready_||!anchorWorking||pending_||!rotation::OwnsExclusiveRotations(this))return;
        stage_="interaction-busy";
        java::EntityHandle self;if(!Context(self)||!rotation::CanSubmitItemUse()){YieldForAura("interaction-busy");return;}
        const auto action=plan_.action;Plan checked=plan_;
        stage_="submission-recheck";if(!Refresh(self,checked)||checked.action!=action||!SameBlock(ActionPos(checked),ActionPos(plan_))){YieldForAura("submission-recheck");return;}
        stage_="delay";
        if(Now()<((action==Action::Place||action==Action::Shield)?nextPlace_:nextBoom_)){YieldForAura("delay");return;}
        stage_="rotation-history";if(!turn_.Ready(rotation::GetRotation()))return;
        java::BlockHitResultInfo hit;double reach=0;if(!java::CrystalBlockReach(reach))return;
        stage_="ray-wait";
        if(!BlockHit(self,checked.hit.blockPos,rotation::GetRotation(),std::min(reach,range_->get()),walls_->get(),hit)||
           ((action==Action::Place||action==Action::Shield)&&hit.direction!=checked.hit.direction))return;
        stage_="equip";if(!slots_.Equip(checked.slot,silent_->get())){YieldForAura("equip");return;}
        bool entered=false;bool accepted=action==Action::Shield?java::PlaceGlowstone(checked.shield,hit,checked.slot,this,entered):
            java::UseAnchor(checked.pos,hit,checked.slot,action,this,entered);
        stage_=entered?"submitted":"native-not-entered";
        if(accepted){if(swing_->get())java::SwingMainHand();else java::SendSwingPacket();}
        if(entered){
            aimTicks_=0;
            pending_=true;pendingPlan_=checked;pendingAngle_=rotation::GetRotation();deadline_=Now()+300;turn_.Submitted();
            if(action==Action::Place||action==Action::Shield)nextPlace_=Now()+std::int64_t(placeDelay_->get());
            if(action==Action::Detonate)nextBoom_=Now()+std::int64_t(detonateDelay_->get());
        }else YieldForAura("native-not-entered");
    }
    Properties::NumberProp *range_{},*fov_{},*speed_{},*placeDelay_{},*detonateDelay_{},*minimum_{},*maximum_{},*remaining_{};
    Properties::BoolProperty *walls_{},*place_{},*switch_{},*silent_{},*detonate_{},*swing_{},*ignore_{},*esp_{},*behind_{},*safeAnchor_{};
    event::Subscription begin_,planEvent_,aimEvent_,use_,motion_,respawn_;
    std::atomic<bool> enabled_{false},reset_{false};
    int aimTicks_=0;
    std::int64_t resumeAimTick_=0;
    HotbarSession slots_;crystal::PlacementEsp espView_;crystal::PlacementTurnGate turn_;
    java::EntityHandle world_,player_;
    Plan plan_,pendingPlan_,best_,rearBest_,candidatePlan_;rotation::Rotation pendingAngle_;
    java::BlockPos failed_;Vec searchEye_;
    std::vector<java::BlockPos> grid_;std::vector<int> candidatePlayers_;std::vector<DamageEntry> damageCache_;
    size_t cursor_=0,candidateCursor_=0;int tick_=0,glowCount_=-1;
    const char* stage_="idle";unsigned rejects_[Count]{};int slotCache_[3]{-2,-2,-2};
    double searchRange_=0;std::int64_t nextReport_=0;
    bool have_=false,ready_=false,pending_=false,used_=false,candidate_=false,bestValid_=false,rearValid_=false,searchBehind_=false,searchSafe_=false;
    std::int64_t deadline_=0,nextPlace_=0,nextBoom_=0,retryAt_=0;
};
}
namespace nova::module {std::unique_ptr<Module> MakeLegitAnchorAuraModule(){return std::make_unique<anchor::LegitAnchorAura>();}}
