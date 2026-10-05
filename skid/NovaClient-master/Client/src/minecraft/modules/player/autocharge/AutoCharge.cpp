#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/modules/player/autocharge/Trajectory.h"
#include "minecraft/modules/player/autocharge/SlotHandoff.h"
#include "minecraft/modules/player/autocharge/ManualUseGate.h"
#include "minecraft/modules/player/autocharge/Prediction.h"
#include "minecraft/modules/player/autocharge/WindPlanner.h"
#include "minecraft/modules/player/autocharge/Targeting.h"
#include "minecraft/modules/combat/automace/AutoMace.h"
#include "minecraft/modules/misc/teams/Teams.h"
#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/modules/player/autocharge/FlightObservation.h"
#include "core/RuntimeControl.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/modules/player/refill/Refill.h"
#include "minecraft/modules/combat/velocity/Velocity.h"
#include "minecraft/modules/combat/antikb/AntiKB.h"
#include "minecraft/modules/combat/backtrack/Backtrack.h"
#include "minecraft/modules/combat/autoblock/AutoBlockRuntime.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_charge.h"
#include "minecraft/java/api/game_itemuse.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_freelook.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_mlg.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_options.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include "ui/clickgui/ClickGui.h"
#include "ui/notifications/Notifications.h"
#include <atomic>
#include <format>
#include <unordered_set>
#include <mutex>

namespace nova::module {
namespace {
std::atomic<bool> working{false};
autocharge::ManualUseGate manualUse;
using namespace autocharge;
enum class Phase { Idle, Pearl, Track, Wind, Hold };
class AutoCharge;
AutoCharge* instance=nullptr;

class AutoCharge final : public Module {
public:
    AutoCharge() : Module("AutoCharge",Category::Player,"General","General") {
        instance=this;
        speed_=&addNumber("Rotation Speed",1,180,1,90,{},[](double n){return std::format("{:.0f}",n);});
        height_=&addNumber("Height",5,45,.5,25,{},[](double n){return std::format("{:.1f} blocks",n);});
        auto fov=subNumber("FOV",1,360,1,180,[](double n){return std::format("{:.0f}",n);});
        auto range=subNumber("Range",1,32,.5,16,[](double n){return std::format("{:.1f} blocks",n);});
        targetFov_=static_cast<Properties::NumberProp*>(fov.getProp());
        targetRange_=static_cast<Properties::NumberProp*>(range.getProp());
        targetPlayer_=&addBool("Target Player",false,{fov,range});
        auto& bus=event::EventBus::Get();
        packet_=bus.Subscribe<event::PacketEvent>([this](auto& e){
            if(e.type!=event::EventType::Outgoing||e.IsCancelled())return;
            java::ChargeMovementPacket p;if(!java::ReadChargeMovementPacket(e.packet,p))return;
            std::lock_guard lock(movementMutex_);
            if(p.kind==java::ChargeMovementPacket::Kind::Move)movement_.Move(p.hasPosition,p.position,p.onGround);
            else if(p.kind==java::ChargeMovementPacket::Kind::EndTick)movement_.EndTick();
        },event::Priority::Lowest+100);
        tick_=bus.Subscribe<event::GameTickEvent>([this](auto& e){if(e.type==event::EventType::Pre)Tick();},event::Priority::Highest);
        interact_=bus.Subscribe<event::PrePlayerInteractEvent>([this](auto&){Interact();},event::Priority::Lowest);
        respawn_=bus.Subscribe<event::RespawnEvent>([this](auto&){
            {std::lock_guard lock(movementMutex_);movement_={};}
            if(working.load()){requested_=false;setEnabled(false);}reset_=true;
        },event::Priority::Highest);
        click_=bus.Subscribe<event::ClickEvent>([this](auto& e){if(manualUse.Blocks())e.Cancel();},event::Priority::Highest);
        input_=bus.Subscribe<event::PrePlayerInteractEvent>([](auto&){
            if(manualUse.Blocks())java::SuppressUseKey();
        },event::Priority::Lowest+100);
    }
    ~AutoCharge() override {
        tick_.Reset();interact_.Reset();respawn_.Reset();click_.Reset();packet_.Reset();input_.Reset();Cleanup();
        if(manualUse.Reset())java::RestoreUseKey();
        java::ReleaseSilentSlot(this);
        instance=nullptr;
    }
    void InterruptForRescue(){if(working.load())Stop("yielding to AutoMLG");}
private:
    void onEnable() override {requested_=true;reset_=true;}
    void onDisable() override {requested_=false;reset_=true;}
    bool SameWorld() const {
        auto p=java::PlayerEntity(),w=java::LevelEntity();
        return p&&w&&player_&&world_&&p.IsSame(player_)&&w.IsSame(world_);
    }
    void Cleanup() {
        // A submitted use may have sent a packet even if its JNI result failed.
        // Keep the effective slot through the rest of this tick; otherwise the
        // vanilla controller can immediately emit another held-item change.
        slotHandoff_.Cleanup([&]{java::ReleaseSilentSlot(this);});
        rotation::ReleaseExclusiveRotations(this);
        working=false;phase_=Phase::Idle;
        player_.Reset();world_.Reset();oldPearls_.clear();blockers_.clear();pearlId_=-1;
        target_.Reset();targetId_=-1;targetMotion_={};targeted_=highArc_=false;launchCheckTick_=-1;
    }
    void Stop(const char* reason=nullptr) {
        LogTo("AutoCharge: stop phase=%d age=%d elapsed=%d reason=%s",int(phase_),age_,age_-sentAt_,reason?reason:"complete/cancel");
        requested_=false;
        if(reason)notifications::Post(std::string("AutoCharge: ")+reason,notifications::Type::Warning);
        Cleanup();setEnabled(false);
    }
    void Waiting(const char* reason) {
        if(lastWaitLog_!=age_&&(age_%10==0||lastWaitLog_<0)){
            lastWaitLog_=age_;
            LogTo("AutoCharge: waiting phase=%d age=%d elapsed=%d reason=%s",int(phase_),age_,age_-sentAt_,reason);
        }
    }
    rotation::Rotation Quantize(rotation::Rotation target) const {
        return AimStep(angle_,speed_->get(),sensitivity_,sensitivityValid_).Quantize(target);
    }
    bool Aim(rotation::Rotation target) {
        const double configured=speed_->get();
        angle_=Quantize(rotation::LinearStep(angle_,target,std::isfinite(configured)?std::clamp(configured,1.,180.):90.));
        if(!rotation::SetExclusiveRotations(this,angle_))return false;
        angle_=rotation::GetRotation();
        return true;
    }
    int FindSlot(java::KnownItem item) {
        for(int i=0;i<9;++i)if(java::ChargeSlotReady(i,item))return i;
        return -1;
    }
    bool Snapshot(std::vector<java::EntityHandle>& entities) {
        Vec3 p;if(!player_.Position(p))return false;
        const double radius=targeted_?72.:40.;
        return java::WorldEntities(player_,{p.x-radius,p.y-10,p.z-radius,p.x+radius,p.y+90,p.z+radius},entities);
    }
    bool ValidTarget(const java::EntityHandle& entity) const {
        return entity&&!entity.IsSame(player_)&&entity.IsPlayer()&&entity.IsAlive()&&!entity.IsSpectator()&&
            !teams::IsTeammate(player_,entity)&&!antibot::IsBot(entity);
    }
    bool SelectTarget() {
        std::vector<java::EntityHandle> players;Vec3 self;
        if(!player_.Position(self)||!java::WorldPlayers(players))return false;
        double best=1e9;
        for(auto& entity:players){
            Vec3 feet;java::Box box;int id=-1;
            if(!ValidTarget(entity)||!entity.Position(feet)||!entity.BoundingBox(box)||!java::EntityId(entity,id)||
               !TargetInView(self,feet,control_.yaw,Setting(targetFov_->get(),1,360,180),Setting(targetRange_->get(),1,32,16)))continue;
            const double distance=feet.DistanceToSqr(self);
            if(distance<best){best=distance;targetId_=id;target_=std::move(entity);}
        }
        return bool(target_);
    }
    bool ReadTarget(bool requireFov) {
        auto entity=java::EntityById(targetId_);Vec3 self,feet;java::Box box;
        if(!ValidTarget(entity)||!entity.IsSame(target_)||!player_.Position(self)||!entity.Position(feet)||
           !entity.BoundingBox(box)||(requireFov&&!TargetInView(self,feet,control_.yaw,Setting(targetFov_->get(),1,360,180),
                Setting(targetRange_->get(),1,32,16))))return false;
        const Vec3 head{(box.minX+box.maxX)*.5,box.maxY,(box.minZ+box.maxZ)*.5};
        if(targetMotion_.tick<0){
            Vec3 now,before;
            if(entity.PositionPair(now,before)&&Finite(now)&&Finite(before))
                targetMotion_.Update(head.Subtract(now.Subtract(before)),age_-1);
        }
        return targetMotion_.Update(head,age_);
    }
    InterceptTarget TargetGoal() const {return targetMotion_.Goal();}
    void FallbackLaunch(const char* reason) {
        // Only used before the first item submission. Keep this activation and
        // its input/rotation ownership, but never reacquire an unusable target.
        LogTo("AutoCharge: normal launch fallback target=%d reason=%s",targetId_,reason);
        targeted_=false;target_.Reset();targetId_=-1;targetMotion_={};highArc_=false;
        launchCheckTick_=-1;
    }
    bool Start() {
        if(!java::ChargeSupported()){Stop("this version has no supported wind charge");return false;}
        pearlSlot_=FindSlot(java::KnownItem::EnderPearl);windSlot_=FindSlot(java::KnownItem::WindCharge);
        if(pearlSlot_<0||windSlot_<0){Stop("ready pearl and wind charge required in hotbar");return false;}
        player_=java::PlayerEntity();world_=java::LevelEntity();
        if(!java::PlayerRotation(control_.yaw,control_.pitch)){Stop("camera unavailable");return false;}
        angle_=control_;
        targeted_=targetPlayer_->get();highArc_=false;age_=0;sentAt_=0;
        if(targeted_&&!SelectTarget())FallbackLaunch("no eligible player");
        std::vector<java::EntityHandle> entities;
        if(!Snapshot(entities)){Stop("entity snapshot unavailable");return false;}
        for(auto& e:entities){Vec3 v;int id;
            if(java::ChargePearl(e,player_,v)&&java::EntityId(e,id))oldPearls_.insert(id);
        }
        if(!rotation::SetExclusiveRotations(this,angle_)){Stop("rotation is occupied");return false;}
        phase_=Phase::Pearl;working=true;age_=0;windAimTick_=-1;lastWaitLog_=-1;
        LogTo("AutoCharge: start height=%.2f speed=%.2f pearlSlot=%d windSlot=%d targeted=%d",height_->get(),speed_->get(),pearlSlot_,windSlot_,targeted_);
        return true;
    }
    bool ReadPearl(Pearl& pearl) {
        auto entity=java::EntityById(pearlId_);
        Pearl observed;double error=0;
        return entity&&java::ChargePearl(entity,player_,observed.velocity)&&entity.Position(observed.position)&&
            AlignPearl(observed,launch_,age_-sentAt_,pearl,error);
    }
    bool LaunchSnapshot(Vec3& eye,Vec3& inherited,bool& forecast){
        double height=0;if(!player_.EyeHeight(height))return false;
        std::lock_guard lock(movementMutex_);forecast=movement_.CanForecast();return movement_.Launch(height,eye,inherited);
    }
    bool LandingTransition(Vec3 eye,Vec3 inherited) {
        bool grounded=false;double height=0;
        if(!player_.OnGround(grounded)||!player_.EyeHeight(height))return true;
        // shootFromRotation zeros inherited Y on ground. Around landing the
        // server can apply this before the next movement snapshot arrives.
        if(grounded&&std::abs(inherited.y)>.001)return true;
        if(inherited.y>=-.05)return false;
        const auto feet=eye.Add(0,-height,0);
        for(const auto offset:{Vec3{},Vec3{-.25,0,-.25},Vec3{-.25,0,.25},Vec3{.25,0,-.25},Vec3{.25,0,.25}}){
            const auto start=Add(feet,offset).Add(0,.05,0);
            java::BlockHitResultInfo hit;
            if(!java::WorldClip(start,start.Add(0,inherited.y-.13,0),player_,hit,java::ClipMode::Collision)||
               hit.kind!=java::HitKind::Miss)return true;
        }
        return false;
    }
    bool TrackPearl() {
        std::vector<java::EntityHandle> entities;if(!Snapshot(entities))return false;
        int candidate=-1;double best=1e9;
        for(auto& e:entities) {
            Pearl p,aligned;int id;
            if(!java::ChargePearl(e,player_,p.velocity)||!java::EntityId(e,id)||oldPearls_.contains(id)||
               !e.Position(p.position))continue;
            double residual=0;
            if(!AlignPearl(p,launch_,age_-sentAt_,aligned,residual)||residual>=best)continue;
            best=residual;candidate=id;
        }
        if(candidate<0)return false;pearlId_=candidate;return true;
    }
    void RefreshBlockers() {
        blockers_.clear();std::vector<java::EntityHandle> entities;
        blockersValid_=Snapshot(entities);
        for(auto& e:entities){int id;java::Box box;
            if(e.IsSame(player_)||(java::EntityId(e,id)&&id==pearlId_))continue;
            if(java::ChargeEntityHittable(e)&&e.BoundingBox(box))blockers_.push_back(java::BoxInflate(box,.3));
        }
    }
    bool Clear(Vec3 from,Vec3 to) {
        java::BlockHitResultInfo hit;
        if(!blockersValid_||!java::WorldClip(from,to,player_,hit,java::ClipMode::Collision)||hit.kind!=java::HitKind::Miss)return false;
        for(const auto& b:blockers_){double f;
            if(SegmentBox(from,to,{b.minX,b.minY,b.minZ},{b.maxX,b.maxY,b.maxZ},f))return false;
        }
        return true;
    }
    bool Context() {
        java::MlgEnvironment env;
        float freeYaw=0,freePitch=0;
        auto p=java::PlayerEntity();
        return p&&p.IsAlive()&&!p.IsSpectator()&&!java::FreeCamActive()&&!scaffold::IsEnabled()&&
            !java::FreeLookAngles(freeYaw,freePitch)&&
            !velocity::IsDelaying()&&!antikb::IsDelaying()&&!backtrack::IsLagging()&&!autoblock::OwnsItemUse()&&
            !automace::IsWorking()&&!automlg::IsWorking()&&!refill::IsWorking()&&java::ReadMlgEnvironment(env)&&
            !env.water&&!env.lava&&!env.flying&&!env.passenger;
    }
    void Tick() {
        observation_.Tick();
        // Seed only when no packet position has been seen for this world.
        // Subsequent launches use outgoing movement packets, never xo/yo/zo.
        if(java::ChargeSupported()){
            auto self=java::PlayerEntity();Vec3 feet;bool ground=false;
            if(self&&self.Position(feet)&&self.OnGround(ground)){
                std::lock_guard lock(movementMutex_);movement_.Seed(feet,ground);
            }else{std::lock_guard lock(movementMutex_);movement_={};}
        }
        // Minecraft.tick HEAD follows the previous tick's native end packet.
        // Slot restoration belongs here, before the new tick's interactions.
        slotHandoff_.BeginTick([&]{
            java::ReleaseSilentSlot(this);
            if(SameWorld())java::SyncSelectedItem();
        });
        // Keep blocking through the complete last use tick, including a failed
        // submission/Stop. Otherwise held RMB can send camera angles after the
        // silent use in that same tick. Resume held state without queued clicks.
        if(manualUse.BeginTick(working.load()))java::RestoreUseKey();
        if(reset_.exchange(false))Cleanup();
        if(!requested_)return;
        if(phase_==Phase::Idle) {
            if(java::ScreenOpen()||clickgui::IsOpen())return;
            {std::lock_guard lock(movementMutex_);if(java::ChargeSupported()&&!movement_.ready)return;}
            if(!Context()){Stop("player or interaction unavailable");return;}
            if(!Start())return;
            manualUse.Begin();
            java::SuppressUseKey();
        } else {
            if(!SameWorld()){Stop();return;}
            if(!Context()||java::ScreenOpen()||clickgui::IsOpen()||!rotation::OwnsExclusiveRotations(this)){Stop("interrupted");return;}
            if(!java::PlayerRotation(control_.yaw,control_.pitch)){Stop("camera unavailable");return;}
        }
        // The bounded planner evaluates many candidate angles. Read JNI once,
        // not again for every candidate and future waiting window.
        sensitivityValid_=java::PlayerSensitivity(sensitivity_);
        ++age_;if(age_>360){Stop("timed out");return;}
        if(targeted_&&phase_!=Phase::Hold&&!ReadTarget(phase_==Phase::Pearl)){
            if(phase_==Phase::Pearl){
                // No item has been used: fall back to the ordinary launch.
                FallbackLaunch("target lost before launch");
            }else{
                // Already committed: keep the last XZ goal, stop extrapolating
                // a missing/teleported target, and finish catching our pearl.
                targetMotion_.velocity={};
            }
        }
        if(phase_==Phase::Pearl) {
            Vec3 eye,inherited;bool forecast=false;
            const bool snapshot=LaunchSnapshot(eye,inherited,forecast);
            auto desired=snapshot?PrimeAngle(inherited,control_.yaw):rotation::Rotation{control_.yaw,-90};
            if(targeted_){
                if(!snapshot||!ReadyToLaunch(inherited))return;
                if(!TargetPearlAngle(eye,inherited,TargetGoal(),desired,highArc_?40.:20.)){
                    if(!highArc_){highArc_=true;return;}
                    FallbackLaunch("target XZ is not reachable");
                    desired=PrimeAngle(inherited,control_.yaw);
                }
            }
            if(!Aim(desired)){Stop("rotation unavailable");return;}
        } else if(phase_==Phase::Track) {
            if(TrackPearl())phase_=Phase::Wind;
            else if(age_-sentAt_>30){Stop("pearl was not confirmed");return;}
        }
        if(phase_==Phase::Wind) {
            if(age_-sentAt_>kPearlMaxAge){Stop("target height has no safe interception window");return;}
            auto entity=java::EntityById(pearlId_);
            if(!entity){Stop("pearl disappeared");return;}
        } else if(phase_==Phase::Hold&&age_>sentAt_) {
            Stop();
        }
        // TrackPearl must identify our pearl BEFORE building collision blockers.
        if(phase_==Phase::Pearl||phase_==Phase::Wind)RefreshBlockers();
    }
    void Interact() {
        if(!requested_||!working||!SameWorld()||java::ScreenOpen()||clickgui::IsOpen()||!Context()||
           !rotation::OwnsExclusiveRotations(this))return;
        if(!rotation::CanSubmitItemUse()){Waiting("interaction already committed");return;}
        Vec3 eye,inherited;bool forecast=false;
        if(!LaunchSnapshot(eye,inherited,forecast)){Waiting("movement snapshot unavailable");return;}
        if((phase_==Phase::Pearl||phase_==Phase::Wind)&&LandingTransition(eye,inherited)){
            Waiting("landing changes launch velocity");return;
        }
        if(phase_==Phase::Pearl) {
            if(!targeted_&&angle_.pitch> -89.5f)return;
            if(!ReadyToLaunch(inherited)){Waiting("waiting for ascent to finish");return;}
            if(targeted_){
                if(!ReadTarget(true)){FallbackLaunch("target unavailable at interaction");return;}
                rotation::Rotation desired;
                if(!TargetPearlAngle(eye,inherited,TargetGoal(),desired,highArc_?40.:20.)){
                    if(!highArc_)highArc_=true;
                    else FallbackLaunch("target arc changed before submission");
                    return;
                }
                desired=Quantize(desired);
                if(std::abs(std::remainder(double(angle_.yaw)-desired.yaw,360.))>.3||std::abs(angle_.pitch-desired.pitch)>.3)return;
                if(launchCheckTick_==age_)return;launchCheckTick_=age_;
                if(!TargetLaunchReady(eye,inherited,TargetGoal(),angle_,speed_->get(),sensitivity_,sensitivityValid_,
                    [&](auto a,auto b){return Clear(a,b);})){ 
                    if(!highArc_){highArc_=true;Waiting("checking alternate target arc");return;}
                    FallbackLaunch("no reliable pearl / wind path above target");return;
                }
            }else if(!Clear(eye,eye.Add(0,4,0))){Stop("not enough overhead space");return;}
            double eyeHeight=0;if(!player_.EyeHeight(eyeHeight))return;
            const double configured=height_->get();
            const double height=std::isfinite(configured)?std::clamp(configured,5.,45.):25.;
            const auto origin=eye.Add(0,-kPearlEyeOffset,0);
            targetY_=targeted_?TargetGoal().point.y:eye.y-eyeHeight+height;
            // Height is an optimization goal, not permission to use the items.
            // A falling launch may have a lower apex; select its closest safe
            // intercept after observing the pearl instead of cancelling here.
            if(!java::ChargeSlotReady(pearlSlot_,java::KnownItem::EnderPearl)||
               !java::ChargeSlotReady(windSlot_,java::KnownItem::WindCharge)||!java::SetSilentSlot(this,pearlSlot_)){
                Stop("items unavailable");return;
            }
            // Advance BEFORE submission: a delayed entity or ambiguous native
            // result must never cause a second pearl to be thrown.
            phase_=Phase::Track;sentAt_=age_;launch_=origin;
            slotHandoff_.BeforeUse();
            if(!java::UseSilentItem(java::KnownItem::EnderPearl,angle_.yaw,angle_.pitch)){Stop("pearl use failed");return;}
            LogTo("AutoCharge: pearl submitted age=%d targetY=%.3f inherited=(%.3f,%.3f,%.3f)",age_,targetY_,inherited.x,inherited.y,inherited.z);
        } else if(phase_==Phase::Wind) {
            Pearl pearl;
            if(!ReadPearl(pearl)){Waiting("pearl snapshot alignment");return;}
            // A repeated callback must not recompute against its own new angle
            // and spend a second smoothing budget or silently change the plan.
            if(windAimTick_==age_)return;
            windAimTick_=age_;
            if(targeted_&&!ReadTarget(false))targetMotion_.velocity={};
            const auto goal=TargetGoal();
            if(targeted_)targetY_=goal.point.y;
            const auto plan=PlanWind(pearl,eye,inherited,targetY_,age_-sentAt_,angle_,speed_->get(),
                sensitivity_,sensitivityValid_,[&](auto a,auto b){return Clear(a,b);},forecast,targeted_?&goal:nullptr,true);
            if(plan.decision==WindDecision::NoSolution){Waiting("no interception angle");return;}
            if(!rotation::SetExclusiveRotations(this,plan.angle)){Stop("rotation unavailable");return;}
            angle_=rotation::GetRotation();
            if(plan.decision!=WindDecision::Fire){Waiting(plan.decision==WindDecision::Waiting?
                "better interception window ahead":"angle, collision or spread not ready");return;}
            // RotationManager may rebase yaw by a complete revolution. Validate
            // its actual published value before issuing either item or packet.
            HeightImpact impact;
            if(!InterceptReady(pearl,eye,inherited,angle_,[&](auto a,auto b){return Clear(a,b);},impact))return;
            if(!java::ChargeSlotReady(windSlot_,java::KnownItem::WindCharge)||!java::SetSilentSlot(this,windSlot_)){
                Stop("wind charge unavailable");return;
            }
            if(targeted_)LogTo("AutoCharge: XZ plan target=%d horizontalError=%.3f predictedY=%.3f delayTicks=%d flightTicks=%d catchFallback=%d",
                targetId_,goal.HorizontalError(impact),impact.destination.y,age_-sentAt_,impact.ticks,plan.targetFallback);
            else LogTo("AutoCharge: height plan targetY=%.3f predictedY=%.3f error=%.3f delayTicks=%d flightTicks=%d",
                targetY_,impact.destination.y,impact.destination.y-targetY_,age_-sentAt_,impact.ticks);
            InterceptQuality quality;StableIntercept(pearl,eye,inherited,angle_,&quality);
            LogTo("AutoCharge: intercept yaw=%.3f pitch=%.3f margin=%.4f spreadMiss=%d/128 edgeMiss=%d/26 phaseMiss=%d/2 phaseSpreadMiss=%d/256 worstPhaseMiss=%d/128 inherited=(%.4f,%.4f,%.4f)",
                angle_.yaw,angle_.pitch,quality.margin,quality.misses,quality.edgeMisses,quality.phaseMisses,quality.phaseSpreadMisses,quality.worstPhaseMisses,inherited.x,inherited.y,inherited.z);
            phase_=Phase::Hold;sentAt_=age_;
            observation_.Begin(player_,world_,pearlId_,eye,Add(Look(angle_).Scale(kLaunchSpeed),inherited),impact.destination);
            slotHandoff_.BeforeUse();
            if(!java::UseSilentItem(java::KnownItem::WindCharge,angle_.yaw,angle_.pitch))Stop("wind charge use failed");
        }
    }
    Properties::NumberProp *speed_{},*height_{},*targetFov_{},*targetRange_{};
    Properties::BoolProperty* targetPlayer_{};
    event::Subscription tick_,interact_,respawn_,click_,packet_,input_;
    std::mutex movementMutex_;
    MovementHistory movement_;
    std::atomic<bool> requested_{false},reset_{false};
    Phase phase_=Phase::Idle;
    java::EntityHandle player_,world_;
    java::EntityHandle target_;
    TargetMotion targetMotion_;
    bool targeted_=false,highArc_=false;
    int targetId_=-1,launchCheckTick_=-1;
    rotation::Rotation control_{},angle_{};
    Vec3 launch_{};std::unordered_set<int> oldPearls_;std::vector<java::Box> blockers_;
    bool blockersValid_=false;
    bool sensitivityValid_=false;
    double sensitivity_=0;
    SlotHandoff slotHandoff_;
    FlightObservation observation_;
    int pearlSlot_=-1,windSlot_=-1,pearlId_=-1,age_=0,sentAt_=0,windAimTick_=-1,lastWaitLog_=-1;
    double targetY_=0;
};
}
namespace autocharge {
bool IsWorking(){return working.load();}
void YieldForRescue(){if(instance)instance->InterruptForRescue();}
}
std::unique_ptr<Module> MakeAutoChargeModule(){return std::make_unique<AutoCharge>();}
}
