#include "minecraft/modules/combat/autototem/AutoTotem.h"
#include "minecraft/modules/Module.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_options.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_player.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/compat/Profile.h"
#include "utils/Keyboard.h"
#include "minecraft/modules/player/refill/Refill.h"
#include "minecraft/modules/player/autoreplenish/AutoReplenish.h"
#include "minecraft/modules/player/cheststealer/ChestStealer.h"
#include "minecraft/utils/inventory/InventoryUtils.h"
#include "ui/clickgui/ClickGui.h"
#include <atomic>
#include <format>

namespace nova::module {
namespace {
std::atomic<bool> enabled{false},working{false};
std::atomic<bool> suppressMovement{false};
class AutoTotem final : public Module {
public:
    AutoTotem():Module("AutoTotem",Category::Combat,"General","General") {
        delay_=&addNumber("Delay",0,1000,5,0,{},[](double v){return std::format("{:.0f} ms",v);});
        grim_=&addBool("Grim",false);
        tick_=event::EventBus::Get().Subscribe<event::GameTickEvent>([this](auto& e){
            if(e.type==event::EventType::Pre)Tick();
        },event::Priority::Highest-3);
        input_=event::EventBus::Get().Subscribe<event::MoveInputEvent>([this](auto& e){
            if(!suppressMovement)return;
            e.forward=e.strafe=e.finalForward=e.finalStrafe=0;
            e.hasFinalForward=e.hasFinalStrafe=true;e.jump=e.sneak=false;
            e.skipRotationFix=e.suppressSprint=true;
            inputTick_=tickId_;
            java::SetSprintKeyDown(false);java::SetPlayerSprinting(false);
        },event::Priority::Lowest+10);
        motion_=event::EventBus::Get().Subscribe<event::MotionEvent>([this](auto& e){
            if(!suppressMovement)return;
            if(e.type==event::EventType::Pre)java::SetPlayerSprinting(false);
            else if(inputTick_==tickId_)neutralTick_=tickId_;
        },event::Priority::Lowest+10);
        strafe_=event::EventBus::Get().Subscribe<event::StrafeEvent>([](auto&){
            if(suppressMovement)java::SetPlayerSprinting(false);
        },event::Priority::Lowest+10);
    }
    ~AutoTotem() override {tick_.Reset();input_.Reset();motion_.Reset();strafe_.Reset();Finish(false);enabled=false;working=false;}
private:
    void Finish(bool resume) {
        const bool paused=suppressMovement.exchange(false);
        rotation::ReleaseAttack(this);phase_=0;working=false;
        if(!paused)return;
        auto player=java::PlayerEntity(),world=java::LevelEntity();
        const bool same=player&&world&&player.IsSame(player_)&&world.IsSame(world_);
        java::FreeCamKeys keys;
        const bool forward=resume&&same&&wasSprinting_&&input::IsProcessForeground()&&!clickgui::IsOpen()&&
            java::ReadFreeCamKeys(keys)&&keys.forward>0&&!keys.down;
        java::RestoreUseKey();java::RestoreSprintKey(forward);
        resumeKey_=forward;
    }
    void onEnable() override {reset_=true;enabled=true;}
    void onDisable() override {enabled=false;working=false;}
    bool Totem(int slot,bool offhand,bool& found) {
        JniAttach attach;if(!attach)return false;
        jobject stack=nullptr;java::ItemFacts facts;
        const bool read=(offhand?java::InventoryOffhand(0,stack):java::InventorySlot(slot,stack))&&
            java::ReadItemFacts(stack,facts);
        if(stack)attach->DeleteLocalRef(stack);
        found=read&&!facts.empty&&facts.count>0&&facts.item==java::KnownItem::TotemOfUndying;
        return read;
    }
    void Tick() {
        ++tickId_;
        if(resumeKey_){java::RestoreSprintKey();resumeKey_=false;}
        // Hold until the tick after swapping, so vanilla cannot resume input
        // in the same tick as the inventory click.
        if(phase_==2){
            bool held=false;
            if(Totem(0,true,held)&&held){missingSince_=-1;next_=0;}
            Finish(true);return;
        }
        working=false;
        if(autoreplenish::IsWorking())return;
        if(!enabled||!java::InventoryHasOffhand()){Finish(true);return;}
        if(phase_&&!grim_->get())Finish(true);
        auto player=java::PlayerEntity(),world=java::LevelEntity();
        if(!player||!world||!player.IsAlive()||player.IsSpectator()){Finish(false);return;}
        if(reset_.exchange(false)||!player.IsSame(player_)||!world.IsSame(world_)) {
            Finish(false);
            player_=std::move(player);world_=std::move(world);next_=0;missingSince_=-1;
        }
        bool held=false;
        if(!Totem(0,true,held)){Finish(true);return;}
        if(held){Finish(true);missingSince_=-1;next_=0;return;}
        const auto now=inventory::NowMillis();
        if(missingSince_<0)missingSince_=now;
        if(now<next_||now-missingSince_<delay_->get()||clickgui::IsOpen()||java::FreeCamActive()||
           refill::IsWorking()||cheststealer::IsWorking()||!java::InventoryCursorEmpty()){Finish(true);return;}
        JniAttach attach;if(!attach){Finish(true);return;}
        auto screen=java::CurrentScreen();
        const bool allowed=!screen||java::ScreenIsInventoryScreen(screen);
        if(screen)attach->DeleteLocalRef(screen);
        if(!allowed){Finish(true);return;}
        int menu=-1;if(!java::PlayerMenuContainerId(menu)){Finish(true);return;}
        for(int slot=0;slot<36;++slot) {
            bool found=false;if(!Totem(slot,false,found)){Finish(true);return;}
            if(!found)continue;
            if(grim_->get()) {
                // The shared attack/use reservation is available on modern
                // runtimes. Fail closed on profiles without that transaction.
                if(!compat::UsesModernRuntime()||!input::IsProcessForeground()||java::PlayerUsingItem()||
                   !rotation::AcquireAttack(this)){Finish(true);return;}
                if(!phase_) {
                    if(!java::PlayerSprinting(wasSprinting_)){Finish(false);return;}
                    phase_=1;started_=tickId_;neutralTick_=inputTick_=-1;
                    suppressMovement=true;
                }
                working=true;
                java::SetSprintKeyDown(false);java::SuppressUseKey();
                if(!java::SetPlayerSprinting(false)){Finish(true);return;}
                if(tickId_-started_>4){Finish(true);next_=now+250;return;}
                if(neutralTick_<started_||neutralTick_>=tickId_)return;
                bool sprinting=true;
                if(!java::PlayerSprinting(sprinting)||sprinting)return;
            }
            // Mio Offhand.do906: SWAP button 40 exchanges the source and offhand
            // atomically, leaving the cursor and selected hotbar slot untouched.
            // Inventory hotbar 0..8 maps to player-menu 36..44.
            working=true;
            java::ClickMenuSlot(menu,slot<9?slot+36:slot,40,java::ClickKind::Swap);
            if(grim_->get())phase_=2;
            // A JNI return is not server confirmation. Re-read next tick; if the
            // click was refused, bound retries instead of swapping repeatedly.
            next_=now+250;
            return;
        }
        Finish(true);
    }
    Properties::NumberProp* delay_{};
    Properties::BoolProperty* grim_{};
    int phase_=0,tickId_=0,started_=0,inputTick_=-1,neutralTick_=-1;
    bool wasSprinting_=false,resumeKey_=false;
    std::atomic<bool> reset_{true};
    long long next_=0,missingSince_=-1;
    java::EntityHandle player_,world_;
    event::Subscription tick_,input_,motion_,strafe_;
};
}
namespace autototem {
bool OwnsOffhand(){return enabled.load()&&java::InventoryHasOffhand();}
bool IsWorking(){return working.load();}
bool SuppressMovement(){return suppressMovement.load();}
}
std::unique_ptr<Module> MakeAutoTotemModule(){return std::make_unique<AutoTotem>();}
}
