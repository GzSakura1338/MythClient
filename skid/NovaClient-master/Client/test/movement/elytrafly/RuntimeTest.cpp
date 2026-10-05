#include <cassert>
#include <cstdio>
#include <functional>
#include <memory>
#include <atomic>
#include <map>
#include <array>
#include <utility>
#include "minecraft/java/api/game_entity.h"
#include "minecraft/modules/movement/elytrafly/EquipLogic.h"
using jobject=void*;
struct MockEnv {void DeleteLocalRef(void*){}};
struct JniAttach {explicit operator bool()const{return true;}MockEnv* operator->(){static MockEnv e;return &e;}};
namespace mock {
int chest=10,selected=0,world=2,uses=0,delay=0,pending=-1,fire=0;
std::array<int,9> slots{};
bool down=false,hold=true,focus=true,screen=false,busy=false,sync=true,accept=true,read=true,modern=true;
bool missing=false;int warnings=0;
void* Ptr(int id){return reinterpret_cast<void*>(intptr_t(id));}
nova::java::EntityHandle Handle(int id){return nova::java::AdoptEntity(Ptr(id));}
void Swap(){std::swap(chest,slots[selected]);}
}
namespace nova::java {
enum class KnownItem{Nothing,Elytra};
struct ItemFacts{KnownItem item=KnownItem::Nothing;bool empty=true;int maxDamage=0,damageValue=0;};
EntityHandle::~EntityHandle()=default;
EntityHandle::EntityHandle(EntityHandle&& o)noexcept:obj(std::exchange(o.obj,nullptr)){}
EntityHandle& EntityHandle::operator=(EntityHandle&& o)noexcept{obj=std::exchange(o.obj,nullptr);return *this;}
EntityHandle AdoptEntity(void* p){EntityHandle h;h.obj=p;return h;}
void EntityHandle::Reset(){obj=nullptr;}
bool EntityHandle::IsSame(const EntityHandle& h)const{return obj==h.obj;}
bool EntityHandle::IsAlive()const{return true;}
bool EntityHandle::IsSpectator()const{return false;}
EntityHandle PlayerEntity(){return mock::Handle(1);}EntityHandle LevelEntity(){return mock::Handle(mock::world);}
bool InventoryArmor(int,jobject& p){p=mock::Ptr(mock::chest);return mock::read;}
bool InventorySlot(int s,jobject& p){p=mock::Ptr(mock::slots[s]);return mock::read&&!mock::missing;}
bool ReadItemFacts(jobject p,ItemFacts& f){f={};f.empty=!p;f.item=intptr_t(p)==20?KnownItem::Elytra:KnownItem::Nothing;return mock::read;}
bool CopyItemStack(jobject p,jobject& out){out=p;return true;}
bool ItemStacksMatch(jobject a,jobject b){return a==b;}
bool SelectedSlot(int& s){s=mock::selected;return true;}
bool EffectiveSelectedSlot(int& s){s=mock::selected;return true;}
bool SetSelectedSlot(int s){mock::selected=s;return true;}
bool SyncSelectedItem(){return mock::sync;}
bool ScreenOpen(){return mock::screen;}
bool UseSelectedEquipment(const EntityHandle& expected){
    assert(mock::slots[mock::selected]==intptr_t(expected.raw()));++mock::uses;mock::busy=true;
    if(!mock::accept)return false;
    if(mock::delay>0)mock::pending=mock::selected;else mock::Swap();
    return true;
}
}
namespace nova::bind {
bool IsPhysicalBindingDown(int){return mock::down;}
struct BindProperty{BindProperty(int,bool){}template<class F>void setGate(F){}int keybind()const{return -5;}bool hold()const{return mock::hold;}};
}
namespace Properties {
struct BoolProperty{bool value;bool get()const{return value;}};
struct PropDesc{PropDesc(const char*,void*){}};
}
namespace nova::event {
enum class EventType{Pre};struct GameTickEvent{EventType type=EventType::Pre;};struct PrePlayerInteractEvent{};struct RespawnEvent{};
namespace Priority{constexpr int Highest=0,Low=100;}
template<class E>std::function<void(E&)> callback;
struct Subscription{std::function<void()> reset;void Reset(){if(reset){reset();reset={};}}};
struct EventBus{static EventBus& Get(){static EventBus b;return b;}
    template<class E,class F>Subscription Subscribe(F f,int){callback<E> = f;return {[]{callback<E> = {};}};}};
template<class E>void Post(){E e;if(callback<E>)callback<E>(e);}
}
namespace nova::compat {bool UsesModernRuntime(){return mock::modern;}bool IsLegacyLunar(){return false;}}
namespace nova::rotation {bool CanSubmitItemUse(){return !mock::busy;}}
namespace nova::input {bool IsProcessForeground(){return mock::focus;}}
namespace nova::clickgui {bool IsOpen(){return false;}}
namespace nova::notifications {enum class Type{Warning};void Post(const std::string&,Type){++mock::warnings;}}
namespace nova::module::automlg {bool IsWorking(){return false;}}
namespace nova::module::autocharge {bool IsWorking(){return false;}}
namespace nova::module::elytrafly {struct AutoFire{void Reset(){}void Tick(bool){}void Interact(bool enabled){if(enabled)++mock::fire;}};}
namespace nova::module {
enum class Category{Movement};
class Module{
public:
    Module(const char*,Category,const char*,const char*){}virtual ~Module()=default;
    std::map<std::string,Properties::BoolProperty> bools;
    void Enable(){onEnable();}void Disable(){onDisable();}
protected:
    virtual void onEnable(){}virtual void onDisable(){}
    Properties::BoolProperty& addBool(const char* name,bool value,std::initializer_list<Properties::PropDesc> = {}){return bools.insert_or_assign(name,Properties::BoolProperty{value}).first->second;}
};
}
// @PRODUCTION_MODULE@
using namespace nova;
void Tick(){
    mock::busy=false;
    if(mock::pending>=0&&--mock::delay<=0){std::swap(mock::chest,mock::slots[mock::pending]);mock::pending=-1;}
    event::Post<event::GameTickEvent>();
}
void Interact(){event::Post<event::PrePlayerInteractEvent>();}
void Run(int ticks){for(int i=0;i<ticks;++i){Tick();Interact();}}
void Setup(module::Module& m){
    event::Post<event::RespawnEvent>();mock::down=false;mock::hold=true;mock::focus=mock::sync=mock::accept=mock::read=true;
    mock::screen=mock::busy=mock::missing=false;mock::modern=true;mock::chest=10;mock::slots={};mock::slots[4]=20;
    mock::selected=0;mock::uses=mock::delay=mock::fire=mock::warnings=0;mock::pending=-1;mock::world=2;
    m.bools["Auto Equip"].value=true;m.bools["Auto Fire"].value=true;m.Enable();Tick();
}
void Wear(){mock::down=true;Tick();Interact();assert(mock::chest==20&&mock::uses==1);Tick();assert(mock::selected==0);}
void Release(){mock::down=false;Tick();Interact();}
int main(){
    auto m=module::MakeElytraFlyModule();
    Setup(*m);Wear();Release();assert(mock::chest==10&&mock::uses==2);Run(2);assert(mock::selected==0);
    // Reproduce the old 10-tick Clear(): key released BEFORE equip sync arrives.
    Setup(*m);mock::delay=18;mock::down=true;Tick();Interact();mock::down=false;Run(25);
    assert(mock::chest==10&&mock::uses==2&&mock::selected==0);
    // Successful but delayed restoration must not be sent twice (which would
    // equip the elytra again when the first swap eventually arrives).
    Setup(*m);Wear();mock::delay=24;Release();Run(30);
    assert(mock::chest==10&&mock::uses==2&&mock::selected==0);
    // Restore denied once must retain original identity and retry, no click spam.
    Setup(*m);Wear();mock::accept=false;Release();int last=mock::uses;Run(9);assert(mock::uses==last);
    mock::accept=true;Run(4);assert(mock::chest==10&&mock::uses==3&&mock::selected==0);
    Setup(*m);Wear();mock::accept=false;Release();mock::read=false;Run(14);
    assert(mock::uses==2);mock::read=mock::accept=true;Run(3);assert(mock::chest==10&&mock::uses==3);
    // Reads can fail longer than the confirmation timeout.
    Setup(*m);Wear();mock::read=false;Release();Run(20);assert(mock::uses==1);
    mock::read=true;Run(3);assert(mock::chest==10&&mock::uses==2);
    Setup(*m);Wear();mock::missing=true;Release();Run(15);assert(mock::uses==1);
    mock::missing=false;Run(3);assert(mock::chest==10&&mock::uses==2);
    Setup(*m);Wear();mock::sync=false;Release();assert(mock::chest==20);
    mock::modern=false;Interact();Interact();assert(mock::uses==1); // Failed sync retries only next tick.
    mock::sync=true;Run(3);assert(mock::chest==10&&mock::uses==2);
    Setup(*m);Wear();mock::screen=true;Release();Run(12);assert(mock::chest==20&&mock::uses==1);
    mock::screen=false;Run(3);assert(mock::chest==10);
    Setup(*m);Wear();m->Disable();Run(3);assert(mock::chest==10);
    Setup(*m);Wear();m->bools["Auto Equip"].value=false;Run(3);assert(mock::chest==10);
    Setup(*m);Wear();mock::focus=false;Release();Run(12);assert(mock::uses==1);mock::focus=true;Run(3);assert(mock::chest==10);
    // Do not swap a manually selected different chestplate, or act in a new world.
    Setup(*m);Wear();mock::chest=30;Release();Run(3);assert(mock::chest==30&&mock::uses==1);
    Setup(*m);Wear();++mock::world;Release();Run(3);assert(mock::uses==1);
    // A click binding still toggles off only on the SECOND press.
    Setup(*m);mock::hold=false;Tick();Wear();Release();Run(3);assert(mock::chest==20);
    mock::down=true;Run(3);assert(mock::chest==10&&mock::uses==2);
    m.reset();
    std::puts("PASS: production ElytraFly Hold release, delayed equip, denied restore/retry, read/sync failures, UI/focus/disable, world/manual swap and Click mode");
}
