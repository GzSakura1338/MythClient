#include <cassert>
#include <cstdio>
#include <functional>
#include <memory>
#include <atomic>
#include <format>
#include <utility>
#include <map>
#include <mutex>
#include "minecraft/java/api/game_entity.h"
#include "minecraft/modules/player/autocharge/WindPlanner.h"
#include "minecraft/modules/player/autocharge/Targeting.h"
#include "minecraft/modules/player/autocharge/SlotHandoff.h"
#include "minecraft/modules/player/autocharge/ManualUseGate.h"
namespace mock {
using namespace nova;
using module::autocharge::Pearl;
int world=2,pearls=0,winds=0,slot=-1,delay=0;
rotation::Rotation pearlAngle{};
bool ground=true,screen=false,blocked=false,items=true,commit=false,useOK=true,manual=false,mace=false;
bool exists=false;
bool targetExists=false,teammate=false,bot=false;
bool landing=false,pearlHittable=false;
rotation::Vec3 targetFeet{0,0,8},targetVelocity{};
rotation::Vec3 feet{},movement{};
Pearl pearl{};
const void *owner=nullptr,*slotOwner=nullptr;
rotation::Rotation angle{};
java::EntityHandle Handle(int id){return java::AdoptEntity(reinterpret_cast<void*>(intptr_t(id)));}
}
void LogTo(const char*,...){}
namespace Properties {
struct NumberProp {double value;double get()const{return value;}};
struct BoolProperty {bool value;bool get()const{return value;}};
struct PropDesc {void* property;PropDesc(const char*,void* p):property(p){}void* getProp()const{return property;}};
}
namespace nova::event {
enum class EventType{Pre,Outgoing};
struct GameTickEvent{EventType type=EventType::Pre;};
struct PrePlayerInteractEvent{};struct RespawnEvent{};
struct ClickEvent{bool canceled=false;void Cancel(){canceled=true;}};
struct PacketEvent{EventType type=EventType::Outgoing;void* packet{};bool IsCancelled()const{return false;}};
namespace Priority {constexpr int Highest=0,Lowest=1000;}
template<class E>std::map<int,std::function<void(E&)>> callbacks;
struct Subscription{std::function<void()> reset;void Reset(){if(reset){reset();reset={};}}};
struct EventBus {
    static EventBus& Get(){static EventBus b;return b;}
    template<class E,class F>Subscription Subscribe(F f,int priority){
        assert(!callbacks<E>.contains(priority));callbacks<E>[priority]=f;
        return {[priority]{callbacks<E>.erase(priority);}};
    }
};
template<class E>void Post(E& e){for(auto& [p,f]:callbacks<E>)f(e);}
template<class E>void Post(){E e;Post(e);}
}
namespace nova::java {
enum class KnownItem{EnderPearl,WindCharge};
struct ChargeMovementPacket{enum class Kind{Move,EndTick};Kind kind=Kind::Move;bool hasPosition=true;rotation::Vec3 position{};bool onGround=true;};
struct MlgEnvironment{bool water=false,lava=false,flying=false,passenger=false;};
EntityHandle::~EntityHandle()=default;
EntityHandle::EntityHandle(EntityHandle&& o)noexcept:obj(std::exchange(o.obj,nullptr)){}
EntityHandle& EntityHandle::operator=(EntityHandle&& o)noexcept{obj=std::exchange(o.obj,nullptr);return *this;}
EntityHandle AdoptEntity(void* p){EntityHandle h;h.obj=p;return h;}
void EntityHandle::Reset(){obj=nullptr;}
bool EntityHandle::IsSame(const EntityHandle& h)const{return obj==h.obj;}
bool EntityHandle::IsAlive()const{return true;}
bool EntityHandle::IsSpectator()const{return false;}
bool EntityHandle::IsPlayer()const{return intptr_t(obj)==200||intptr_t(obj)==1;}
bool EntityHandle::Position(rotation::Vec3& out)const{out=intptr_t(obj)==100?mock::pearl.position:intptr_t(obj)==200?mock::targetFeet:mock::feet;return true;}
bool EntityHandle::PositionPair(rotation::Vec3& now,rotation::Vec3& before)const{Position(now);before=now.Subtract(mock::targetVelocity);return true;}
bool EntityHandle::OnGround(bool& out)const{out=mock::ground;return true;}
bool EntityHandle::EyeHeight(double& out)const{out=1.62;return true;}
bool EntityHandle::BoundingBox(Box& out)const{rotation::Vec3 p;Position(p);out={p.x-.3,p.y,p.z-.3,p.x+.3,p.y+1.8,p.z+.3};return true;}
EntityHandle PlayerEntity(){return mock::Handle(1);}
EntityHandle LevelEntity(){return mock::Handle(mock::world);}
EntityHandle EntityById(int id){if(id==200&&mock::targetExists)return mock::Handle(200);return id==100&&mock::exists&&mock::pearl.age>=mock::delay?mock::Handle(100):EntityHandle{};}
bool WorldPlayers(std::vector<EntityHandle>& out){out.push_back(mock::Handle(1));out.push_back(mock::Handle(201));if(mock::targetExists)out.push_back(mock::Handle(200));return true;}
bool EntityId(const EntityHandle& h,int& id){id=int(intptr_t(h.raw()));return true;}
bool WorldEntities(const EntityHandle&,const Box&,std::vector<EntityHandle>& out){auto p=EntityById(100);if(p)out.push_back(std::move(p));return true;}
bool ChargeSupported(){return true;}
bool ChargePearl(const EntityHandle& h,const EntityHandle&,rotation::Vec3& v){v=mock::pearl.velocity;return intptr_t(h.raw())==100;}
bool ChargeEntityHittable(const EntityHandle& h){return mock::pearlHittable&&intptr_t(h.raw())==100;}
Box BoxInflate(const Box& b,double){return b;}
bool WorldClip(const rotation::Vec3& from,const rotation::Vec3& to,const EntityHandle&,BlockHitResultInfo& hit,ClipMode){
    hit.kind=mock::blocked||(mock::landing&&to.y<from.y&&from.y<=mock::feet.y+.1)?HitKind::Block:HitKind::Miss;return true;
}
bool ChargeSlotReady(int s,KnownItem item){return mock::items&&s==(item==KnownItem::EnderPearl?2:5);}
bool SetSilentSlot(const void* p,int s){if(mock::slotOwner&&mock::slotOwner!=p)return false;mock::slotOwner=p;mock::slot=s;return true;}
void ReleaseSilentSlot(const void* p){if(p==mock::slotOwner){assert(!mock::commit);mock::slotOwner=nullptr;mock::slot=-1;}}
bool SyncSelectedItem(){return true;}
bool PlayerRotation(float& y,float& p){y=0;p=0;return true;}
bool PlayerSensitivity(double& out){out=.5;return true;}
bool ScreenOpen(){return mock::screen;}
bool FreeCamActive(){return false;}
bool FreeLookAngles(float&,float&){return false;}
bool ReadMlgEnvironment(MlgEnvironment& out){out={};return true;}
bool ReadChargeMovementPacket(void* p,ChargeMovementPacket& out){out=*static_cast<ChargeMovementPacket*>(p);return true;}
void SuppressUseKey(){mock::manual=true;}
void RestoreUseKey(){mock::manual=false;}
bool UseSilentItem(KnownItem item,float yaw,float pitch){
    using namespace module::autocharge;
    assert(!mock::commit&&mock::manual&&mock::owner&&mock::slotOwner);
    assert(yaw==mock::angle.yaw&&pitch==mock::angle.pitch);
    mock::commit=true;
    if(item==KnownItem::EnderPearl){assert(mock::slot==2);++mock::pearls;mock::exists=true;
        mock::pearlAngle={yaw,pitch};
        auto inherited=mock::movement;if(mock::ground)inherited.y=0;
        mock::pearl={mock::feet.Add(0,1.62-kPearlEyeOffset,0),Add(Look({yaw,pitch}).Scale(kLaunchSpeed),inherited),0};
    }else{assert(mock::slot==5);++mock::winds;}
    return mock::useOK;
}
}
namespace nova::rotation {
double Random(){return .5;}
Rotation GetRotation(){return mock::angle;}
bool SetExclusiveRotations(const void* p,Rotation a){if(mock::owner&&mock::owner!=p)return false;mock::owner=p;mock::angle=a;return true;}
void ReleaseExclusiveRotations(const void* p){if(p==mock::owner)mock::owner=nullptr;}
bool OwnsExclusiveRotations(const void* p){return p==mock::owner;}
bool CanSubmitItemUse(){return !mock::commit;}
}
namespace nova::module::autocharge {struct FlightObservation{void Tick(){}void Begin(const java::EntityHandle&,const java::EntityHandle&,int,Vec3,Vec3,Vec3){}};}
namespace nova::module::automace {bool IsWorking(){return mock::mace;}}
namespace nova::module::teams {bool IsTeammate(const java::EntityHandle&,const java::EntityHandle&){return mock::teammate;}}
namespace nova::module::antibot {bool IsBot(const java::EntityHandle&){return mock::bot;}}
namespace nova::module::automlg {bool IsWorking(){return false;}}
namespace nova::module::scaffold {bool IsEnabled(){return false;}}
namespace nova::module::refill {bool IsWorking(){return false;}}
namespace nova::module::autoblock {bool OwnsItemUse(){return false;}}
namespace nova::module::velocity {bool IsDelaying(){return false;}}
namespace nova::module::antikb {bool IsDelaying(){return false;}}
namespace nova::module::backtrack {bool IsLagging(){return false;}}
namespace nova::clickgui {bool IsOpen(){return false;}}
namespace nova::notifications {enum class Type{Warning};void Post(const std::string&,Type){}}
namespace nova::module {
enum class Category{Player};
class Module {
public:
    Module(const char*,Category,const char*,const char*){}virtual ~Module()=default;
    bool enabled=false;
    void setEnabled(bool value){if(value==enabled)return;enabled=value;if(value)onEnable();else onDisable();}
    std::map<std::string,Properties::NumberProp> numbers;
    std::map<std::string,Properties::BoolProperty> bools;
protected:
    virtual void onEnable(){}virtual void onDisable(){}
    Properties::NumberProp& addNumber(const char* name,double,double,double,double v,std::vector<int>,std::function<std::string(double)>){return numbers.insert_or_assign(name,Properties::NumberProp{v}).first->second;}
    Properties::PropDesc subNumber(const char* name,double,double,double,double v,std::function<std::string(double)>){return {name,&numbers.insert_or_assign(name,Properties::NumberProp{v}).first->second};}
    Properties::BoolProperty& addBool(const char* name,bool v,std::initializer_list<Properties::PropDesc>){return bools.insert_or_assign(name,Properties::BoolProperty{v}).first->second;}
};
}
// @PRODUCTION_MODULE@
using namespace nova;
void Tick(){
    mock::commit=false;
    if(mock::exists)module::autocharge::Step(mock::pearl);
    mock::feet=module::autocharge::Add(mock::feet,mock::movement);
    mock::targetFeet=module::autocharge::Add(mock::targetFeet,mock::targetVelocity);
    java::ChargeMovementPacket p;p.position=mock::feet;p.onGround=mock::ground;
    event::PacketEvent e{event::EventType::Outgoing,&p};event::Post(e);
    p.kind=java::ChargeMovementPacket::Kind::EndTick;event::Post(e);
    event::Post<event::GameTickEvent>();
}
void Interact(){event::Post<event::PrePlayerInteractEvent>();}
void Setup(module::Module& m){
    m.setEnabled(false);Tick();Tick();assert(!mock::owner&&!mock::slotOwner&&!mock::manual);
    mock::world=2;mock::pearls=mock::winds=mock::delay=0;
    mock::ground=mock::items=mock::useOK=true;
    mock::exists=mock::screen=mock::blocked=mock::mace=false;
    mock::feet=mock::movement={};
    mock::targetExists=mock::teammate=mock::bot=false;mock::targetFeet={0,0,8};mock::targetVelocity={};
    mock::landing=mock::pearlHittable=false;
    m.bools["Target Player"].value=false;m.numbers["Range"].value=16;m.numbers["FOV"].value=180;
    event::Post<event::RespawnEvent>();Tick();Tick();
    m.numbers["Rotation Speed"].value=180;m.numbers["Height"].value=20;m.setEnabled(true);
}
int main(){
    auto m=module::MakeAutoChargeModule();
    Setup(*m);Tick();Interact();assert(mock::pearls==1&&mock::winds==0&&mock::slot==2);
    event::ClickEvent click;event::Post(click);assert(click.canceled);
    for(int i=0;i<25&&m->enabled;++i){Tick();Interact();auto angle=mock::angle;Interact();assert(angle.yaw==mock::angle.yaw&&angle.pitch==mock::angle.pitch);}
    assert(mock::winds==1&&!m->enabled);Tick();assert(!mock::owner&&!mock::slotOwner&&!mock::manual);
    Setup(*m);mock::ground=false;mock::movement.y=.42;Tick();Interact();assert(!mock::pearls);
    mock::ground=true;mock::movement={};Tick();Interact();assert(mock::pearls==1);
    Setup(*m);mock::blocked=true;Tick();Interact();assert(!mock::pearls&&!m->enabled);
    Setup(*m);mock::items=false;Tick();Interact();assert(!mock::pearls&&!m->enabled);
    Setup(*m);Tick();mock::commit=true;Interact();assert(!mock::pearls);Tick();Interact();assert(mock::pearls==1);
    Setup(*m);mock::useOK=false;Tick();Interact();assert(mock::pearls==1&&!m->enabled&&mock::slot==2&&mock::manual);
    Interact();assert(mock::pearls==1);Tick();assert(!mock::slotOwner&&!mock::manual);
    Setup(*m);mock::delay=35;Tick();Interact();for(int i=0;i<32;++i){Tick();Interact();}
    assert(mock::pearls==1&&mock::winds==0&&!m->enabled);
    Setup(*m);Tick();Interact();module::autocharge::YieldForRescue();assert(!m->enabled&&mock::slot==2&&mock::manual);
    Tick();assert(!mock::slotOwner&&!mock::manual);
    Setup(*m);mock::mace=true;Tick();Interact();assert(!mock::pearls&&!m->enabled);
    Setup(*m);Tick();Interact();++mock::world;Tick();assert(!m->enabled&&!mock::owner&&!mock::slotOwner);Tick();assert(!mock::manual);
    Setup(*m);Tick();Interact();mock::screen=true;Tick();assert(!m->enabled&&!mock::owner);Tick();assert(!mock::manual);
    Setup(*m);Tick();Interact();m->setEnabled(false);assert(mock::slot==2);Tick();Tick();assert(!mock::slotOwner&&!mock::manual);
    Setup(*m);m->bools["Target Player"].value=true;mock::targetExists=true;
    for(int i=0;i<60&&m->enabled;++i){Tick();Interact();}
    assert(mock::pearls==1&&mock::winds==1&&!m->enabled);Tick();assert(!mock::slotOwner&&!mock::owner&&!mock::manual);
    for(int rejected=0;rejected<5;++rejected){
        Setup(*m);m->bools["Target Player"].value=true;mock::targetExists=rejected!=0;
        if(rejected==1)mock::teammate=true;if(rejected==2)mock::bot=true;
        if(rejected==3)mock::targetFeet={0,0,-8};if(rejected==4)m->numbers["Range"].value=4;
        for(int i=0;i<35&&m->enabled;++i){Tick();Interact();}
        assert(!m->enabled&&mock::pearls==1&&mock::winds==1); // No eligible target falls back normally.
    }
    Setup(*m);m->bools["Target Player"].value=true;mock::targetExists=true;mock::targetFeet={0,0,-8};m->numbers["FOV"].value=360;
    for(int i=0;i<60&&m->enabled;++i){Tick();Interact();}
    assert(mock::pearls==1&&mock::winds==1);
    Setup(*m);m->bools["Target Player"].value=true;mock::targetExists=true;
    for(int i=0;i<5&&!mock::pearls;++i){Tick();Interact();}assert(mock::pearls==1);
    mock::targetExists=false;
    for(int i=0;i<40&&m->enabled;++i){Tick();Interact();}
    assert(mock::winds==1&&!m->enabled);Tick();assert(!mock::manual);
    Setup(*m);m->bools["Target Player"].value=true;mock::targetExists=true;mock::blocked=true;
    for(int i=0;i<10&&m->enabled;++i){Tick();Interact();}assert(mock::pearls==0&&!m->enabled);
    rotation::Rotation first{};
    for(double height:{5.,25.,45.}){
        Setup(*m);m->bools["Target Player"].value=true;mock::targetExists=true;m->numbers["Height"].value=height;
        Tick();Interact();assert(mock::pearls==1);
        if(height==5)first=mock::angle;else assert(first.yaw==mock::angle.yaw&&first.pitch==mock::angle.pitch);
        // Crossing the initial Range after launch must not drop the lock.
        m->numbers["Range"].value=1;
        for(int i=0;i<60&&m->enabled;++i){Tick();Interact();}
        assert(mock::winds==1&&!m->enabled);
    }
    // Target escapes the original pearl path; completing the catch takes
    // priority over a no-longer-attainable destination band.
    Setup(*m);m->bools["Target Player"].value=true;mock::targetExists=true;
    Tick();Interact();assert(mock::pearls==1);mock::targetVelocity={.65,0,0};
    for(int i=0;i<50&&m->enabled;++i){Tick();Interact();}
    assert(mock::winds==1&&!m->enabled);
    // Losing a target while priming falls back without needing another bind.
    Setup(*m);m->bools["Target Player"].value=true;mock::targetExists=true;m->numbers["Rotation Speed"].value=20;
    Tick();Interact();assert(!mock::pearls);mock::targetExists=false;
    for(int i=0;i<50&&m->enabled;++i){Tick();Interact();}
    assert(mock::pearls==1&&mock::winds==1&&!m->enabled);
    // A selected player can be in range while both target arcs fail preflight.
    // The production path must finish a normal launch, not just turn and stop.
    for(double distance:{16.,24.,32.})for(double speed:{20.,180.}){
        Setup(*m);m->bools["Target Player"].value=true;mock::targetExists=true;
        mock::targetFeet={0,0,distance};m->numbers["Range"].value=32;m->numbers["Rotation Speed"].value=speed;
        for(int i=0;i<90&&m->enabled;++i){
            Tick();Interact();const auto angle=mock::angle;Interact();
            assert(angle.yaw==mock::angle.yaw&&angle.pitch==mock::angle.pitch);
        }
        assert(mock::pearls==1&&mock::winds==1&&!m->enabled&&mock::pearlAngle.pitch<=-89.5f);
        Tick();assert(!mock::slotOwner&&!mock::owner&&!mock::manual);
    }
    // Both short and high target arcs are physically unreachable, not merely
    // rejected by preflight. Exercise fallback from the Tick angle solver.
    Setup(*m);m->bools["Target Player"].value=true;mock::targetExists=true;
    mock::targetFeet={0,0,24};mock::targetVelocity={0,0,.8};m->numbers["Range"].value=32;
    for(int i=0;i<45&&m->enabled;++i){Tick();Interact();}
    assert(mock::pearls==1&&mock::winds==1&&!m->enabled&&mock::pearlAngle.pitch<=-89.5f);
    // Target disappears between Tick and interaction: no stale angle throw.
    Setup(*m);m->bools["Target Player"].value=true;mock::targetExists=true;
    Tick();mock::targetExists=false;Interact();assert(!mock::pearls&&m->enabled);
    for(int i=0;i<40&&m->enabled;++i){Tick();Interact();}
    assert(mock::pearls==1&&mock::winds==1&&!m->enabled&&mock::pearlAngle.pitch<=-89.5f);
    Setup(*m);mock::pearlHittable=true;
    for(int i=0;i<35&&m->enabled;++i){Tick();Interact();}
    assert(mock::pearls==1&&mock::winds==1); // Own tracked pearl is not a path blocker.
    // Do not spend either item with ambiguous inherited Y across landing.
    Setup(*m);mock::ground=false;mock::movement.y=-.37;mock::landing=true;
    Tick();Interact();assert(mock::pearls==0&&m->enabled);
    mock::ground=true;mock::movement={};mock::landing=false;Tick();Interact();assert(mock::pearls==1);
    mock::ground=false;mock::movement.y=-.37;mock::landing=true;Tick();Interact();assert(mock::winds==0&&m->enabled);
    mock::ground=true;mock::movement={};mock::landing=false;
    for(int i=0;i<35&&m->enabled;++i){Tick();Interact();}
    assert(mock::winds==1&&!m->enabled);
    m->setEnabled(false);Tick();Tick();
    m.reset();
    std::puts("PASS: production AutoCharge event flow, exact aim submission, one pearl/wind, ascent wait, obstruction, item conflicts, failure cleanup, observation timeout, MLG handoff and Mace priority");
}
