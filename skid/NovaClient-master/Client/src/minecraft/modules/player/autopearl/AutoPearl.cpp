#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/events/GameEvents.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_itemuse.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_player.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/modules/combat/automace/AutoMace.h"
#include "minecraft/modules/combat/autorod/AutoRod.h"
#include "minecraft/modules/combat/autoblock/AutoBlockRuntime.h"
#include "ui/clickgui/ClickGui.h"
#include "ui/notifications/Notifications.h"
#include "utils/Keyboard.h"
#include <atomic>

namespace nova::module {
namespace {
class AutoPearl final : public Module {
public:
    AutoPearl():Module("AutoPearl",Category::Player,"General","General") {
        auto& bus=event::EventBus::Get();
        tick_=bus.Subscribe<event::GameTickEvent>([this](auto& e){if(e.type==event::EventType::Pre)Tick();},event::Priority::Highest);
        interact_=bus.Subscribe<event::PrePlayerInteractEvent>([this](auto&){Interact();},event::Priority::Low);
        respawn_=bus.Subscribe<event::RespawnEvent>([this](auto&){reset_=true;},event::Priority::Highest);
    }
    ~AutoPearl() override {
        tick_.Reset();interact_.Reset();respawn_.Reset();
        // Destruction can occur off the game thread; never send packets here.
        java::ReleaseSilentSlot(this);
    }
private:
    void onEnable() override {requested_=true;}
    void onDisable() override {requested_=false;}
    void Stop(const char* reason=nullptr) {
        requested_=false;setEnabled(false);
        if(reason)notifications::Post(std::string("AutoPearl: ")+reason,notifications::Type::Warning);
    }
    bool SameWorld() const {
        auto p=java::PlayerEntity(),w=java::LevelEntity();
        return p&&w&&player_&&world_&&p.IsSame(player_)&&w.IsSame(world_);
    }
    void Clear() {
        java::ReleaseSilentSlot(this);previous_=used_=-1;restored_=false;
        player_.Reset();world_.Reset();age_=0;
    }
    static bool CanSwitch(){return !compat::IsLegacyLunar()||compat::lunar::HeldSlotChangeReady();}
    static bool PearlAt(int slot) {
        JniAttach attach;jobject stack=nullptr;java::ItemFacts facts;
        if(!attach)return false;
        const bool ok=java::InventorySlot(slot,stack)&&stack&&java::ReadItemFacts(stack,facts)&&
            !facts.empty&&facts.count>0&&facts.item==java::KnownItem::EnderPearl;
        if(stack)attach->DeleteLocalRef(stack);
        return ok;
    }
    void Restore() {
        // Called only at the NEXT GameTick Pre, including failure/disable paths.
        // Retain the lease and saved selection until both write and sync succeed.
        if(!java::OwnsPhysicalSlot(this)){Clear();return;}
        int selected=-1;if(!java::SelectedSlot(selected))return;
        if(selected!=(restored_?previous_:used_)) {Clear();return;} // Respect manual scroll.
        if(!CanSwitch())return;
        if(!restored_) {
            if(!java::SetOwnedPhysicalSlot(this,previous_))return;
            restored_=true;
        }
        if(!java::SyncSelectedItem())return;
        Clear();
    }
    void Tick() {
        if(reset_.exchange(false)||(player_&&!SameWorld())){Stop();Clear();return;}
        if(previous_>=0){Restore();return;}
        if(!requested_){Clear();return;}
        if(!player_) {
            player_=java::PlayerEntity();world_=java::LevelEntity();age_=0;
            if(!player_||!world_){Stop();Clear();return;}
        }
        if(!player_.IsAlive()||player_.IsSpectator()){Stop();Clear();return;}
        // Do not throw unexpectedly long after a blocked activation.
        if(++age_>60){Stop("Item use stayed busy; cancelled");Clear();}
    }
    void Interact() {
        if(!requested_||previous_>=0||!SameWorld()||!player_.IsAlive()||player_.IsSpectator())return;
        if(!input::IsProcessForeground()||clickgui::IsOpen()||java::ScreenOpen()||java::FreeCamActive()||
           automlg::IsWorking()||autocharge::IsWorking()||automace::IsWorking()||autorod::OwnsItemUse()||
           autoblock::OwnsItemUse()||rotation::IsActive()||java::PlayerUsingItem()||java::UseKeyDown()||!CanSwitch())return;
        if(compat::UsesModernRuntime()&&!rotation::CanSubmitItemUse())return;
        int source=-1;
        for(int i=0;i<9;++i)if(PearlAt(i)){source=i;break;}
        if(source<0){Stop("Put an ender pearl in the hotbar");return;}
        int old=-1;
        if(!java::SelectedSlot(old)||old<0||old>8||!java::AcquirePhysicalSlot(this))return;
        // Arm restoration BEFORE any mutation. A failed JNI call may still
        // have changed selection or sent USE_ITEM; never auto-repeat the use.
        previous_=old;used_=source;restored_=false;
        if(!java::SetOwnedPhysicalSlot(this,source)||!java::SyncSelectedItem()) {
            Stop("Could not switch to the pearl slot");return;
        }
        requested_=false;
        const bool accepted=java::UseSelectedItem(java::KnownItem::EnderPearl);
        Stop(accepted?nullptr:"Pearl use was not accepted (check cooldown)");
    }
    std::atomic<bool> requested_{false},reset_{false};
    java::EntityHandle player_,world_;
    int previous_=-1,used_=-1,age_=0;
    bool restored_=false;
    event::Subscription tick_,interact_,respawn_;
};
}
std::unique_ptr<Module> MakeAutoPearlModule(){return std::make_unique<AutoPearl>();}
}
