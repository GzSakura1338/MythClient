#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <format>
#include <limits>
#include <memory>
#include <mutex>
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/combat/antikb/AntiKB.h"
#include "minecraft/modules/combat/backtrack/Backtrack.h"
#include "minecraft/modules/combat/backtrack/EspClip.h"
#include "minecraft/modules/combat/velocity/Velocity.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_backtrack.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/game_chest.h"
#include "minecraft/host/render/WorldProjection.h"
#include "ui/hud/world/ScreenGeometry.h"

namespace nova::module {
namespace {
std::atomic<bool> g_lagging{false};
using rotation::Vec3;
using java::Box;
using Clock = std::chrono::steady_clock;
long long Now() { return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count(); }
constexpr long long kAttackLifetime = 750;
constexpr long long kWindowLimit = 500;
constexpr long long kRecovery = 150;
constexpr std::size_t kQueueLimit = 256;

double DistanceSq(const Box& box, const Vec3& point) {
    const double x=point.x-std::clamp(point.x,box.minX,box.maxX);
    const double y=point.y-std::clamp(point.y,box.minY,box.maxY);
    const double z=point.z-std::clamp(point.z,box.minZ,box.maxZ);
    return x*x+y*y+z*z;
}
Box Offset(Box b,const Vec3& offset) {
    b.minX+=offset.x;b.maxX+=offset.x;b.minY+=offset.y;b.maxY+=offset.y;b.minZ+=offset.z;b.maxZ+=offset.z;return b;
}
bool BlocksBacktrack() { return antikb::BlocksBacktrack() || velocity::IsDelaying() || velocity::AttackQueue()>0; }

class BacktrackModule final : public Module {
public:
    BacktrackModule() : Module("Backtrack",Category::Combat,"General","General") {
        delay_ = &addNumber("Delay", 0, 200, 10, 100, {},
            [](double value) { return std::format("{:.0f} ms", value); });
        const auto distance=[](double value){return std::format("{:.1f} blocks",value);};
        auto maxRange=subNumber("Max Range",0,10,0.1,6,distance);
        maxRange_=static_cast<Properties::NumberProp*>(maxRange.getProp());
        minRange_=&addNumber("Min Range",0,10,0.1,0,{maxRange},distance);
        esp_ = &addBool("ESP", true);
        enableOverlay();
        auto& bus=event::EventBus::Get();
        attack_=bus.Subscribe<event::AttackEntityEvent>([this](auto& e){OnAttack(e);});
        tick_=bus.Subscribe<event::GameTickEvent>([this](auto& e){OnTick(e);},event::Priority::High);
        // Release the preceding stream before AntiKB sees the triggering velocity.
        packet_=bus.Subscribe<event::PacketEvent>([this](auto& e){OnPacket(e);},event::Priority::Highest);
    }
    ~BacktrackModule() override {
        attack_.Reset();tick_.Reset();packet_.Reset();
        std::lock_guard lock(mutex_);
        if(active_)java::SetIncomingPacketsWanted(false);
        active_=false;Stop(false,false);
    }
    std::string suffix() const override {
        std::lock_guard lock(mutex_);
        return holding_ ? std::format("{} ms", delayMs_) : "";
    }
private:
    struct Queued { jobject packet; long long time; };
    // Every queue mutation and replay submission shares this lock. Recursive
    // replay callbacks must not deadlock or overtake the preceding packet.
    mutable std::recursive_mutex mutex_;
    std::deque<Queued> queue_;
    bool replaying_ = false;

    void Drain(bool replay, long long before=std::numeric_limits<long long>::max()) {
        if(queue_.empty())return;
        JniAttach attach;if(!attach)return;
        const bool wasReplaying=replaying_;replaying_=true;
        while(!queue_.empty()&&queue_.front().time<=before) {
            const auto item=queue_.front();queue_.pop_front();
            if(replay)java::HandleIncomingPacket(item.packet);
            attach.env->DeleteGlobalRef(item.packet);
        }
        replaying_=wasReplaying;
        g_lagging.store(active_&&!queue_.empty());
    }
    void Stop(bool replay,bool recovery=true) {
        holding_=false;geometryValid_=false;targetId_=0;target_.Reset();
        if(recovery)nextAllowed_=Now()+kRecovery;
        Drain(replay);
    }
    void onEnable() override {
        std::lock_guard lock(mutex_);
        Stop(false,false);world_.Reset();pendingTarget_.Reset();pendingAttack_=false;active_=true;nextAllowed_=0;
        delayMs_=std::clamp(static_cast<int>(delay_->get()),0,200);
        UpdateRange();
        java::SetIncomingPacketsWanted(true);
    }
    void onDisable() override {
        std::lock_guard lock(mutex_);
        active_=false;java::SetIncomingPacketsWanted(false);Stop(true,false);world_.Reset();pendingTarget_.Reset();pendingAttack_=false;
    }
    void UpdateRange() {
        const double a=std::clamp(minRange_->get(),0.0,10.0),b=std::clamp(maxRange_->get(),0.0,10.0);
        const double min=std::min(a,b),max=std::max(a,b);
        minRangeSq_=min*min;maxRangeSq_=max*max;
    }
    bool InRange(double squaredDistance) const {
        return squaredDistance+1e-6>=minRangeSq_&&squaredDistance<=maxRangeSq_+1e-6;
    }
    bool RefreshGeometry() {
        const auto self=java::PlayerEntity();
        if(!self||!self.IsAlive()||!target_||!target_.IsPlayer()||!target_.IsAlive()||target_.IsSame(self))return false;
        auto current=java::EntityById(targetId_);
        if(!current||!current.IsSame(target_))return false;
        geometryValid_=self.EyePosition(1,eye_)&&target_.BoundingBox(clientBox_)&&target_.Position(clientPos_)&&java::EntityId(self,selfId_);
        return geometryValid_&&InRange(DistanceSq(clientBox_,eye_));
    }
    bool WorldReady() {
        auto current=java::LevelEntity();
        if(!current||!java::PlayerEntity()) {Stop(false,false);world_.Reset();pendingTarget_.Reset();pendingAttack_=false;return false;}
        if(world_&&!world_.IsSame(current)) {Stop(false,false);pendingTarget_.Reset();pendingAttack_=false;}
        world_=std::move(current);return true;
    }
    void OnAttack(event::AttackEntityEvent& e) {
        std::lock_guard lock(mutex_);
        if(!active_||replaying_)return;
        // In 1.8 the controller hook is BETWEEN animation and attack. Replaying
        // a transaction here inserts its acknowledgement into that pair.
        // Only record intent; all switch/stop paths run at the next tick head.
        pendingTarget_=java::AdoptEntity(e.target);
        pendingAttackAt_=Now();pendingAttack_=true;
    }
    void ProcessAttack(java::EntityHandle enemy,long long attackedAt) {
        const auto now=Now();
        delayMs_=std::clamp(static_cast<int>(delay_->get()),0,200);
        if(BlocksBacktrack()||delayMs_==0) {Stop(true);return;}
        if(!enemy||!enemy.IsPlayer()||!enemy.IsAlive()) {Stop(true);return;}
        const bool same=target_&&target_.IsSame(enemy);
        if(!same)Stop(true,false);
        if(now<nextAllowed_||java::PlayerTickCount()<=10)return;
        if(!same) {
            target_=std::move(enemy);
            if(!java::EntityId(target_,targetId_)||!java::BacktrackBase(target_,tracked_)) {Stop(true);return;}
        }
        if(!RefreshGeometry()) {Stop(true);return;}
        lastAttack_=attackedAt;
    }
    void OnTick(event::GameTickEvent& e) {
        if(e.type!=event::EventType::Pre)return;
        std::lock_guard lock(mutex_);
        if(!active_||replaying_||!WorldReady())return;
        UpdateRange();
        if(pendingAttack_) {
            pendingAttack_=false;
            ProcessAttack(std::move(pendingTarget_),pendingAttackAt_);
            pendingTarget_.Reset();
        }
        const auto now=Now();
        delayMs_=std::clamp(static_cast<int>(delay_->get()),0,200);
        if(!target_)return;
        if(delayMs_==0||BlocksBacktrack()||now-lastAttack_>kAttackLifetime||
            (holding_&&now-windowStart_>=kWindowLimit)||!RefreshGeometry()) {Stop(true);return;}
        const double serverDistance=DistanceSq(Offset(clientBox_,tracked_.Subtract(clientPos_)),eye_);
        if(!InRange(serverDistance)|| (holding_&&serverDistance+0.0001<DistanceSq(clientBox_,eye_))) {Stop(true);return;}
        Drain(true,now-delayMs_);
        // When nothing is held, the vanilla codec is again the authoritative base.
        if(queue_.empty()&&!holding_)java::BacktrackBase(target_,tracked_);
    }
    void OnPacket(event::PacketEvent& e) {
        if(e.type!=event::EventType::Incoming||!e.packet)return;
        std::lock_guard lock(mutex_);
        if(!active_||replaying_)return;
        if(java::IsDisconnectPacket(e.packet)||java::IsRespawnPacket(e.packet)) {Stop(false,false);world_.Reset();pendingTarget_.Reset();pendingAttack_=false;return;}
        if(!target_)return;
        if(e.IsCancelled()||BlocksBacktrack()) {Stop(true);return;}
        const auto now=Now();
        if(delayMs_==0||now-lastAttack_>kAttackLifetime||
            (holding_&&now-windowStart_>=kWindowLimit)) {Stop(true);return;}
        java::BacktrackPacket info;
        if(!java::ReadBacktrackPacket(e.packet,targetId_,selfId_,tracked_,info)) {Stop(true);return;}
        if(info.resetWorld) {Stop(false,false);world_.Reset();pendingTarget_.Reset();pendingAttack_=false;return;}
        // Corrections, local knockback/explosions and death must not wait behind
        // the artificial delay. Flush first and let normal handlers own them.
        if(info.flush||info.deadHealth) {Stop(true);return;}
        if(info.movedTarget) {
            const double oldDistance=DistanceSq(Offset(clientBox_,tracked_.Subtract(clientPos_)),eye_);
            const double newDistance=DistanceSq(Offset(clientBox_,info.position.Subtract(clientPos_)),eye_);
            tracked_=info.position;
            if(!InRange(newDistance)||newDistance+0.0001<oldDistance) {Stop(true);return;}
            if(!holding_) {
                if(!geometryValid_||newDistance<=oldDistance+0.0001||now<nextAllowed_)return;
                holding_=true;windowStart_=now;
            }
        }
        if(!holding_)return;
        Drain(true,now-delayMs_);
        if(queue_.size()>=kQueueLimit) {Stop(true);return;}
        JniAttach attach;
        jobject kept=attach?attach.env->NewGlobalRef(e.packet):nullptr;
        if(!kept) {
            if(attach&&attach.env->ExceptionCheck())attach.env->ExceptionClear();
            Stop(true);return;
        }
        // Keep the whole ordered stream, including ping/transaction and chat.
        // Never fabricate acknowledgements or change outgoing movement timing.
        queue_.push_back({kept,now});g_lagging.store(true);e.Cancel();
    }
    void onRender() override {
        if(!ImGui::GetCurrentContext()||!esp_->get())return;
        Box box;
        {
            std::lock_guard lock(mutex_);
            if(!holding_||queue_.empty()||!geometryValid_)return;
            box=Offset(clientBox_,tracked_.Subtract(clientPos_));
        }
        const float partial=java::FrameTime();java::CameraView view;
        if(!java::ReadCameraView(partial,view))return;
        const auto basis=minecraft::projection::MakeBasis(view,partial);const auto display=ImGui::GetIO().DisplaySize;
        const std::array<Vec3,8> corners{{{box.minX,box.minY,box.minZ},{box.maxX,box.minY,box.minZ},
            {box.maxX,box.minY,box.maxZ},{box.minX,box.minY,box.maxZ},
            {box.minX,box.maxY,box.minZ},{box.maxX,box.maxY,box.minZ},
            {box.maxX,box.maxY,box.maxZ},{box.minX,box.maxY,box.maxZ}}};
        ui::ScreenGeometry geometry;
        backtrack::DrawClippedBox<ImVec2>(corners,
            [&](const Vec3& world,ImVec2& point) {
                minecraft::projection::ScreenPoint p;
                if(!minecraft::projection::Project(basis,world,display,p))return false;
                point={p.x,p.y};return true;
            },
            [&](auto& polygon){geometry.faces.push_back(std::move(polygon));},
            [&](ImVec2 a,ImVec2 b){geometry.edges.push_back({a,b});});
        ui::DrawGeometry(geometry,0x23FFEAE5u,0xBEFFEAE5u,1.5f);
    }
    Properties::NumberProp* delay_;
    Properties::NumberProp *minRange_,*maxRange_;
    double minRangeSq_=0,maxRangeSq_=36;
    Properties::BoolProperty* esp_;
    java::EntityHandle target_,world_,pendingTarget_;
    Vec3 tracked_,clientPos_,eye_;
    Box clientBox_{};
    bool active_=false,holding_=false,geometryValid_=false;
    int targetId_=0,selfId_=0,delayMs_=100;
    long long lastAttack_=0,windowStart_=0,nextAllowed_=0;
    bool pendingAttack_=false;
    long long pendingAttackAt_=0;
    event::Subscription attack_,tick_,packet_;
};
}
std::unique_ptr<Module> MakeBacktrackModule() {return std::make_unique<BacktrackModule>();}
namespace backtrack {bool IsLagging(){return g_lagging.load();}}
}
