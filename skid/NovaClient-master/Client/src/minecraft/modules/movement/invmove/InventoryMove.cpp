#include "minecraft/modules/movement/invmove/InventoryMove.h"
#include "minecraft/modules/movement/invmove/WatchdogAccess.h"
#include "minecraft/modules/movement/invmove/WatchdogState.h"
#include "minecraft/modules/movement/invmove/ModeState.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/player/invmanager/InvManager.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_options.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "ui/clickgui/ClickGui.h"
#include "utils/Keyboard.h"
#include <atomic>
#include <cmath>
#include <memory>
#include <mutex>
#include <vector>

namespace nova::module {
namespace {
std::atomic<bool> suppressSprint{false};
class InventoryMove;
InventoryMove* runtime=nullptr;
class InventoryMove final:public Module {
public:
    InventoryMove():Module("InvMove",Category::Movement,"General","General"),
        mode_(addMode("Mode",invmove::Normal,{"Normal","Buffer Abuse","Cancel","Grim","Grim 2","Watchdog"})),
        clicks_(addNumber("Clicks",2,10,1,3,{},Integer,[this]{return mode_.get()==invmove::BufferAbuse;})),
        amount_(addNumber("Amount",1,10,1,5,{},Integer,[this]{return mode_.get()==invmove::BufferAbuse;})),
        extra_(addNumber("Manager Extra Sprint Ticks",0,20,1,9,{},Integer,[this]{return mode_.get()==invmove::Grim;})),
        prediction_(addBool("Prediction Mode",false,{},[this]{return Watchdog();})),
        ticks_(addNumber("Ticks",1,20,1,1,{},Integer,[this]{return Watchdog();})),
        measure_(addBool("Measure Chest Open",true,{},[this]{return Watchdog();})){
        runtime=this;
        auto& bus=event::EventBus::Get();
        tick_=bus.Subscribe<event::PlayerTickEvent>([this](auto&){Update();},event::Priority::Highest);
        input_=bus.Subscribe<event::MoveInputEvent>([this](auto& e){Input(e);},event::Priority::Highest);
        motion_=bus.Subscribe<event::MotionEvent>([this](auto& e){if(e.type==event::EventType::Pre)Motion();},event::Priority::Low);
        strafe_=bus.Subscribe<event::StrafeEvent>([this](auto&){
            if(suppressSprint)java::SetPlayerSprinting(false);
        },event::Priority::Lowest);
        packet_=bus.Subscribe<event::PacketEvent>([this](auto& e){Packet(e);},event::Priority::Lowest);
        respawn_=bus.Subscribe<event::RespawnEvent>([this](auto&){Reset();});
    }
    ~InventoryMove()override{ClearQueue(false);runtime=nullptr;suppressSprint=false;}
    void ManagerClicked(){if(Running()&&mode_.get()==invmove::Grim)++grim_.clicks;}
    bool BlockAcceleration(){std::lock_guard lock(queueMutex_);return Running()&&mode_.get()==invmove::BufferAbuse&&buffer_.holding(int(clicks_.get()));}
    bool ZeroHorizontalDisplacement()const{return Working()&&!prediction_.get()&&!java::PlayerOnGround();}
    bool PrepareManagerClick(){
        if(!invmanager::UsesInventoryMovement()||!input::IsProcessForeground()||clickgui::CapturesInput())return false;
        invmove::access::View view;if(!invmove::access::Read(view)||(view.open&&!view.inventory))return false;
        managerUntil_=java::PlayerTickCount()+4;
        return !Watchdog()||(active_&&state_.clicking&&state_.delayPassed);
    }
    void ReleaseManager(){
        managerUntil_=-1;
        if(managerWasActive_){
            invmove::access::View view;int id=0;
            if(lastMode_==invmove::Watchdog&&invmove::access::Read(view)&&(!view.open||view.inventory)&&java::PlayerMenuContainerId(id))java::SendContainerClose(id);
            managerWasActive_=false;state_={};suppressSprint=false;
        }
    }
private:
    static std::string Integer(double n){return std::to_string(int(n));}
    bool Watchdog()const{return mode_.get()==invmove::Watchdog;}
    bool Running()const{return isEnabled()||invmanager::UsesInventoryMovement();}
    bool ManagerSession()const{return invmanager::UsesInventoryMovement()&&managerUntil_>=java::PlayerTickCount();}
    bool Working()const{return Watchdog()&&Running()&&active_&&(view_.chest||state_.clicking);}
    void ClearQueue(bool replay){
        std::vector<jobject> packets;
        {std::lock_guard lock(queueMutex_);packets.swap(queued_);buffer_={};}
        JniAttach attach;if(!attach)return;
        for(auto packet:packets){if(replay)java::SendPacketNoEvent(packet);attach.env->DeleteGlobalRef(packet);}
    }
    void BufferMotion(){
        std::lock_guard lock(queueMutex_);
        if(!buffer_.motion(int(clicks_.get())))return;
        invmove::access::Frame f;if(!f)return;
        for(int i=0;i<int(amount_.get());++i){
            auto empty=f.create(invmove::access::mapping::emptyClick);
            if(!empty||!f.check())return;
            java::SendPacketNoEvent(empty);
        }
        for(auto packet:queued_){java::SendPacketNoEvent(packet);f.env->DeleteGlobalRef(packet);}
        queued_.clear();buffer_.flushed=true;
    }
    void Reset(){ClearQueue(false);grim_={};state_={};view_={};keys_={};active_=false;managerUntil_=-1;managerWasActive_=false;forward_=strafeValue_=0;groundTicks_=0;lastTick_=-1;clicked_=false;attempt_=false;inventoryPacket_=false;suppressSprint=false;player_.Reset();world_.Reset();}
    void onDisable()override{if(!invmanager::UsesInventoryMovement()){ClearQueue(true);Reset();java::RestoreMovementKeys();}}
    void onEnable()override{if(!invmanager::UsesInventoryMovement())Reset();}
    void Stop(){rotation::Vec3 v;if(java::PlayerDeltaMovement(v))java::SetPlayerDeltaMovement(0,v.y,0);}
    static float Direction(float yaw,float forward,float strafe,float angle){
        if(forward<0)yaw+=180;
        const float factor=forward<0?-.5f:forward>0?.5f:1.f;
        if(strafe>0)yaw-=angle*factor;if(strafe<0)yaw+=angle*factor;return yaw;
    }
    void Update(){
        if(!Running()){ReleaseManager();ClearQueue(true);if(active_)Reset();return;}
        auto player=java::PlayerEntity(),world=java::LevelEntity();
        if(!player||!world){Reset();return;}
        if(!player_||!world_||!player.IsSame(player_)||!world.IsSame(world_)){Reset();player_=std::move(player);world_=std::move(world);}
        if(lastMode_!=mode_.get()){ClearQueue(true);ReleaseManager();Reset();lastMode_=mode_.get();player_=java::PlayerEntity();world_=java::LevelEntity();}
        const int tick=java::PlayerTickCount();if(tick==lastTick_)return;lastTick_=tick;
        groundTicks_=java::PlayerOnGround()?groundTicks_+1:0;
        view_={};const bool readable=invmove::access::Read(view_);
        const bool manager=ManagerSession()&&readable&&(!view_.open||view_.inventory);
        if(managerWasActive_&&!manager){int id=0;if(Watchdog()&&readable&&(!view_.open||view_.inventory)&&java::PlayerMenuContainerId(id))java::SendContainerClose(id);state_={};}
        managerWasActive_=manager;
        active_=input::IsProcessForeground()&&!clickgui::CapturesInput()&&readable&&(view_.open||manager||mode_.get()==invmove::Cancel||mode_.get()==invmove::BufferAbuse)&&!view_.chat;
        if(!Watchdog()){
            const bool extra=mode_.get()==invmove::Grim&&grim_.update(int(extra_.get()));
            suppressSprint=(mode_.get()==invmove::Grim||mode_.get()==invmove::Grim2)&&(view_.inventory||view_.chest||extra);
            if(suppressSprint)java::SetPlayerSprinting(false);
            if(active_&&!java::ReadInventoryMoveKeys(keys_))active_=false;
            return;
        }
        const bool clicked=clicked_.exchange(false);
        if(tick>=50)state_.Tick(tick,view_.inventory||manager,view_.chest,clicked||manager,attempt_.exchange(false),int(ticks_.get()));
        else if(!view_.inventory)state_.clicking=false;else if(clicked)state_.clicking=true;
        suppressSprint=Working();
        if(!active_)return;
        if(!java::ReadInventoryMoveKeys(keys_)){active_=false;suppressSprint=false;return;}
        if(Working()){
            java::SetPlayerSprinting(false);java::SetSprintKeyDown(false);
            if(tick>=50&&keys_.directions>1){float yaw=0,pitch=0;if(java::PlayerRotation(yaw,pitch))rotation::SetRotations({Direction(yaw,keys_.forward,keys_.strafe,70),pitch},10.0);}
        }
    }
    void Input(event::MoveInputEvent& e){
        if(!Running()||!active_)return;
        if(!Watchdog()){e.forward=keys_.forward;e.strafe=keys_.strafe;e.jump=keys_.jump;return;}
        if(view_.inventory&&clicked_.exchange(false))state_.clicking=true;
        suppressSprint=Working();
        e.forward=keys_.forward;e.strafe=keys_.strafe;e.jump=keys_.jump;
        if(Working()&&prediction_.get()&&!view_.speedLevel){e.jump=false;invmove::access::ClearJump();}
        if(state_.BlockInput(java::PlayerTickCount(),GetTickCount64(),view_.chest,prediction_.get(),view_.speedLevel>0,java::PlayerOnGround()))e.forward=e.strafe=0;
        forward_=e.forward;strafeValue_=e.strafe;
    }
    void Motion(){
        if(Running()&&mode_.get()==invmove::BufferAbuse){BufferMotion();return;}
        if(!Watchdog())return;
        if(!Running()||!active_)return;
        // Rise only sends this while no inventory action has occurred since C03.
        if(view_.inventory&&!state_.clicking&&!clicked_.load()&&!inventoryPacket_.load()){
            int id=0;if(java::PlayerMenuContainerId(id))java::SendContainerClose(id);
        }
        if(!Working())return;
        java::SetPlayerSprinting(false);java::SetSprintKeyDown(false);
        if(prediction_.get())return;
        const bool ground=java::PlayerOnGround();
        rotation::Vec3 pos,v;if(!player_.Position(pos)||!java::PlayerDeltaMovement(v))return;
        double speed=.09;
        if(groundTicks_<10&&std::abs(pos.y-std::round(pos.y))<=.03&&!view_.chest)speed=.0365;
        else if(!ground)speed=0;
        else if(view_.speedLevel>1)speed=.0185*view_.speedLevel;
        else if(view_.speedLevel==1)speed=.0635;
        if(view_.jumping)speed=0;
        if(forward_==0&&strafeValue_==0){if(speed==0)Stop();return;}
        // Rise applies a separate diagonal correction after setting strafe speed.
        if(keys_.directions!=1)speed-=ground?.0026000750109401644:.0005199896488849598;
        float yaw=0,pitch=0;if(!java::PlayerRotation(yaw,pitch))return;
        const double radians=Direction(yaw,forward_,strafeValue_,90)*3.14159265358979323846/180;
        java::SetPlayerDeltaMovement(-std::sin(radians)*speed,v.y,std::cos(radians)*speed);
    }
    void Packet(event::PacketEvent& e){
        if(!Running()||e.type!=event::EventType::Outgoing||e.IsCancelled())return;
        if(mode_.get()==invmove::Normal||mode_.get()==invmove::Grim||mode_.get()==invmove::Grim2)return;
        invmove::access::PacketFacts facts;if(!invmove::access::ReadPacket(e.packet,facts))return;
        if(mode_.get()==invmove::Cancel){if(facts.open||facts.close)e.Cancel();return;}
        if(mode_.get()==invmove::BufferAbuse){
            if(facts.click){
                std::lock_guard lock(queueMutex_);
                if(buffer_.holding(int(clicks_.get()))){
                    JniAttach attach;if(!attach)return;
                    auto kept=attach.env->NewGlobalRef(e.packet);
                    if(kept){queued_.push_back(kept);e.Cancel();}
                    else if(attach.env->ExceptionCheck())attach.env->ExceptionClear();
                }else buffer_.click(int(clicks_.get()));
            }
            return;
        }
        if(!Watchdog())return;
        if(facts.move)inventoryPacket_=false;
        if(facts.inventory)inventoryPacket_=true;
        if(facts.clicked)clicked_=true;
        if(facts.openAttempt&&measure_.get())attempt_=true;
    }
    Properties::ModeProperty& mode_;
    Properties::NumberProp &clicks_,&amount_,&extra_;
    Properties::BoolProperty& prediction_;
    Properties::NumberProp& ticks_;
    Properties::BoolProperty& measure_;
    invmove::WatchdogState state_;
    invmove::BufferState buffer_;
    invmove::GrimState grim_;
    std::mutex queueMutex_;
    std::vector<jobject> queued_;
    int lastMode_=-1;
    invmove::access::View view_;
    java::InventoryMoveKeys keys_;
    java::EntityHandle player_,world_;
    bool active_=false;
    int managerUntil_=-1;
    bool managerWasActive_=false;
    int groundTicks_=0,lastTick_=-1;
    float forward_=0,strafeValue_=0;
    std::atomic<bool> clicked_{false},attempt_{false},inventoryPacket_{false};
    event::Subscription tick_,input_,motion_,strafe_,packet_,respawn_;
};
}
namespace invmove {
bool SuppressSprint(){return suppressSprint.load();}
bool PrepareManagerClick(){return runtime&&runtime->PrepareManagerClick();}
void ManagerClicked(){if(runtime)runtime->ManagerClicked();}
bool BlockAcceleration(){return runtime&&runtime->BlockAcceleration();}
bool ZeroHorizontalDisplacement(){return runtime&&runtime->ZeroHorizontalDisplacement();}
void ReleaseManager(){if(runtime)runtime->ReleaseManager();}
}
std::unique_ptr<Module> MakeInventoryMoveModule(){return std::make_unique<InventoryMove>();}
}
