#include <cassert>
#include <cstdio>
#include <functional>
#include <memory>
#include <atomic>
#include <format>
#include <utility>
#include <map>
#include "minecraft/java/api/game_mace.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include "minecraft/utils/rotation/RayCastUtil.h"
#include "minecraft/utils/rotation/AttackTransaction.h"

namespace mock {
using namespace nova;
java::MaceState state{4,-.8,.1f,false,false,false,false};
int selected=0,world=2,slot=4,attacks=0,syncs=0,targetId=3,originalIdentity=100,maceIdentity=104;
int axeSlot=6,axeIdentity=106;
bool shield=false,axeAvailable=true,axeReachable=true;
std::vector<int> attackSlots;
bool supported=true,exists=true,reachable=true,blocked=false,screen=false,alive=true,attackResult=true,syncOK=true;
bool mlg=false,yieldAllowed=true,used=false,motion=false,locked=false,busy=false;
unsigned types=java::kTypeLiving|java::kTypePlayer;
float cameraYaw=0,cameraPitch=0;
const void* owner=nullptr;const void* slotOwner=nullptr;
rotation::Rotation angle{};
rotation::AttackTransaction attack;
java::EntityHandle Handle(int id){return java::AdoptEntity(reinterpret_cast<void*>(static_cast<intptr_t>(id)));}
}
void LogTo(const char*,...){}
namespace Properties {
struct NumberProp{double value;double get()const{return value;}};
struct ModeProperty{int value=1;int get()const{return value;}};
struct BoolProperty{bool value;bool get()const{return value;}};
struct PropDesc {
    std::string label;
    void* property=nullptr;
    std::vector<PropDesc> children;
    std::shared_ptr<BoolProperty> p;
    std::function<bool()> visible;
    PropDesc()=default;
    PropDesc(const char* name,void* prop,std::initializer_list<PropDesc> sub={}):label(name),property(prop),children(sub){}
    void* getProp()const{return p?p.get():property;}
};
}
namespace nova::event {
enum class EventType{Pre,Post};
struct GameTickEvent{EventType type=EventType::Pre;};struct PrePlayerInteractEvent{};struct RespawnEvent{};
namespace Priority {constexpr int Highest=0;}
template<class E>std::function<void(E&)> callback;
struct Subscription{std::function<void()> reset;void Reset(){if(reset){reset();reset={};}}};
struct EventBus {
    static EventBus& Get(){static EventBus b;return b;}
    template<class E,class F>Subscription Subscribe(F f,int priority){
        if constexpr(!std::is_same_v<E,RespawnEvent>)assert(priority<0); // Before MLG.
        callback<E> = f;return {[]{callback<E> = {};}};
    }
};
template<class E>void Post(E e={}){if(callback<E>)callback<E>(e);}
}
namespace nova::java {
EntityHandle::~EntityHandle()=default;
EntityHandle::EntityHandle(EntityHandle&& o)noexcept:obj(std::exchange(o.obj,nullptr)){}
EntityHandle& EntityHandle::operator=(EntityHandle&& o)noexcept{obj=std::exchange(o.obj,nullptr);return *this;}
EntityHandle AdoptEntity(void* p){EntityHandle h;h.obj=p;return h;}
void EntityHandle::Reset(){obj=nullptr;}
bool EntityHandle::IsSame(const EntityHandle& h)const{return obj==h.obj;}
bool EntityHandle::IsAlive()const{return mock::alive;}
bool EntityHandle::IsSpectator()const{return false;}
bool EntityHandle::Position(rotation::Vec3& out)const{out={0,2,0};return true;}
bool EntityHandle::EyePosition(float,rotation::Vec3& out)const{out={0,3.62,0};return true;}
bool EntityHandle::BoundingBox(Box& out)const{out={-.3,0,-.3,.3,1.8,.3};return true;}
EntityHandle PlayerEntity(){return mock::Handle(1);}
EntityHandle LevelEntity(){return mock::Handle(mock::world);}
EntityHandle EntityById(int id){return mock::exists&&id==mock::targetId?mock::Handle(id):EntityHandle{};}
bool EntityId(const EntityHandle&,int& id){id=mock::targetId;return true;}
bool EntityTypes(const EntityHandle&,unsigned& types){types=mock::types;return true;}
bool WorldEntities(const EntityHandle&,const Box&,std::vector<EntityHandle>& out){if(mock::exists)out.push_back(mock::Handle(mock::targetId));return true;}
bool ReadMaceState(MaceState& s){s=mock::state;return mock::supported;}
bool MaceSlotAvailable(int s){return s==mock::slot;}
bool MaceAxeSlotAvailable(int s){return mock::axeAvailable&&s==mock::axeSlot;}
bool MaceTargetBlockingShield(const EntityHandle&){return mock::shield;}
EntityHandle MaceSlotIdentity(int s){return mock::Handle(s==mock::slot?mock::maceIdentity:s==mock::axeSlot?mock::axeIdentity:mock::originalIdentity);}
bool MaceSlotMatches(int s,const EntityHandle& h){auto current=MaceSlotIdentity(s);return current.IsSame(h);}
bool MaceReach(int,double& reach){reach=3;return true;}
bool MaceCanReach(int s,const EntityHandle&){return mock::reachable&&(s!=mock::axeSlot||mock::axeReachable);}
bool SelectedSlot(int& s){s=mock::selected;return true;}
bool AcquirePhysicalSlot(const void* p){if(mock::slotOwner&&mock::slotOwner!=p)return false;mock::slotOwner=p;return true;}
bool OwnsPhysicalSlot(const void* p){return mock::slotOwner==p;}
bool SetOwnedPhysicalSlot(const void* p,int s){assert(OwnsPhysicalSlot(p));mock::selected=s;return true;}
void ReleaseSilentSlot(const void* p){if(p==mock::slotOwner)mock::slotOwner=nullptr;}
bool SyncSelectedItem(){++mock::syncs;return mock::syncOK;}
bool PlayerRotation(float& y,float& p){y=mock::cameraYaw;p=mock::cameraPitch;return true;}
bool PlayerSensitivity(double&){return false;}
bool ScreenOpen(){return mock::screen;}
bool FreeCamActive(){return false;}
bool FreeLookAngles(float&,float&){return false;}
bool AttackAndSwing(const EntityHandle&) {
    assert((mock::selected==mock::slot||mock::selected==mock::axeSlot)&&mock::syncs>0&&mock::owner);
    assert(mock::attack.Reserved()&&mock::attack.CanCall());
    assert(mock::attack.EnterNative());++mock::attacks;
    mock::attackSlots.push_back(mock::selected);
    assert(!mock::attack.EnterNative()); // Native call scope permits exactly one.
    return mock::attackResult;
}
}
namespace nova::rotation {
Rotation GetRotation(){return mock::owner?mock::angle:Rotation{mock::cameraYaw,mock::cameraPitch};}
Rotation ApplySensitivityPatch(Rotation a,Rotation,double){return a;}
Rotation Calculate(Vec3 a,Vec3 b){auto d=b.Subtract(a);return {float(std::atan2(d.z,d.x)*180/3.141592653589793-90),float(-std::atan2(d.y,std::hypot(d.x,d.z))*180/3.141592653589793)};}
Vec3 ClosestPointOnAabb(const java::Box& b,const Vec3& p){return {std::clamp(p.x,b.minX,b.maxX),std::clamp(p.y,b.minY,b.maxY),std::clamp(p.z,b.minZ,b.maxZ)};}
HitResult InteractionRayCast(const Rotation& r,double reach){HitResult h;if(mock::exists&&!mock::blocked&&mock::reachable&&reach>=1.82&&r.pitch>88){h.type=HitResultType::Entity;h.entity=mock::Handle(mock::targetId);}return h;}
bool SetExclusiveRotations(const void* p,Rotation a){if(mock::owner&&mock::owner!=p)return false;mock::owner=p;mock::angle=a;return true;}
void ReleaseExclusiveRotations(const void* p){if(p==mock::owner)mock::owner=nullptr;}
bool AcquireAttack(const void* p){return !mock::used&&!mock::motion&&!mock::locked&&mock::attack.Acquire(p);}
void ReleaseAttack(const void* p){mock::attack.Release(p);}
bool CanSubmitAttack(const void* p){return !mock::used&&!mock::motion&&!mock::locked&&mock::attack.Ready(p);}
bool BeginAttackSubmission(const void* p){if(!CanSubmitAttack(p)||!mock::attack.Begin(p))return false;mock::locked=true;return true;}
bool EndAttackSubmission(){return mock::attack.End();}
}
namespace nova::module::automlg {
bool YieldForMace(){if(!mock::yieldAllowed)return false;mock::mlg=false;return true;}
}
namespace nova::module::teams {bool IsTeammate(const java::EntityHandle&,const java::EntityHandle&){return false;}}
namespace nova::module::antibot {bool IsBot(const java::EntityHandle&){return false;}}
namespace nova::module::scaffold {bool IsEnabled(){return false;}}
namespace nova::module::autocharge {bool IsWorking(){return false;}}
namespace nova::module::refill {bool IsWorking(){return false;}}
namespace nova::module::autoblock {bool OwnsItemUse(){return mock::busy;}}
namespace nova::module::autorod {bool OwnsItemUse(){return false;}}
namespace nova::module::velocity {bool IsDelaying(){return false;}}
namespace nova::module::antikb {bool IsDelaying(){return false;}}
namespace nova::module::backtrack {bool IsLagging(){return false;}}
namespace nova::clickgui {bool IsOpen(){return false;}}
namespace nova::module {
enum class Category{Combat};
class Module {
public:
    Module(const char*,Category,const char*,const char*){}virtual ~Module()=default;
    void Enable(){onEnable();}void Disable(){onDisable();}
    Properties::ModeProperty mode;
    std::map<std::string,Properties::NumberProp> numbers;
    std::map<std::string,Properties::PropDesc> bools;
    // Match the production base constructor and registration order, rather
    // than preallocating fictitious rows that hide dropped-setting bugs.
    std::vector<Properties::PropDesc> rows{{"Enabled",nullptr},{"Bind",nullptr}};
protected:
    virtual void onEnable(){}virtual void onDisable(){}
    Properties::ModeProperty& addMode(const char* name,int v,std::initializer_list<const char*>){mode.value=v;rows.emplace_back(name,&mode);return mode;}
    Properties::NumberProp& addNumber(const char* name,double,double,double,double v,std::vector<int>,std::function<std::string(double)>){
        auto& prop=numbers.insert_or_assign(name,Properties::NumberProp{v}).first->second;rows.emplace_back(name,&prop);return prop;
    }
    Properties::PropDesc subBool(const char* name,bool v){Properties::PropDesc p;p.label=name;p.p=std::make_shared<Properties::BoolProperty>(v);bools[name]=p;return p;}
    Properties::BoolProperty& addBool(const char* name,bool v,std::vector<Properties::PropDesc>,std::function<bool()> visible){
        auto p=subBool(name,v);p.visible=std::move(visible);bools[name]=p;rows.push_back(p);return *p.p;
    }
    const std::vector<Properties::PropDesc>& settings(){return rows;}
    void replaceSettings(std::vector<Properties::PropDesc> p){rows=std::move(p);}
    void addSettings(std::vector<Properties::PropDesc> p){for(auto& row:p)rows.push_back(std::move(row));}
};
}
// @PRODUCTION_MODULE@
using namespace nova;
void Tick(){mock::used=mock::motion=mock::locked=false;mock::attack.BeginTick();event::Post<event::GameTickEvent>();}
void Interact(){event::Post<event::PrePlayerInteractEvent>();}
void Setup(module::Module& m,int mode=1) {
    m.Disable();Tick();assert(!mock::owner&&!mock::slotOwner&&!mock::attack.Reserved());
    mock::state={4,-.8,.1f,false,false,false,false};mock::selected=0;mock::world=2;mock::slot=4;
    mock::attacks=mock::syncs=0;mock::originalIdentity=100;mock::maceIdentity=104;mock::types=java::kTypeLiving|java::kTypePlayer;
    mock::axeSlot=6;mock::axeIdentity=106;mock::shield=false;mock::axeAvailable=mock::axeReachable=true;mock::attackSlots.clear();
    m.bools["Axe"].p->value=false;
    mock::supported=mock::exists=mock::reachable=mock::alive=mock::attackResult=mock::syncOK=mock::yieldAllowed=true;
    mock::blocked=mock::screen=mock::busy=mock::mlg=false;mock::cameraYaw=mock::cameraPitch=0;
    m.mode.value=mode;m.numbers["Rotation speed"].value=180;m.numbers["Range"].value=3;
    m.bools["Player"].p->value=true;m.bools["Mobs"].p->value=m.bools["Animals"].p->value=false;m.Enable();
}
int main(){
    auto m=module::MakeAutoMaceModule();
    const std::vector<std::string> expected{"Enabled","Bind","Mode","Range","FOV","Rotation speed","Axe","Target"};
    assert(m->rows.size()==expected.size());
    for(size_t i=0;i<expected.size();++i)assert(m->rows[i].label==expected[i]);
    assert(m->rows[5].getProp()==&m->numbers["Rotation speed"]);
    assert(m->rows[6].getProp()==m->bools["Axe"].p.get());
    m->mode.value=0;assert(m->rows[6].visible());
    m->mode.value=1;assert(!m->rows[6].visible());
    assert(m->rows[7].children.size()==3&&m->rows[7].children[0].label=="Player");
    std::puts("PASS: UI/config descriptors retain Enabled, Bind, rotation speed, Quick Switch-only Axe and grouped targets");
    for(int mode:{0,1}) {
        Setup(*m,mode);mock::mlg=true;Tick();assert(module::automace::IsWorking()&&!mock::mlg);
        assert(mock::selected==(mode==0?0:4));Interact();assert(mock::attacks==1&&mock::selected==4);
        Interact();assert(mock::attacks==1);Tick();assert(!module::automace::IsWorking()&&mock::selected==0);
        for(int i=0;i<6;++i){Tick();Interact();}assert(mock::attacks==1); // Same fall cannot rearm.
        mock::state.ground=true;mock::state.fall=0;Tick();mock::state={4,-.8,.1f,false,false,false,false};Tick();Interact();assert(mock::attacks==2);
    }
    Setup(*m);mock::state.fall=1;Tick();assert(mock::selected==4);Interact();assert(!mock::attacks);
    mock::state.fall=2;Tick();Interact();assert(mock::attacks==1); // No full cooldown gate.
    Setup(*m,0);mock::state.fall=1;Tick();assert(!module::automace::IsWorking()&&mock::selected==0);
    Setup(*m);m->numbers["Rotation speed"].value=20;Tick();Interact();assert(!mock::attacks);
    Tick();Interact();Tick();Interact();Tick();assert(!module::automace::IsWorking()&&mock::selected==0);
    for(int i=0;i<5;++i){Tick();Interact();}assert(!mock::attacks); // Expired aim hands off permanently this fall.
    Setup(*m);mock::reachable=false;mock::mlg=true;Tick();assert(mock::mlg&&!module::automace::IsWorking());
    Setup(*m);mock::blocked=true;Tick();assert(!module::automace::IsWorking());
    Setup(*m);mock::yieldAllowed=false;mock::mlg=true;Tick();assert(mock::mlg&&!mock::attack.Reserved());
    Setup(*m);Tick();mock::exists=false;Tick();assert(!module::automace::IsWorking()&&mock::selected==0);
    Setup(*m);Tick();mock::selected=7;Tick();assert(mock::selected==7&&!module::automace::IsWorking());
    Setup(*m);Tick();++mock::originalIdentity;Tick();assert(mock::selected==4&&!module::automace::IsWorking());
    Setup(*m);Tick();++mock::world;mock::selected=6;Tick();assert(mock::selected==6&&!module::automace::IsWorking());
    Setup(*m);Tick();m->Disable();assert(mock::selected==4);Tick();assert(mock::selected==0);
    Setup(*m);Tick();mock::attackResult=false;Interact();Tick();Tick();Interact();assert(mock::attacks==1&&mock::selected==0);
    Setup(*m);mock::types=java::kTypeLiving|java::kTypeAnimal;Tick();assert(!module::automace::IsWorking());
    m->bools["Mobs"].p->value=true;Tick();assert(!module::automace::IsWorking());
    m->bools["Animals"].p->value=true;Tick();Interact();assert(mock::attacks==1);
    Setup(*m);mock::state.interacting=true;Tick();assert(!module::automace::IsWorking());
    Setup(*m);mock::busy=true;Tick();assert(!module::automace::IsWorking());
    Setup(*m);Tick();mock::motion=true;Interact();assert(!mock::attacks);
    Setup(*m);Tick();mock::used=true;Interact();assert(!mock::attacks);
    Setup(*m);mock::syncOK=false;Tick();assert(!module::automace::IsWorking()&&mock::selected==0);
    Setup(*m);mock::supported=false;Tick();assert(!module::automace::IsWorking());
    // Optional axe is visible/effective only in Quick Switch, with an active shield.
    Setup(*m,0);assert(m->bools["Axe"].visible());m->bools["Axe"].p->value=true;
    mock::shield=true;mock::mlg=true;Tick();assert(mock::selected==0&&!mock::mlg);
    Interact();assert(mock::attackSlots==std::vector<int>{6}&&mock::selected==6&&module::automace::IsWorking());
    Interact();assert(mock::attacks==1); // Repeated callbacks cannot consume mace in axe tick.
    Tick();assert(module::automace::IsWorking());Interact();
    assert((mock::attackSlots==std::vector<int>{6,4})&&mock::selected==4);
    Interact();assert(mock::attacks==2);Tick();assert(mock::selected==0&&!module::automace::IsWorking());
    for(int i=0;i<4;++i){Tick();Interact();}assert(mock::attacks==2); // No repeated shield breaking this fall.
    for(int fallback=0;fallback<5;++fallback){
        Setup(*m,fallback==0?1:0);m->bools["Axe"].p->value=fallback!=4;mock::shield=fallback!=1;
        if(fallback==0)assert(!m->bools["Axe"].visible());
        if(fallback==2)mock::axeAvailable=false;
        if(fallback==3)mock::axeReachable=false;
        Tick();Interact();assert(mock::attackSlots==std::vector<int>{4});Tick();assert(mock::selected==0);
    }
    // Interruption between axe and mace must release MLG/rotation/slot ownership.
    for(int interruption=0;interruption<9;++interruption){
        Setup(*m,0);m->bools["Axe"].p->value=true;mock::shield=true;Tick();Interact();assert(mock::attacks==1);
        switch(interruption){
        case 0:mock::exists=false;break;
        case 1:m->Disable();break;
        case 2:mock::state.ground=true;break;
        case 3:mock::selected=7;break;
        case 4:++mock::world;mock::selected=7;break;
        case 5:m->mode.value=1;break;
        case 6:mock::busy=true;break;
        case 7:++mock::axeIdentity;break;
        case 8:m->bools["Axe"].p->value=false;break;
        }
        Tick();Interact();assert(mock::attacks==1&&!module::automace::IsWorking());
        assert(!mock::owner&&!mock::slotOwner&&!mock::attack.Reserved());
        assert(mock::selected==((interruption==3||interruption==4)?7:interruption==7?6:0));
    }
    Setup(*m,0);m->bools["Axe"].p->value=true;mock::shield=true;Tick();mock::attackResult=false;
    Interact();Tick();Interact();assert(mock::attacks==1&&mock::selected==0&&!module::automace::IsWorking());
    Setup(*m,0);m->bools["Axe"].p->value=true;mock::shield=true;Tick();Interact();Tick();mock::motion=true;
    Interact();assert(mock::attacks==1);Tick();assert(mock::selected==0&&!module::automace::IsWorking());
    // If the original held item is the axe/mace itself, restore that exact slot.
    for(int original:{4,6}){
        Setup(*m,0);m->bools["Axe"].p->value=true;mock::shield=true;mock::selected=original;
        Tick();Interact();Tick();Interact();Tick();assert((mock::attackSlots==std::vector<int>{6,4})&&mock::selected==original);
    }
    m->Disable();Tick();m.reset();
    std::puts("PASS: production AutoMace quick/normal, axe-shield-mace sequence, one native attack per tick, switchback, bounded aim/follow-up, MLG priority/fallback, slot/world/disable safety and interaction conflicts");
}
