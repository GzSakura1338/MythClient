#include <cassert>
#include <cstdio>
#include <utility>
#include "minecraft/modules/movement/elytrafly/AutoFire.h"
#include "minecraft/java/api/game_elytra.h"

namespace mock {
bool rotating=true,busy=false,rescue=false,charge=false,boosting=false;
bool gliding=true,known=true,rejectBeforeUse=false,ambiguousUse=false;
int selected=0,uses=0;
nova::java::EntityHandle Handle(int id){return nova::java::AdoptEntity(reinterpret_cast<void*>(intptr_t(id)));}
}
namespace nova::java {
enum class KnownItem {FireworkRocket};
EntityHandle::~EntityHandle()=default;
EntityHandle::EntityHandle(EntityHandle&& e)noexcept:obj(std::exchange(e.obj,nullptr)){}
EntityHandle& EntityHandle::operator=(EntityHandle&& e)noexcept{obj=std::exchange(e.obj,nullptr);return *this;}
EntityHandle AdoptEntity(void* p){EntityHandle e;e.obj=p;return e;}
void EntityHandle::Reset(){obj=nullptr;}
bool EntityHandle::IsSame(const EntityHandle& e)const{return obj==e.obj;}
bool EntityHandle::IsAlive()const{return true;}
bool EntityHandle::IsSpectator()const{return false;}
EntityHandle PlayerEntity(){return mock::Handle(1);}
EntityHandle LevelEntity(){return mock::Handle(2);}
bool ReadElytraFlight(ElytraFlight& f){f={};f.gliding=mock::gliding;f.boosting=mock::boosting;f.look={0,0,1};return mock::known;}
bool FireworkFlightDuration(int slot,int& duration){duration=1;return slot==4;}
bool SelectedSlot(int& s){s=mock::selected;return true;}
bool EffectiveSelectedSlot(int& s){return SelectedSlot(s);}
bool SetSelectedSlot(int s){mock::selected=s;return true;}
bool SyncSelectedItem(){return true;}
bool ScreenOpen(){return false;}
bool FreeCamActive(){return false;}
bool UseKeyDown(){return false;}
bool PlayerUsingItem(){return false;}
bool UseSelectedItem(KnownItem,bool& attempted){
    attempted=false;assert(!mock::busy&&mock::selected==4);
    if(mock::rejectBeforeUse)return false;
    attempted=true;mock::busy=true;++mock::uses;return !mock::ambiguousUse;
}
}
namespace nova::compat {bool IsLegacyLunar(){return false;}bool UsesModernRuntime(){return true;}}
namespace nova::rotation {bool CanSubmitItemUse(){return !mock::busy;}bool IsActive(){return mock::rotating;}}
namespace nova::input {bool IsProcessForeground(){return true;}}
namespace nova::clickgui {bool IsOpen(){return false;}}
namespace nova::module::automlg {bool IsWorking(){return mock::rescue;}}
namespace nova::module::autocharge {bool IsWorking(){return mock::charge;}}
// @PRODUCTION_AUTOFIRE@
int main(){
    using nova::module::elytrafly::AutoFire;
    // A continuously active aura rotation must not prevent a ready rocket.
    AutoFire fire;fire.Tick(true);fire.Interact(true);
    assert(mock::uses==1&&mock::selected==4&&mock::rotating);
    fire.Interact(true);assert(mock::uses==1);
    mock::busy=false;fire.Tick(true);assert(mock::selected==0);
    fire.Interact(true);assert(mock::uses==1); // Existing launch confirmation lock.
    for(int tick=0;tick<80;++tick){mock::boosting=true;mock::busy=false;fire.Tick(true);fire.Interact(true);}
    assert(mock::uses==1); // No extra rockets while the first is attached.
    for(int gate=0;gate<3;++gate){
        AutoFire blocked;mock::boosting=false;mock::busy=gate==0;mock::rescue=gate==1;mock::charge=gate==2;
        blocked.Tick(true);blocked.Interact(true);assert(mock::uses==1&&mock::selected==0);
    }
    mock::busy=mock::rescue=mock::charge=false;
    AutoFire retry;mock::busy=true;retry.Tick(true);retry.Interact(true);assert(mock::uses==1);
    mock::busy=false;retry.Tick(true);retry.Interact(true);assert(mock::uses==2);
    retry.Tick(false);assert(mock::selected==0);
    // The start-flying input runs after Tick Pre: Interact must refresh flight.
    AutoFire takeoff;mock::busy=false;mock::gliding=false;takeoff.Tick(true);
    mock::gliding=true;takeoff.Interact(true);assert(mock::uses==3);
    mock::busy=false;takeoff.Tick(false);assert(mock::selected==0);
    // A rejected inventory/native precondition never consumed a rocket.
    AutoFire rejected;mock::rejectBeforeUse=true;rejected.Tick(true);rejected.Interact(true);
    rejected.Interact(true); // Repeated callback must retain the original slot.
    assert(mock::uses==3);
    mock::rejectBeforeUse=false;rejected.Tick(true);assert(mock::selected==0);
    rejected.Interact(true);assert(mock::uses==4);
    // An unconfirmed use is retried after 500 ms, with fresh telemetry.
    for(int tick=0;tick<9;++tick){mock::busy=false;rejected.Tick(true);rejected.Interact(true);}
    assert(mock::uses==4);
    mock::known=false;rejected.Tick(true);rejected.Interact(true);assert(mock::uses==4);
    mock::known=true;rejected.Interact(true);assert(mock::uses==5);
    mock::busy=false;rejected.Tick(false);assert(mock::selected==0);
    // A manually launched booster appearing between callbacks blocks use.
    AutoFire manual;manual.Tick(true);mock::boosting=true;manual.Interact(true);
    assert(mock::uses==5&&mock::selected==0);
    // PASS or a JNI error after native use may still send a packet. Keep the
    // guard through toggling and a failed takeoff followed by another glide.
    AutoFire ambiguous;mock::boosting=false;mock::ambiguousUse=true;
    ambiguous.Tick(true);ambiguous.Interact(true);assert(mock::uses==6);
    mock::busy=false;ambiguous.Tick(false);assert(mock::selected==0);
    mock::gliding=false;ambiguous.Tick(true);ambiguous.Interact(true);
    mock::gliding=true;
    for(int tick=0;tick<7;++tick){ambiguous.Tick(true);ambiguous.Interact(true);}
    assert(mock::uses==6);
    ambiguous.Tick(true);ambiguous.Interact(true);assert(mock::uses==7);
    std::puts("PASS: AutoFire startup refresh, failed use retry, ambiguous use/toggle/landing guard, aura and interaction priority, booster and slot restore");
}
