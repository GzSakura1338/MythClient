#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include "minecraft/modules/player/pearlphase/PearlAim.h"
#include "minecraft/modules/player/autocharge/ManualUseGate.h"

using jobject=void*;
struct JniAttach {explicit operator bool()const{return true;} JniAttach* operator->(){return this;} void DeleteLocalRef(void*){} };
namespace fixture {
struct Stack {int type=0,count=0;};
std::array<Stack,36> items;
Stack copied;
int selected=2,world=1,uses=0,swaps=0,restores=0;
bool useOk=true,syncOk=true,canSwitch=true,screen=false,busy=false,ready=true,rotateReady=true,legacy=false;
std::vector<std::string> actions;
const void *slotOwner=nullptr,*rotationOwner=nullptr;
nova::rotation::Rotation angle;
void Reset(){
    items={};selected=2;world=1;uses=swaps=restores=0;
    useOk=syncOk=canSwitch=ready=rotateReady=true;screen=busy=legacy=false;
    slotOwner=rotationOwner=nullptr;actions.clear();items[2]={4,1};items[6]={1,8};
}
}
namespace Properties {struct BoolProperty {bool value;bool get()const{return value;}};}
namespace nova::module {
enum class Category{Player};
class Module {
public:
    Module(const char*,Category,const char*,const char*){} virtual ~Module()=default;
    void setEnabled(bool value){if(enabled==value)return;enabled=value;if(value)onEnable();else onDisable();}
    virtual std::string suffix()const{return {};}
    std::map<std::string,Properties::BoolProperty> props;
    bool enabled=false;
protected:
    virtual void onEnable(){} virtual void onDisable(){}
    Properties::BoolProperty& addBool(std::string label,bool value,std::vector<int> = {},std::function<bool()> = {}){return props.emplace(label,Properties::BoolProperty{value}).first->second;}
};
#define BUSY_NS(name,method) namespace name {bool method(){return fixture::busy;}}
BUSY_NS(automlg,IsWorking) BUSY_NS(autocharge,IsWorking) BUSY_NS(automace,IsWorking)
BUSY_NS(scaffold,IsEnabled) BUSY_NS(autorod,OwnsItemUse) BUSY_NS(autoblock,OwnsItemUse)
BUSY_NS(autototem,IsWorking) BUSY_NS(refill,IsWorking) BUSY_NS(autoreplenish,IsWorking) BUSY_NS(cheststealer,IsWorking)
#undef BUSY_NS
}
namespace nova::event {
enum class EventType{Pre};
struct GameTickEvent{EventType type=EventType::Pre;};
struct PrePlayerInteractEvent{};struct RespawnEvent{};struct ClickEvent{bool cancelled=false;void Cancel(){cancelled=true;}};
namespace Priority {constexpr int Highest=0,High=5,Lowest=20;}
template<class E> std::vector<std::function<void(E&)>> callbacks;
struct Subscription{std::function<void()> reset;void Reset(){if(reset)reset();reset={};}};
struct EventBus {
    static EventBus& Get(){static EventBus value;return value;}
    template<class E,class F> Subscription Subscribe(F callback,int){
        auto index=callbacks<E>.size();callbacks<E>.push_back(callback);
        return {[index]{callbacks<E>[index]={};}};
    }
};
template<class E> void Dispatch(){E e;for(auto& callback:callbacks<E>)if(callback)callback(e);}
}
namespace nova::compat {
bool IsLegacyLunar(){return fixture::legacy;} bool UsesModernRuntime(){return !fixture::legacy;}
namespace lunar {bool HeldSlotChangeReady(){return fixture::canSwitch;}}
}
namespace nova::input {bool IsProcessForeground(){return true;}}
namespace nova::clickgui {bool IsOpen(){return false;}}
namespace nova::notifications {enum class Type{Warning};void Post(std::string,Type){} }
namespace nova::java {
enum class KnownItem{Nothing,EnderPearl};enum class ClickKind{Swap};
struct EntityHandle {
    void* id=nullptr;
    explicit operator bool()const{return id;}
    void Reset(){id=nullptr;}void* raw()const{return id;}
    bool IsSame(const EntityHandle& e)const{return id==e.id;}
    bool IsAlive()const{return true;}bool IsSpectator()const{return false;}
};
EntityHandle AdoptEntity(void* p){return {p};}
EntityHandle PlayerEntity(){return {reinterpret_cast<void*>(1)};}
EntityHandle LevelEntity(){return {reinterpret_cast<void*>(static_cast<std::uintptr_t>(fixture::world+10))};}
struct ItemFacts{bool empty=true;int count=0;KnownItem item=KnownItem::EnderPearl;};
bool InventorySlot(int slot,jobject& out){out=&fixture::items[slot];return true;}
bool ReadItemFacts(jobject p,ItemFacts& facts){auto s=static_cast<fixture::Stack*>(p);facts.empty=s->count<=0;facts.count=s->count;facts.item=s->type==1?KnownItem::EnderPearl:KnownItem::Nothing;return true;}
bool CopyItemStack(jobject p,jobject& out){fixture::copied=*static_cast<fixture::Stack*>(p);out=&fixture::copied;return true;}
bool ItemStacksMatch(jobject a,jobject b){auto x=static_cast<fixture::Stack*>(a),y=static_cast<fixture::Stack*>(b);return x->type==y->type&&x->count==y->count;}
bool ScreenOpen(){return fixture::screen;}bool InventoryCursorEmpty(){return true;}
bool FreeCamActive(){return false;}bool PlayerUsingItem(){return false;}
bool PlayerMenuContainerId(int& out){out=0;return true;}
bool ClickMenuSlot(int,int slot,int hotbar,ClickKind){++fixture::swaps;std::swap(fixture::items[slot],fixture::items[hotbar]);return true;}
bool SelectedSlot(int& out){out=fixture::selected;return true;}
bool AcquirePhysicalSlot(const void* owner){if(fixture::slotOwner&&fixture::slotOwner!=owner)return false;fixture::slotOwner=owner;return true;}
bool OwnsPhysicalSlot(const void* owner){return fixture::slotOwner==owner;}
bool SetOwnedPhysicalSlot(const void* owner,int slot){assert(OwnsPhysicalSlot(owner));fixture::selected=slot;fixture::actions.push_back("select");return true;}
bool SyncSelectedItem(){fixture::actions.push_back("sync");return fixture::syncOk;}
void ReleaseSilentSlot(const void* owner){if(fixture::slotOwner==owner)fixture::slotOwner=nullptr;}
void RestoreUseKey(){++fixture::restores;}void SuppressUseKey(){}
bool ChargeSupported(){return !fixture::legacy;}bool ChargeSlotReady(int,KnownItem){return fixture::ready;}
bool SendMoveRotation(float,float,bool){fixture::actions.push_back("look");return true;}
bool UseSilentItem(KnownItem,float yaw,float pitch,const void*,bool& attempted){
    attempted=true;
    assert(fixture::rotationOwner&&fixture::slotOwner);
    assert(yaw==fixture::angle.yaw&&pitch==fixture::angle.pitch);
    fixture::actions.push_back("use");++fixture::uses;
    if(fixture::useOk)--fixture::items[fixture::selected].count;
    return fixture::useOk;
}
struct PearlPhaseState {
    rotation::Vec3 position{.5,64,.5};float yaw=0,forward=0,strafe=0;
    bool ground=true,horizontalCollision=true,ladder=false,blocked=false,risingLowPose=false;
};
PearlPhaseState state;
bool ReadPearlPhaseState(PearlPhaseState& out){out=state;return true;}
}
namespace nova::rotation {
bool CanSubmitItemUse(){return fixture::ready;}
void ReleaseAttack(const void*){}
bool SetExclusiveRotations(const void* owner,const Rotation& angle){
    if(!fixture::rotateReady)return false;
    fixture::rotationOwner=owner;fixture::angle=angle;fixture::actions.push_back("aim");return true;
}
void ReleaseExclusiveRotations(const void* owner){if(fixture::rotationOwner==owner)fixture::rotationOwner=nullptr;}
Rotation GetRotation(){return fixture::angle;}
}

// @PRODUCTION_MODULE@

using namespace nova;
void Tick(){event::Dispatch<event::GameTickEvent>();}
void Interact(){event::Dispatch<event::PrePlayerInteractEvent>();}
void CheckAim(){
    using module::pearlphase::Aim;
    for(int yaw=-720;yaw<=720;yaw+=90){
        auto a=Aim({.5,64,.5},float(yaw),0,0,false);
        assert(std::abs(std::remainder(double(a.yaw-yaw),360.))<.01&&a.pitch==85);
    }
    assert(Aim({.5,64,.5},0,1,1,false).yaw==315);
    assert(Aim({.5,64,.5},0,-1,0,false).yaw==180);
    assert(Aim({.5,64,.5},0,0,1,false).yaw==270);
    assert(Aim({.5,64.2,.5},0,0,0,false).pitch==81);
    assert(Aim({.5,-.9,.5},0,0,0,true).pitch==80);
    // Mirrored worlds and whole-turn camera rebases must select the same edge.
    std::mt19937 rng(83);std::uniform_real_distribution<double> d(-179,179);
    for(int i=0;i<10000;++i){
        const float yaw=float(d(rng));const double x=d(rng),z=d(rng);
        auto a=Aim({x,64,z},yaw,1,0,false),b=Aim({x-100,64,z+100},yaw,1,0,false);
        assert(a.yaw==b.yaw&&a.pitch==b.pitch);
        assert(std::isfinite(a.yaw)&&a.yaw>=0&&a.yaw<=360);
    }
}
int main(){
    CheckAim();
    auto run=[](auto test){fixture::Reset();java::state={};auto m=module::MakePearlPhaseModule();m->setEnabled(true);Tick();test(*m);};
    run([](auto& m){Interact();assert(fixture::uses==1&&!m.enabled&&fixture::selected==6);Interact();assert(fixture::uses==1);Tick();assert(fixture::selected==2&&!fixture::slotOwner&&!fixture::rotationOwner&&fixture::restores==1);});
    run([](auto& m){fixture::useOk=false;Interact();Tick();assert(!m.enabled&&fixture::uses==1&&fixture::selected==2);Interact();assert(fixture::uses==1);});
    run([](auto&){fixture::syncOk=false;Interact();assert(fixture::uses==0);fixture::syncOk=true;Tick();assert(fixture::selected==2&&!fixture::slotOwner);});
    run([](auto&){Interact();fixture::selected=4;Tick();assert(fixture::selected==4&&!fixture::slotOwner);});
    run([](auto& m){Interact();fixture::world=2;fixture::selected=3;Tick();assert(!m.enabled&&fixture::selected==3&&!fixture::slotOwner&&!fixture::rotationOwner);});
    run([](auto&){fixture::items[6]={};fixture::items[25]={1,3};Interact();assert(fixture::uses==1&&fixture::swaps==2&&fixture::items[2].type==4&&fixture::items[25].count==2);Tick();assert(fixture::selected==2&&!fixture::slotOwner);});
    run([](auto&){fixture::items[6]={};fixture::items[25]={1,3};fixture::useOk=false;Interact();Tick();assert(fixture::swaps==2&&fixture::items[2].type==4&&fixture::items[25].count==3&&!fixture::slotOwner);});
    run([](auto& m){m.props.at("Always").value=true;java::state.horizontalCollision=false;Interact();assert(fixture::uses==0);java::state.horizontalCollision=true;java::state.ground=false;Interact();assert(fixture::uses==0);java::state.ground=true;Interact();assert(fixture::uses==1&&m.enabled);Tick();Tick();Interact();assert(fixture::uses==1);});
    run([](auto& m){fixture::busy=true;Interact();assert(fixture::uses==0&&!fixture::slotOwner);fixture::busy=false;fixture::ready=false;Interact();assert(fixture::uses==0&&!fixture::rotationOwner);fixture::ready=true;fixture::rotateReady=false;Interact();assert(fixture::uses==0&&!fixture::slotOwner);m.setEnabled(false);Tick();});
    run([](auto& m){java::state.blocked=true;Interact();assert(!m.enabled&&fixture::uses==0);});
    run([](auto&){java::state.risingLowPose=true;Interact();assert(fixture::uses==0);});
    run([](auto&){fixture::legacy=true;Interact();auto look=std::find(fixture::actions.begin(),fixture::actions.end(),"look"),use=std::find(fixture::actions.begin(),fixture::actions.end(),"use");assert(look<use);Tick();assert(fixture::selected==2);});
    run([](auto& m){fixture::items={};Interact();assert(!m.enabled&&fixture::uses==0);});
    std::cout<<"PearlPhase: aim and 13 transaction/lifecycle scenarios passed\n";
}
