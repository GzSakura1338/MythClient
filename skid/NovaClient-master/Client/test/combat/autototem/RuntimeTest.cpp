#include <cassert>
#include <cstdio>
#include <functional>
#include <memory>
#include <atomic>
#include <format>
#include <array>
#include <utility>
#include "minecraft/java/api/game_entity.h"
using jobject=void*;
struct MockEnv {void DeleteLocalRef(void*){}};
struct JniAttach {explicit operator bool()const{return true;}MockEnv* operator->(){static MockEnv e;return &e;}};
namespace mock {
std::array<int,36> slots{};
int offhand=7,clicks=0,source=-1,screen=0;
long long now=1000;
bool cursor=true,supported=true,accept=true;
bool sprinting=true,sprintKey=true,physicalForward=true,lease=false,otherLease=false;
bool usingItem=false,focused=true;
int world=2;
void* Ptr(int v){return reinterpret_cast<void*>(intptr_t(v));}
nova::java::EntityHandle Handle(int v){return nova::java::AdoptEntity(Ptr(v));}
}
namespace nova::java {
enum class KnownItem{Nothing,TotemOfUndying};
enum class ClickKind{Swap};
struct ItemFacts{bool empty=true;int count=0;KnownItem item=KnownItem::Nothing;};
EntityHandle::~EntityHandle()=default;
EntityHandle::EntityHandle(EntityHandle&& e)noexcept:obj(std::exchange(e.obj,nullptr)){}
EntityHandle& EntityHandle::operator=(EntityHandle&& e)noexcept{obj=std::exchange(e.obj,nullptr);return *this;}
EntityHandle AdoptEntity(void* p){EntityHandle e;e.obj=p;return e;}
bool EntityHandle::IsSame(const EntityHandle& e)const{return obj==e.obj;}
bool EntityHandle::IsAlive()const{return true;}bool EntityHandle::IsSpectator()const{return false;}
EntityHandle PlayerEntity(){return mock::Handle(1);}EntityHandle LevelEntity(){return mock::Handle(mock::world);}
struct FreeCamKeys{float forward=0;bool down=false;};
bool ReadFreeCamKeys(FreeCamKeys& k){k.forward=mock::physicalForward?1:0;return true;}
bool PlayerSprinting(bool& s){s=mock::sprinting;return true;}
bool SetPlayerSprinting(bool s){mock::sprinting=s;return true;}
bool SetSprintKeyDown(bool s){mock::sprintKey=s;return true;}
bool RestoreSprintKey(bool resume=false){mock::sprintKey=resume;return true;}
void SuppressUseKey(){}void RestoreUseKey(){}
bool PlayerUsingItem(){return mock::usingItem;}
bool InventorySlot(int s,jobject& p){p=mock::Ptr(mock::slots[s]);return true;}
bool InventoryOffhand(int,jobject& p){p=mock::Ptr(mock::offhand);return true;}
bool ReadItemFacts(jobject p,ItemFacts& f){f.empty=!p;f.count=p?1:0;f.item=intptr_t(p)==9?KnownItem::TotemOfUndying:KnownItem::Nothing;return true;}
bool InventoryHasOffhand(){return mock::supported;}bool InventoryCursorEmpty(){return mock::cursor;}
bool FreeCamActive(){return false;}
jobject CurrentScreen(){return mock::Ptr(mock::screen);}
bool ScreenIsInventoryScreen(jobject p){return intptr_t(p)==1;}
bool PlayerMenuContainerId(int& out){out=0;return true;}
bool ClickMenuSlot(int menu,int slot,int button,ClickKind){
    assert(menu==0&&button==40&&mock::cursor&&mock::screen!=2);
    mock::source=slot;++mock::clicks;
    if(mock::accept)std::swap(mock::offhand,mock::slots[slot>=36?slot-36:slot]);
    return mock::accept;
}
}
namespace Properties{struct NumberProp{double value;double get()const{return value;}};struct BoolProperty{bool value=false;bool get()const{return value;}};}
namespace nova::event {
enum class EventType{Pre,Post};struct GameTickEvent{EventType type=EventType::Pre;};
struct MoveInputEvent{float forward=1,strafe=0,finalForward=1,finalStrafe=0;bool jump=true,sneak=true,hasFinalForward=false,hasFinalStrafe=false,skipRotationFix=false,suppressSprint=false;};
struct MotionEvent{EventType type=EventType::Post;};struct StrafeEvent{};
namespace Priority{constexpr int Highest=0,Lowest=20;}
template<class E>std::function<void(E&)> callback;
struct Subscription{std::function<void()> reset;void Reset(){if(reset)reset();}};
struct EventBus{static EventBus& Get(){static EventBus bus;return bus;}
template<class E,class F>Subscription Subscribe(F f,int){callback<E> = f;return {[]{callback<E> = {};}};}};
}
namespace nova::compat{bool UsesModernRuntime(){return mock::supported;}}
namespace nova::input{bool IsProcessForeground(){return mock::focused;}}
namespace nova::rotation{bool AcquireAttack(const void*){if(mock::otherLease)return false;mock::lease=true;return true;}void ReleaseAttack(const void*){mock::lease=false;}}
namespace nova::module::refill{bool IsWorking(){return false;}}
namespace nova::module::autoreplenish{bool IsWorking(){return false;}}
namespace nova::module::cheststealer{bool IsWorking(){return false;}}
namespace nova::clickgui{bool IsOpen(){return false;}}
namespace nova::inventory{long long NowMillis(){return mock::now;}}
namespace nova::module {
enum class Category{Combat};
class Module{
public:
Module(const char*,Category,const char*,const char*){}virtual ~Module()=default;
void Enable(){onEnable();}void Disable(){onDisable();}
Properties::NumberProp delay{};
Properties::BoolProperty grim{};
protected:
virtual void onEnable(){}virtual void onDisable(){}
Properties::NumberProp& addNumber(const char*,double,double,double v,double initial,std::initializer_list<int>,std::function<std::string(double)>){delay.value=initial;return delay;}
Properties::BoolProperty& addBool(const char*,bool initial){grim.value=initial;return grim;}
};
}
// @PRODUCTION_MODULE@
void Tick(){nova::event::GameTickEvent e;nova::event::callback<nova::event::GameTickEvent>(e);}
void Neutral(){
    nova::event::MoveInputEvent e;nova::event::callback<nova::event::MoveInputEvent>(e);
    assert(e.forward==0&&e.strafe==0&&!e.jump&&!e.sneak&&e.suppressSprint&&e.hasFinalForward&&e.finalForward==0);
    nova::event::MotionEvent motion;nova::event::callback<nova::event::MotionEvent>(motion);
}
int main(){
    auto m=nova::module::MakeAutoTotemModule();m->Enable();
    mock::slots[5]=9;Tick();assert(mock::clicks==1&&mock::offhand==9&&mock::slots[5]==7&&mock::source==41);
    Tick();assert(mock::clicks==1&&!nova::module::autototem::IsWorking());
    mock::offhand=0;mock::slots[20]=9;mock::now+=50;Tick();assert(mock::clicks==2&&mock::source==20&&mock::offhand==9);
    Tick();mock::offhand=0;mock::slots[12]=9;mock::cursor=false;Tick();assert(mock::clicks==2);
    mock::cursor=true;mock::screen=2;Tick();assert(mock::clicks==2);
    mock::screen=1;Tick();assert(mock::clicks==3&&mock::offhand==9);
    Tick();mock::offhand=0;mock::slots[15]=9;m->delay.value=100;Tick();assert(mock::clicks==3);
    mock::now+=99;Tick();assert(mock::clicks==3);++mock::now;Tick();assert(mock::clicks==4);
    Tick();m->delay.value=0;mock::offhand=0;mock::slots[2]=9;mock::accept=false;Tick();assert(mock::clicks==5);
    mock::now+=50;Tick();assert(mock::clicks==5);mock::now+=200;mock::accept=true;Tick();assert(mock::clicks==6&&mock::offhand==9);
    m->Disable();assert(!nova::module::autototem::OwnsOffhand());mock::offhand=0;mock::slots[3]=9;Tick();assert(mock::clicks==6);
    m->Enable();mock::supported=false;Tick();assert(mock::clicks==6);
    mock::supported=true;mock::screen=0;mock::now+=1000;m->grim.value=true;m->Enable();Tick();
    assert(mock::clicks==6&&nova::module::autototem::SuppressMovement()&&mock::lease&&!mock::sprinting);
    // No click without both input and motion; only the following tick may swap.
    Tick();assert(mock::clicks==6);Neutral();mock::now+=50;Tick();
    assert(mock::clicks==7&&mock::offhand==9&&mock::lease&&nova::module::autototem::SuppressMovement());
    Tick();assert(!mock::lease&&!nova::module::autototem::SuppressMovement()&&mock::sprintKey);
    Tick();assert(!mock::sprintKey); // One-tick vanilla resume request, no stuck key.
    mock::offhand=0;mock::slots[6]=9;mock::sprinting=true;Tick();Neutral();mock::physicalForward=false;
    Tick();assert(mock::clicks==8);Tick();assert(!mock::sprintKey&&!mock::lease);
    // Toggle off or change worlds mid-pause releases the lease and input hold.
    mock::offhand=0;mock::slots[7]=9;Tick();assert(mock::lease);m->Disable();Tick();assert(!mock::lease&&!nova::module::autototem::SuppressMovement());
    m->Enable();Tick();assert(mock::lease);++mock::world;mock::slots={};Tick();assert(!mock::lease&&!nova::module::autototem::SuppressMovement());
    mock::slots[8]=9;mock::otherLease=true;Tick();assert(!mock::lease&&!nova::module::autototem::SuppressMovement());mock::otherLease=false;
    mock::usingItem=true;Tick();assert(!mock::lease);mock::usingItem=false;
    Tick();assert(mock::lease);mock::screen=2;Tick();assert(!mock::lease);mock::screen=0;
    // Missing motion hook cannot leave movement frozen forever.
    Tick();for(int i=0;i<5;++i)Tick();assert(!mock::lease&&!nova::module::autototem::SuppressMovement());
    std::puts("PASS: AutoTotem hotbar/backpack swap, displaced item preservation, pop refill, delay, cursor/container gates, retry and disable/legacy");
}
