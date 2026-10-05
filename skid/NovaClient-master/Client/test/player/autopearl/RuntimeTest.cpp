#include <cassert>
#include <cstdio>
#include <functional>
#include <memory>
#include <atomic>
#include <utility>
#include "minecraft/java/api/game_entity.h"
using jobject=void*;
struct MockEnv{void DeleteLocalRef(void*){}};
struct JniAttach{explicit operator bool()const{return true;}MockEnv* operator->(){static MockEnv e;return &e;}};
namespace mock {
int slot=0,pearl=4,world=2,uses=0,syncs=0;
bool read=true,select=true,sync=true,accept=true,busy=false,screen=false,focus=true,active=false,legacy=false,ready=true;
const void* owner=nullptr;
void* Ptr(int id){return reinterpret_cast<void*>(intptr_t(id));}
nova::java::EntityHandle Handle(int id){return nova::java::AdoptEntity(Ptr(id));}
}
namespace nova::java {
enum class KnownItem{Nothing,EnderPearl};
struct ItemFacts{KnownItem item=KnownItem::Nothing;bool empty=true;int count=0;};
EntityHandle::~EntityHandle()=default;
EntityHandle::EntityHandle(EntityHandle&& o)noexcept:obj(std::exchange(o.obj,nullptr)){}
EntityHandle& EntityHandle::operator=(EntityHandle&& o)noexcept{obj=std::exchange(o.obj,nullptr);return *this;}
EntityHandle AdoptEntity(void* p){EntityHandle h;h.obj=p;return h;}
void EntityHandle::Reset(){obj=nullptr;}
bool EntityHandle::IsSame(const EntityHandle& h)const{return obj==h.obj;}
bool EntityHandle::IsAlive()const{return true;}
bool EntityHandle::IsSpectator()const{return false;}
EntityHandle PlayerEntity(){return mock::Handle(1);}EntityHandle LevelEntity(){return mock::Handle(mock::world);}
bool InventorySlot(int s,jobject& p){p=s==mock::pearl?mock::Ptr(20):nullptr;return true;}
bool ReadItemFacts(jobject p,ItemFacts& f){f={};if(p){f.item=KnownItem::EnderPearl;f.empty=false;f.count=1;}return true;}
bool SelectedSlot(int& s){s=mock::slot;return mock::read;}
bool AcquirePhysicalSlot(const void* p){if(mock::owner&&mock::owner!=p)return false;mock::owner=p;return true;}
bool OwnsPhysicalSlot(const void* p){return p==mock::owner;}
void ReleaseSilentSlot(const void* p){if(p==mock::owner)mock::owner=nullptr;}
bool SetOwnedPhysicalSlot(const void* p,int s){assert(p==mock::owner);if(!mock::select)return false;mock::slot=s;return true;}
bool SyncSelectedItem(){++mock::syncs;return mock::sync;}
bool ScreenOpen(){return mock::screen;}bool FreeCamActive(){return false;}
bool PlayerUsingItem(){return false;}bool UseKeyDown(){return false;}
bool UseSelectedItem(KnownItem item){
    assert(item==KnownItem::EnderPearl&&mock::slot==mock::pearl&&mock::owner&&!mock::busy);
    ++mock::uses;mock::busy=true;return mock::accept;
}
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
namespace nova::compat {bool UsesModernRuntime(){return !mock::legacy;}bool IsLegacyLunar(){return mock::legacy;}}
namespace nova::compat::lunar {bool HeldSlotChangeReady(){return mock::ready;}}
namespace nova::rotation {bool CanSubmitItemUse(){return !mock::busy;}bool IsActive(){return mock::active;}}
namespace nova::input {bool IsProcessForeground(){return mock::focus;}}
namespace nova::clickgui {bool IsOpen(){return false;}}
namespace nova::notifications {enum class Type{Warning};void Post(const std::string&,Type){}}
namespace nova::module::automlg {bool IsWorking(){return false;}}
namespace nova::module::autocharge {bool IsWorking(){return false;}}
namespace nova::module::automace {bool IsWorking(){return false;}}
namespace nova::module::autorod {bool OwnsItemUse(){return false;}}
namespace nova::module::autoblock {bool OwnsItemUse(){return false;}}
namespace nova::module {
enum class Category{Player};
class Module{
public:
    Module(const char*,Category,const char*,const char*){}virtual ~Module()=default;
    bool setting=false,enabled=false;
    void setEnabled(bool v){setting=v;}
    void Update(){if(setting!=enabled){enabled=setting;if(enabled)onEnable();else onDisable();}}
protected:virtual void onEnable(){}virtual void onDisable(){}
};
}
// @PRODUCTION_MODULE@
using namespace nova;
void Tick(module::Module& m){mock::busy=false;m.Update();event::Post<event::GameTickEvent>();}
void Interact(){event::Post<event::PrePlayerInteractEvent>();}
void Setup(module::Module& m,int old=0,int pearl=4){
    event::Post<event::RespawnEvent>();Tick(m);assert(!mock::owner);
    mock::slot=old;mock::pearl=pearl;mock::world=2;mock::uses=mock::syncs=0;
    mock::read=mock::select=mock::sync=mock::accept=mock::focus=mock::ready=true;
    mock::screen=mock::active=mock::legacy=false;m.setEnabled(true);Tick(m);
}
int main(){
    auto m=module::MakeAutoPearlModule();
    for(int old=0;old<9;++old)for(int pearl=0;pearl<9;++pearl){
        Setup(*m,old,pearl);Interact();assert(mock::uses==1&&mock::slot==pearl&&!m->setting);
        Interact();assert(mock::uses==1&&mock::slot==pearl);Tick(*m);
        assert(mock::slot==old&&!mock::owner);Interact();assert(mock::uses==1);
    }
    Setup(*m);mock::accept=false;Interact();assert(mock::uses==1);Tick(*m);assert(mock::slot==0&&!mock::owner);
    Setup(*m);mock::sync=false;Interact();assert(mock::uses==0&&mock::slot==4);Tick(*m);
    assert(mock::slot==0&&mock::owner);mock::sync=true;Tick(*m);assert(!mock::owner&&mock::slot==0);
    Setup(*m);mock::select=false;Interact();Tick(*m);assert(!mock::owner&&mock::slot==0&&mock::uses==0);
    Setup(*m);Interact();mock::read=false;Tick(*m);assert(mock::owner&&mock::slot==4);
    mock::read=true;mock::select=false;Tick(*m);assert(mock::owner&&mock::slot==4);
    mock::select=true;Tick(*m);assert(!mock::owner&&mock::slot==0);
    Setup(*m);Interact();mock::slot=7;Tick(*m);assert(mock::slot==7&&!mock::owner);
    Setup(*m);Interact();mock::screen=true;mock::focus=false;m->setEnabled(false);Tick(*m);assert(mock::slot==0&&!mock::owner);
    Setup(*m);Interact();++mock::world;mock::slot=6;Tick(*m);assert(mock::slot==6&&!mock::owner);
    Setup(*m);mock::busy=true;Interact();assert(mock::uses==0&&mock::slot==0);Tick(*m);Interact();assert(mock::uses==1);Tick(*m);
    Setup(*m);mock::active=true;Interact();assert(mock::uses==0);for(int i=0;i<61;++i)Tick(*m);assert(!m->setting&&mock::slot==0);
    Setup(*m);mock::pearl=-1;Interact();assert(mock::uses==0&&!m->setting&&!mock::owner);
    Setup(*m);mock::legacy=true;mock::ready=false;Interact();assert(mock::uses==0);mock::ready=true;Interact();assert(mock::uses==1);
    mock::ready=false;Tick(*m);assert(mock::slot==4&&mock::owner);mock::ready=true;Tick(*m);assert(mock::slot==0&&!mock::owner);
    m.reset();std::puts("PASS: production AutoPearl 81 slot pairs, exactly one use, deferred switch back, read/write/sync failures, manual scroll, disable/UI/focus, world reset, busy timeout and legacy slot gate");
}
