#include "minecraft/modules/movement/elytrafly/AutoFire.h"
#include "minecraft/java/api/game_elytra.h"
#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_itemuse.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_player.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/modules/player/automlg/AutoMLG.h"
#include "minecraft/modules/player/autocharge/AutoCharge.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/compat/Profile.h"
#include "ui/clickgui/ClickGui.h"
#include "utils/Keyboard.h"
namespace nova::module::elytrafly {
namespace {
struct SlotIO {
    bool Selected(int& out){return java::SelectedSlot(out);}
    bool Select(int slot){return java::SetSelectedSlot(slot);}
    bool Sync(){return java::SyncSelectedItem();}
};
}
void AutoFire::Reset(){
    policy_={};selection_.Reset();player_.Reset();world_.Reset();
}
bool AutoFire::SameWorld()const{
    auto p=java::PlayerEntity(),w=java::LevelEntity();
    return p&&w&&p.IsSame(player_)&&w.IsSame(world_)&&p.IsAlive()&&!p.IsSpectator();
}
bool AutoFire::Observe(){
    java::ElytraFlight flight;
    const bool known=java::ReadElytraFlight(flight);
    const double length=std::sqrt(flight.look.DistanceToSqr(0,0,0));
    const double forward=length>1e-6?(flight.velocity.x*flight.look.x+flight.velocity.y*flight.look.y+
        flight.velocity.z*flight.look.z)/length:0.;
    policy_.Observe(tick_,known,flight.gliding,flight.boosting,forward);
    return known;
}
void AutoFire::Tick(bool enabled){
    ++tick_;
    if(compat::IsLegacyLunar())return;
    if(player_&&!SameWorld())Reset();
    SlotIO io;selection_.Restore(io);
    if(!enabled)return; // Preserve launch lock across toggles, preventing double use.
    if(!player_){
        player_=java::PlayerEntity();world_=java::LevelEntity();
        if(!SameWorld()){Reset();return;}
    }
    Observe();
}
void AutoFire::Interact(bool enabled){
    if(compat::UsesModernRuntime() && !rotation::CanSubmitItemUse())return;
    // Even a rejected use must restore its original slot before trying again.
    // Re-entering this callback must not replace that snapshot with the rocket slot.
    if(selection_.previous>=0)return;
    if(!enabled||compat::IsLegacyLunar()||!SameWorld()||!input::IsProcessForeground()||
       clickgui::IsOpen()||java::ScreenOpen()||java::FreeCamActive()||
       automlg::IsWorking()||autocharge::IsWorking()||
       java::UseKeyDown()||java::PlayerUsingItem())return;
    // A gliding rocket needs no separate aim. Reuse the current managed rotation
    // through UseSelectedItem; an active aura aim alone is not an item-use conflict.
    // CanSubmitItemUse still defers to an already reserved/submitted interaction.
    // Revalidate on the interaction thread immediately before selecting a slot.
    if(!Observe()||!policy_.Ready(tick_))return;
    int physical=-1,effective=-1;
    if(!java::SelectedSlot(physical)||!java::EffectiveSelectedSlot(effective)||physical!=effective)return;
    // Longer-duration rockets maintain thrust for more ticks per item.
    int best=-1,duration=-1;
    for(int slot=0;slot<9;++slot){
        int flight=0;
        if(java::FireworkFlightDuration(slot,flight)&&flight>duration){best=slot;duration=flight;}
    }
    if(best<0)return;
    selection_.Capture(physical,best);
    if(!java::SetSelectedSlot(best)||!java::SyncSelectedItem())return;
    // Only entering vanilla use starts the spawn wait. A failed precondition
    // must not leave a newly gliding player waiting for a rocket never sent.
    // PASS/JNI failure after entering vanilla remains an ambiguous submission.
    bool attempted=false;
    java::UseSelectedItem(java::KnownItem::FireworkRocket,attempted);
    if(attempted)policy_.Submitted(tick_);
}
}
