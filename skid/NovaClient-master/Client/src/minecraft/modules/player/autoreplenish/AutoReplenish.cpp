#include "minecraft/modules/player/autoreplenish/AutoReplenish.h"
#include "minecraft/modules/Module.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_options.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_player.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/inventory/InventoryUtils.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/modules/combat/autototem/AutoTotem.h"
#include "minecraft/modules/player/refill/Refill.h"
#include "minecraft/modules/player/cheststealer/ChestStealer.h"
#include "ui/clickgui/ClickGui.h"
#include "utils/Keyboard.h"
#include <array>
#include <atomic>
#include <format>

namespace nova::module {
namespace {
std::atomic<bool> working{false},suppressMovement{false};
class AutoReplenish final : public Module {
public:
    AutoReplenish():Module("AutoReplenish",Category::Player,"General","General") {
        delay_=&addNumber("Delay",0,1000,5,0,{},[](double v){return std::format("{:.0f} ms",v);});
        grim_=&addBool("Grim",false);
        auto& bus=event::EventBus::Get();
        tick_=bus.Subscribe<event::GameTickEvent>([this](auto& e){if(e.type==event::EventType::Pre)Tick();},event::Priority::Highest-2);
        input_=bus.Subscribe<event::MoveInputEvent>([this](auto& e){
            if(!suppressMovement)return;
            e.forward=e.strafe=e.finalForward=e.finalStrafe=0;
            e.hasFinalForward=e.hasFinalStrafe=true;e.jump=e.sneak=false;
            e.skipRotationFix=e.suppressSprint=true;inputTick_=tickId_;
            java::SetSprintKeyDown(false);java::SetPlayerSprinting(false);
        },event::Priority::Lowest+11);
        motion_=bus.Subscribe<event::MotionEvent>([this](auto& e){
            if(!suppressMovement)return;
            if(e.type==event::EventType::Pre)java::SetPlayerSprinting(false);
            else if(inputTick_==tickId_)neutralTick_=tickId_;
        },event::Priority::Lowest+11);
        strafe_=bus.Subscribe<event::StrafeEvent>([](auto&){if(suppressMovement)java::SetPlayerSprinting(false);},event::Priority::Lowest+11);
    }
    ~AutoReplenish()override{tick_.Reset();input_.Reset();motion_.Reset();strafe_.Reset();Finish(false);}
private:
    struct Slot {java::EntityHandle identity;long long missing=-1,retry=0;};
    void onEnable()override{reset_=true;enabled_=true;}
    void onDisable()override{enabled_=false;}
    void Clear(){for(auto& slot:slots_)slot=Slot{};}
    void Finish(bool resume){
        const bool paused=suppressMovement.exchange(false);
        rotation::ReleaseAttack(this);working=false;phase_=0;destination_=-1;
        if(!paused)return;
        auto player=java::PlayerEntity(),world=java::LevelEntity();java::FreeCamKeys keys;
        const bool forward=resume&&player&&world&&player.IsSame(player_)&&world.IsSame(world_)&&wasSprinting_&&
            input::IsProcessForeground()&&!clickgui::IsOpen()&&java::ReadFreeCamKeys(keys)&&keys.forward>0&&!keys.down;
        java::RestoreUseKey();java::RestoreSprintKey(forward);resumeKey_=forward;
    }
    // Never retain the live stack: consuming the last item mutates it to empty.
    bool Read(int index,java::EntityHandle& identity,bool& empty){
        JniAttach attach;if(!attach)return false;
        jobject stack=nullptr,copy=nullptr;
        bool ok=java::InventorySlot(index,stack)&&java::ReadItemIdentity(stack,copy,empty);
        if(ok&&!empty){identity=java::AdoptEntity(copy);ok=bool(identity);}
        if(copy)attach->DeleteLocalRef(copy);if(stack)attach->DeleteLocalRef(stack);return ok;
    }
    bool Same(const java::EntityHandle& a,const java::EntityHandle& b){
        return a&&b&&java::ItemStacksMatch(static_cast<jobject>(a.raw()),static_cast<jobject>(b.raw()));
    }
    bool Observe(long long now){
        for(int i=0;i<9;++i){java::EntityHandle item;bool empty=false;
            if(!Read(i,item,empty))return false;
            auto& slot=slots_[i];
            if(!empty){
                if(!Same(slot.identity,item))slot.identity=std::move(item);
                slot.missing=-1;slot.retry=0;
            }else if(slot.identity&&slot.missing<0)slot.missing=now;
        }return true;
    }
    void Tick(){
        ++tickId_;
        if(resumeKey_){java::RestoreSprintKey();resumeKey_=false;}
        if(!enabled_||compat::IsLegacyLunar()){Finish(true);Clear();return;}
        auto player=java::PlayerEntity(),world=java::LevelEntity();
        if(!player||!world||!player.IsAlive()||player.IsSpectator()){Finish(false);Clear();return;}
        if(reset_.exchange(false)||!player.IsSame(player_)||!world.IsSame(world_)){
            Finish(false);Clear();player_=std::move(player);world_=std::move(world);
        }
        if(phase_==2){Finish(true);return;}
        if(phase_&&!grim_->get())Finish(true);
        const auto now=inventory::NowMillis();
        JniAttach attach;if(!attach){Finish(true);return;}
        auto screen=java::CurrentScreen();const bool screenOpen=screen!=nullptr;
        if(screen)attach->DeleteLocalRef(screen);
        // Manual inventory changes establish a new baseline; they aren't depletion.
        const bool organizer=autototem::IsWorking()||refill::IsWorking()||cheststealer::IsWorking();
        if(screenOpen||organizer||!java::InventoryCursorEmpty()){
            Finish(true);Clear();return;
        }
        if(!Observe(now)){Finish(true);Clear();return;}
        if(clickgui::IsOpen()||!input::IsProcessForeground()||java::FreeCamActive()||java::PlayerUsingItem()){
            Finish(true);return;
        }
        int menu=-1;if(!java::PlayerMenuContainerId(menu)){Finish(true);return;}
        int dest=-1,source=-1;
        for(int d=0;d<9&&source<0;++d){auto& slot=slots_[d];
            if(!slot.identity||slot.missing<0||now-slot.missing<delay_->get()||now<slot.retry)continue;
            for(int s=9;s<36;++s){java::EntityHandle item;bool empty=false;
                if(!Read(s,item,empty)){Finish(true);return;}
                if(!empty&&Same(slot.identity,item)){dest=d;source=s;break;}
            }
        }
        if(source<0){Finish(true);return;}
        if(phase_&&destination_!=dest)Finish(true);
        if(grim_->get()){
            if(!compat::UsesModernRuntime()||!rotation::AcquireAttack(this)){Finish(true);return;}
            if(!phase_){
                if(!java::PlayerSprinting(wasSprinting_)){Finish(false);return;}
                phase_=1;destination_=dest;started_=tickId_;inputTick_=neutralTick_=-1;suppressMovement=true;
            }
            working=true;java::SetSprintKeyDown(false);java::SuppressUseKey();
            if(!java::SetPlayerSprinting(false)){Finish(true);return;}
            if(tickId_-started_>4){slots_[dest].retry=now+250;Finish(true);return;}
            if(neutralTick_<started_||neutralTick_>=tickId_)return;
            bool sprinting=true;if(!java::PlayerSprinting(sprinting)||sprinting)return;
        }
        // Recheck immediately before SWAP; never displace a newly occupied slot.
        java::EntityHandle destination,stock;bool empty=false,stockEmpty=false;
        if(!Read(dest,destination,empty)||!empty||!Read(source,stock,stockEmpty)||stockEmpty||
           !Same(slots_[dest].identity,stock)||!java::InventoryCursorEmpty()){Finish(true);return;}
        working=true;
        // Main inventory indices 9..35 equal player-menu slots; SWAP button 0..8
        // chooses the original hotbar slot without changing the selected hand.
        java::ClickMenuSlot(menu,source,dest,java::ClickKind::Swap);
        slots_[dest].retry=now+250;
        if(grim_->get())phase_=2;
    }
    Properties::NumberProp* delay_{};Properties::BoolProperty* grim_{};
    std::array<Slot,9> slots_;
    java::EntityHandle player_,world_;
    std::atomic<bool> enabled_{false},reset_{true};
    int phase_=0,tickId_=0,started_=0,inputTick_=-1,neutralTick_=-1,destination_=-1;
    bool wasSprinting_=false,resumeKey_=false;
    event::Subscription tick_,input_,motion_,strafe_;
};
}
namespace autoreplenish {
bool IsWorking(){return working.load();}
bool SuppressMovement(){return suppressMovement.load();}
}
std::unique_ptr<Module> MakeAutoReplenishModule(){return std::make_unique<AutoReplenish>();}
}
