#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/modules/player/pearlphase/PearlAim.h"
#include "minecraft/modules/player/autocharge/ManualUseGate.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/java/api/game_pearlphase.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_itemuse.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/java/api/game_options.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_player.h"
#include "minecraft/java/api/game_charge.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/modules/player/scaffold/Scaffold.h"
#include "minecraft/modules/player/refill/Refill.h"
#include "minecraft/modules/player/autoreplenish/AutoReplenish.h"
#include "minecraft/modules/player/cheststealer/ChestStealer.h"
#include "minecraft/modules/combat/automace/AutoMace.h"
#include "minecraft/modules/combat/autorod/AutoRod.h"
#include "minecraft/modules/combat/autototem/AutoTotem.h"
#include "minecraft/modules/combat/autoblock/AutoBlockRuntime.h"
#include "ui/clickgui/ClickGui.h"
#include "ui/notifications/Notifications.h"
#include "utils/Keyboard.h"
#include <atomic>
#include <chrono>

namespace nova::module {
namespace {
class PearlPhase final : public Module {
public:
    PearlPhase() : Module("PearlPhase",Category::Player,"General","General") {
        always_ = &addBool("Always",false);
        inventory_ = &addBool("Inventory",true,{},[]{return !compat::IsLegacyLunar();});
        auto& bus = event::EventBus::Get();
        tick_ = bus.Subscribe<event::GameTickEvent>([this](auto& e){
            if(e.type==event::EventType::Pre) Tick();
        },event::Priority::Highest);
        interact_ = bus.Subscribe<event::PrePlayerInteractEvent>([this](auto&){Interact();},event::Priority::High);
        respawn_ = bus.Subscribe<event::RespawnEvent>([this](auto&){reset_=true;},event::Priority::Highest);
        click_ = bus.Subscribe<event::ClickEvent>([this](auto& e){if(manual_.Blocks())e.Cancel();},event::Priority::Highest);
        input_ = bus.Subscribe<event::PrePlayerInteractEvent>([this](auto&){
            if(manual_.Blocks())java::SuppressUseKey();
        },event::Priority::Lowest+100);
    }
    ~PearlPhase() override {
        tick_.Reset();interact_.Reset();respawn_.Reset();click_.Reset();input_.Reset();
        // Module shutdown can run off-thread. Never issue inventory packets here.
        java::ReleaseSilentSlot(this);
    }
    std::string suffix() const override {return "Pearl";}
private:
    void onEnable() override {restart_=true;requested_=true;}
    void onDisable() override {requested_=false;}
    static auto Now(){return std::chrono::steady_clock::now();}
    void Stop(const char* reason=nullptr) {
        requested_=false;setEnabled(false);
        if(reason)notifications::Post(std::string("PearlPhase: ")+reason,notifications::Type::Warning);
    }
    bool SameWorld() const {
        auto p=java::PlayerEntity(),w=java::LevelEntity();
        return p&&w&&player_&&world_&&p.IsSame(player_)&&w.IsSame(world_);
    }
    static bool CanSwitch(){return !compat::IsLegacyLunar()||compat::lunar::HeldSlotChangeReady();}
    static bool PearlAt(int slot) {
        JniAttach attach;
        jobject stack=nullptr;java::ItemFacts facts;
        if(!attach)return false;
        const bool ok=java::InventorySlot(slot,stack)&&stack&&java::ReadItemFacts(stack,facts)&&
            !facts.empty&&facts.count>0&&facts.item==java::KnownItem::EnderPearl;
        if(stack)attach->DeleteLocalRef(stack);
        return ok;
    }
    bool SnapshotHand(int slot) {
        JniAttach attach;jobject stack=nullptr,copy=nullptr;
        if(!attach||!java::InventorySlot(slot,stack))return false;
        const bool ok=stack&&java::CopyItemStack(stack,copy);
        if(stack)attach->DeleteLocalRef(stack);
        if(ok&&copy)savedHand_=java::AdoptEntity(copy);
        else if(copy)attach->DeleteLocalRef(copy);
        return bool(savedHand_);
    }
    bool RestoreInventory() {
        if(stock_<0)return true;
        if(java::ScreenOpen()||!java::InventoryCursorEmpty())return false;
        JniAttach attach;jobject stock=nullptr,hand=nullptr;java::ItemFacts facts;
        if(!attach||!java::InventorySlot(stock_,stock))return false;
        const bool matches=stock&&savedHand_&&java::ItemStacksMatch(stock,static_cast<jobject>(savedHand_.raw()));
        if(stock)attach->DeleteLocalRef(stock);
        if(!java::InventorySlot(previous_,hand))return false;
        const bool pearlOrEmpty=hand&&java::ReadItemFacts(hand,facts)&&
            (facts.empty||facts.count<=0||facts.item==java::KnownItem::EnderPearl);
        if(hand)attach->DeleteLocalRef(hand);
        // A manual/server edit invalidates the inverse SWAP. Never move an
        // unrelated item back into the hand on an assumption about old slots.
        if(!matches||!pearlOrEmpty){Stop("Inventory changed; restore cancelled");stock_=-1;return true;}
        int menu=-1;
        if(!java::PlayerMenuContainerId(menu)||menu!=menu_)return false;
        if(!java::ClickMenuSlot(menu,stock_,previous_,java::ClickKind::Swap))return false;
        stock_=-1;savedHand_.Reset();return true;
    }
    void Clear() {
        rotation::ReleaseAttack(this);
        rotation::ReleaseExclusiveRotations(this);
        java::ReleaseSilentSlot(this);
        previous_=used_=stock_=menu_=-1;restored_=false;restoreAge_=0;
        savedHand_.Reset();player_.Reset();world_.Reset();
    }
    void Restore() {
        // Keep the requested rotation through the use/movement tick. Restore
        // selection on the next tick even after disable or an uncertain use.
        rotation::ReleaseExclusiveRotations(this);
        if(!java::OwnsPhysicalSlot(this)){Clear();return;}
        if(!RestoreInventory()) {
            if(++restoreAge_>60){Stop("Inventory restore timed out");Clear();}
            return;
        }
        int selected=-1;
        if(!java::SelectedSlot(selected))return;
        if(selected!=(restored_?previous_:used_)){Clear();return;}
        if(!CanSwitch())return;
        if(!restored_){if(!java::SetOwnedPhysicalSlot(this,previous_))return;restored_=true;}
        if(!java::SyncSelectedItem())return;
        Clear();
    }
    void Tick() {
        rotation::ReleaseAttack(this);
        if(manual_.BeginTick(false))java::RestoreUseKey();
        if(reset_.exchange(false)||(player_&&!SameWorld())){Stop();Clear();return;}
        if(previous_>=0){Restore();return;}
        if(restart_.exchange(false)){nextUse_={};waitTicks_=0;}
        if(!requested_){Clear();return;}
        if(!player_){player_=java::PlayerEntity();world_=java::LevelEntity();}
        if(!SameWorld()||!player_.IsAlive()||player_.IsSpectator()){Stop();Clear();return;}
        if(!always_->get()&&++waitTicks_>60){Stop("Item use stayed unavailable");Clear();}
    }
    bool Busy() const {
        return !input::IsProcessForeground()||java::ScreenOpen()||clickgui::IsOpen()||java::FreeCamActive()||
            scaffold::IsEnabled()||automlg::IsWorking()||autocharge::IsWorking()||automace::IsWorking()||
            autorod::OwnsItemUse()||autoblock::OwnsItemUse()||autototem::IsWorking()||
            refill::IsWorking()||autoreplenish::IsWorking()||cheststealer::IsWorking()||java::PlayerUsingItem();
    }
    void Interact() {
        if(!requested_||previous_>=0||!SameWorld()||Busy()||Now()<nextUse_||!CanSwitch())return;
        if(compat::UsesModernRuntime()&&!rotation::CanSubmitItemUse())return;
        java::PearlPhaseState state;
        if(!java::ReadPearlPhaseState(state))return;
        if(always_->get()&&(!state.ground||!state.horizontalCollision))return;
        if(state.blocked){if(!always_->get())Stop();return;}
        if(state.risingLowPose)return;
        int source=-1;
        for(int i=0;i<9;++i)if(PearlAt(i)){source=i;break;}
        if(source<0&&inventory_->get()&&!compat::IsLegacyLunar())
            for(int i=35;i>=9;--i)if(PearlAt(i)){source=i;break;}
        if(source<0){Stop("No ender pearls available");return;}
        if(source<9&&java::ChargeSupported()&&!java::ChargeSlotReady(source,java::KnownItem::EnderPearl))return;
        int old=-1;
        if(!java::SelectedSlot(old)||old<0||old>8)return;
        if(source>=9&&(!java::InventoryCursorEmpty()||!java::PlayerMenuContainerId(menu_)||!SnapshotHand(old)))return;
        if(!java::AcquirePhysicalSlot(this))return;
        const auto aim=pearlphase::Aim(state.position,state.yaw,state.forward,state.strafe,state.ladder);
        if(!rotation::SetExclusiveRotations(this,aim)){java::ReleaseSilentSlot(this);return;}
        // Arm rollback before mutation. Failed JNI can still have changed state.
        previous_=old;used_=source<9?source:old;restored_=false;restoreAge_=0;
        manual_.Begin();java::SuppressUseKey();
        if(source>=9){
            stock_=source;
            if(!java::ClickMenuSlot(menu_,source,old,java::ClickKind::Swap)){Stop("Could not retrieve pearl");return;}
        }
        if(!java::SetOwnedPhysicalSlot(this,used_)||!java::SyncSelectedItem()){
            Stop("Could not select pearl");return;
        }
        if(!compat::UsesModernRuntime()&&!java::SendMoveRotation(aim.yaw,aim.pitch,state.ground)){
            Stop("Could not submit pearl rotation");return;
        }
        // The controller owns modern interaction sequencing. No extra raw use
        // or post-use movement packet can split the shared rotation transaction.
        nextUse_=Now()+std::chrono::seconds(1);
        const auto angle=rotation::GetRotation();
        bool attempted=false;
        const bool used=java::UseSilentItem(java::KnownItem::EnderPearl,angle.yaw,angle.pitch,this,attempted);
        RestoreInventory();
        if(!used)Stop("Pearl use was rejected (check cooldown)");
        else if(!always_->get())Stop();
    }
    Properties::BoolProperty *always_=nullptr,*inventory_=nullptr;
    std::atomic<bool> requested_{false},reset_{false},restart_{false};
    autocharge::ManualUseGate manual_;
    java::EntityHandle player_,world_,savedHand_;
    std::chrono::steady_clock::time_point nextUse_{};
    int previous_=-1,used_=-1,stock_=-1,menu_=-1,restoreAge_=0,waitTicks_=0;
    bool restored_=false;
    event::Subscription tick_,interact_,respawn_,click_,input_;
};
}
std::unique_ptr<Module> MakePearlPhaseModule(){return std::make_unique<PearlPhase>();}
}
