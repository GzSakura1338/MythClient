#include <cassert>
#include <cstdio>
#include <functional>
#include <memory>
#include <atomic>
#include <chrono>
#include <format>
#include <utility>
#include "minecraft/modules/player/automlg/Prediction.h"
#include "minecraft/modules/player/automlg/WaterSearch.h"
#include "minecraft/modules/player/automlg/WebPlacement.h"
#include "minecraft/java/api/game_mlg.h"
#include "minecraft/java/game/MlgWaterState.h"
#include "minecraft/utils/rotation/LinearAim.h"
namespace mock {
using namespace nova;
struct Clock {using time_point=std::chrono::steady_clock::time_point;static time_point now(){return time_point{}+std::chrono::milliseconds(ms);}static inline long long ms=1000;};
double y=4.4,vy=-.8; float fall=4; int selected=0,switches=0,uses=0,world=2,suspends=0;
bool ground=false,inWater=false,water=false,allow=true,screen=false,denied=false,blocked=false,flowing=false,ackOnly=false;
bool burning=false,web=false;
bool headWeb=false,headBlocked=false;
bool itemUseAvailable=true;
bool maceWorking=false;
java::MlgBucket bucket=java::MlgBucket::Water;
const void* owner=nullptr;
const void* slotOwner=nullptr;int silentSlot=-1;
rotation::Rotation angle{};
}
namespace Properties {
struct NumberProp {double value;double get()const{return value;}};
struct BoolProperty {bool value=true;bool get()const{return value;}};
struct Sub {std::shared_ptr<NumberProp> p;void* getProp()const{return p.get();}};
}
namespace nova::event {
enum class EventType{Pre,Post};
struct GameTickEvent{EventType type=EventType::Pre;};
struct PrePlayerInteractEvent{};struct RespawnEvent{};
struct MotionEvent{EventType type=EventType::Post;float yaw=0,pitch=90;};
namespace Priority {constexpr int Highest=0,Lowest=20;}
template<class E> std::function<void(E&)> callback;
struct Subscription{std::function<void()> reset;void Reset(){if(reset){reset();reset={};}}};
struct EventBus{static EventBus& Get(){static EventBus b;return b;}template<class E,class F> Subscription Subscribe(F f,int){callback<E> = f;return {[]{callback<E> = {};}};}};
template<class E> void Post(E e={}){if(callback<E>)callback<E>(e);}
}
namespace nova::rotation {
bool CanSubmitItemUse(){return mock::itemUseAvailable;}
bool AutomatedItemUseSubmitted(){return !mock::itemUseAvailable;}
struct Manager{bool hasRotations=false,active=false;};
Manager& State(){static Manager m;return m;}
Rotation GetRotation(){return mock::angle;}
Rotation ApplySensitivityPatch(Rotation a,Rotation,double){return a;}
Rotation Calculate(Vec3 a,Vec3 b){auto d=b.Subtract(a);return {float(std::atan2(d.z,d.x)*180/3.141592653589793-90),float(-std::atan2(d.y,std::hypot(d.x,d.z))*180/3.141592653589793)};}
bool OwnsExclusiveRotations(const void* p){return p==mock::owner;}
bool SetExclusiveRotations(const void* p,Rotation a){if(mock::owner&&mock::owner!=p)return false;mock::owner=p;mock::angle=a;State()={true,true};return true;}
void ReleaseExclusiveRotations(const void* p){if(p==mock::owner){mock::owner=nullptr;State()={};}}
}
namespace nova::java {
EntityHandle::~EntityHandle()=default;
EntityHandle::EntityHandle(EntityHandle&& o)noexcept:obj(std::exchange(o.obj,nullptr)){}
EntityHandle& EntityHandle::operator=(EntityHandle&& o)noexcept{obj=std::exchange(o.obj,nullptr);return *this;}
EntityHandle AdoptEntity(void* p){EntityHandle h;h.obj=p;return h;}
void EntityHandle::Reset(){obj=nullptr;}
bool EntityHandle::IsSame(const EntityHandle& h)const{return obj==h.obj;}
bool EntityHandle::IsAlive()const{return true;}
bool EntityHandle::IsSpectator()const{return false;}
bool EntityHandle::EyePosition(float,rotation::Vec3& out)const{out={.5,mock::y+1.62,.5};return true;}
bool EntityHandle::BoundingBox(Box& out)const{out={.2,mock::y,.2,.8,mock::y+1.8,.8};return true;}
EntityHandle PlayerEntity(){return AdoptEntity((void*)1);}
EntityHandle LevelEntity(){return AdoptEntity(reinterpret_cast<void*>(static_cast<intptr_t>(mock::world)));}
bool PlayerDeltaMovement(rotation::Vec3& out){out={0,mock::vy,0};return true;}
bool PlayerFallDistance(float& out){out=mock::fall;return true;}
bool PlayerOnGround(){return mock::ground;}
bool ScreenOpen(){return mock::screen;}
bool ReadMlgEnvironment(MlgEnvironment& out){out={mock::inWater,false,false,false};return true;}
bool ReadMlgHazards(bool& burning,bool& web,BlockPos& pos){burning=mock::burning;web=mock::web;pos={0,mock::headWeb?1:0,0};return true;}
bool SelectedSlot(int& out){out=mock::selected;return true;}
bool SetSelectedSlot(int v){mock::selected=v;++mock::switches;return true;}
bool SyncSelectedItem(){return true;}
bool SetSilentSlot(const void* owner,int slot){if(mock::slotOwner&&mock::slotOwner!=owner)return false;mock::slotOwner=owner;mock::silentSlot=slot;return true;}
void ReleaseSilentSlot(const void* owner){if(mock::slotOwner==owner){mock::slotOwner=nullptr;mock::silentSlot=-1;}}
bool EffectiveSelectedSlot(int& out){out=mock::silentSlot>=0?mock::silentSlot:mock::selected;return true;}
MlgBucket MlgBucketAt(int s){return s==4?mock::bucket:MlgBucket::None;}
bool MlgAllowsWater(const BlockPos&){return mock::allow;}
bool BlockStateReplaceable(const BlockPos&){return true;}
bool MlgWaterAt(const BlockPos& pos,bool& water,bool& source){water=mock::water&&pos.x==0&&pos.y==0&&pos.z==0;source=water&&!mock::flowing;return true;}
bool PlayerSensitivity(double&){return false;}
bool PlayerRotation(float& yaw,float& pitch){yaw=pitch=0;return true;}
bool Vec3DirectionFromRotation(float pitch,float yaw,rotation::Vec3& out){double p=pitch*3.141592653589793/180,y=yaw*3.141592653589793/180;out={-std::sin(y)*std::cos(p),-std::sin(p),std::cos(y)*std::cos(p)};return true;}
bool WorldClip(const rotation::Vec3& a,const rotation::Vec3& b,const EntityHandle&,BlockHitResultInfo& hit,ClipMode mode){
    if(mode==ClipMode::Outline&&mock::headWeb&&mock::web&&!mock::headBlocked&&b.y>2&&a.y<2){
        const double t=(2-a.y)/(b.y-a.y),x=a.x+(b.x-a.x)*t,z=a.z+(b.z-a.z)*t;
        if(x>0&&x<1&&z>0&&z<1){hit={};hit.kind=HitKind::Block;hit.hasBlockPos=true;hit.blockPos={0,1,0};hit.location={x,2,z};hit.direction=1;return true;}
    }
    const double plane=mode==ClipMode::WaterSource&&mock::water&&!mock::flowing?1:0;
    if(mode==ClipMode::WaterSource&&mock::blocked){hit={};hit.kind=HitKind::Block;hit.hasBlockPos=true;hit.blockPos={1,0,0};return true;}
    hit={};if(a.y>=plane&&b.y<=plane&&a.y>b.y){double f=(a.y-plane)/(a.y-b.y);hit.kind=HitKind::Block;hit.hasBlockPos=true;hit.location={a.x+(b.x-a.x)*f,plane,a.z+(b.z-a.z)*f};hit.blockPos={int(std::floor(hit.location.x)),plane==1?0:-1,int(std::floor(hit.location.z))};hit.direction=1;}return true;
}
bool UseMlgBucket(MlgBucket expected,float yaw,float pitch){assert(mock::selected==0&&mock::silentSlot==4);assert(yaw==mock::angle.yaw&&pitch==mock::angle.pitch);++mock::uses;if(mock::denied)return false;if(mock::ackOnly)return true;if(expected==MlgBucket::Water){mock::water=true;mock::bucket=MlgBucket::Empty;}else{mock::water=false;mock::bucket=MlgBucket::Water;}return true;}
}
namespace nova::compat {bool IsLegacyLunar(){return false;}bool UsesModernRuntime(){return true;}namespace lunar{bool PlayerReportedRotation(float&,float&){return true;}}}
namespace nova::module::autocharge {bool IsWorking(){return false;}void YieldForRescue(){}}
namespace nova::module::automace {bool IsWorking(){return mock::maceWorking;}}
namespace nova::clickgui {bool IsOpen(){return false;}}
namespace nova::module {
namespace autoblock {void SuspendForInteraction(){++mock::suspends;}}
enum class Category{Player};
class Module {
public:Module(const char*,Category,const char*,const char*){}virtual ~Module()=default;void Enable(){onEnable();}void Disable(){onDisable();}
Properties::NumberProp minimum{3},speed{45};Properties::BoolProperty recovery,collect,fire,web;
std::vector<Properties::Sub> subs,collectSubs,fireSubs,webSubs;
protected:virtual void onEnable(){}virtual void onDisable(){}
Properties::NumberProp& addNumber(const char* name,double,double,double,double v,std::vector<int>,std::function<std::string(double)>){auto& p=std::string_view(name)=="Rotation Speed"?speed:minimum;p.value=v;return p;}
Properties::Sub subNumber(const char*,double,double,double step,double v,std::function<std::string(double)>){assert(step==5&&v==0);return {std::make_shared<Properties::NumberProp>(v)};}
Properties::BoolProperty& addBool(const char* name,bool v,std::vector<Properties::Sub> p={}){
    if(std::string_view(name)=="Auto Collect Water"){collect.value=v;collectSubs=p;return collect;}
    if(std::string_view(name)=="Anti Fire"){fire.value=v;fireSubs=p;return fire;}
    if(std::string_view(name)=="Anti Web"){web.value=v;webSubs=p;return web;}
    recovery.value=v;subs=p;return recovery;
}
};
}
// @PRODUCTION_MODULE@
using namespace nova;
void Tick(int elapsed=50){mock::Clock::ms+=elapsed;event::Post<event::GameTickEvent>();}
void Interact(){event::Post<event::PrePlayerInteractEvent>();}
void Publish(){event::Post<event::MotionEvent>({event::EventType::Post,mock::angle.yaw,mock::angle.pitch});}
void Start(module::Module& m){m.Enable();Tick();Tick(300);assert(module::automlg::IsWorking());assert(mock::selected==0);}
void NearLanding(){mock::y=2.9;mock::vy=-.95;Tick();assert(mock::selected==0&&mock::silentSlot==4&&mock::switches==0);}
void Reset(){mock::y=4.4;mock::vy=-.8;mock::fall=4;mock::selected=0;mock::headWeb=mock::headBlocked=mock::burning=mock::web=mock::ground=mock::inWater=mock::water=mock::screen=mock::denied=mock::blocked=mock::flowing=mock::ackOnly=false;mock::allow=true;mock::bucket=java::MlgBucket::Water;mock::switches=mock::uses=0;}
void CollectSetup(module::Module& m){m.Disable();Tick();Reset();mock::y=0;mock::vy=0;mock::fall=0;mock::ground=true;mock::water=true;mock::bucket=java::MlgBucket::Empty;m.Enable();Tick();}
int main(){
    assert(java::mlg::IsSourceState("minecraft:water[level=0]"));
    assert(java::mlg::IsSourceState("Block{minecraft:water}[level=0]"));
    assert(!java::mlg::IsSourceState("Block{minecraft:water}[level=10]"));
    assert(!java::mlg::IsWaterState("Block{minecraft:water_cauldron}[level=0]"));
    assert(!java::mlg::IsWaterState("Block{minecraft:oak_slab}[waterlogged=true]"));
    using namespace module::automlg;
    assert(!SwitchReady(4,2,-1)&&!SwitchReady(3,8,-1)&&SwitchReady(3,5,-1));
    Landing l;java::Box box{.2,3,.2,.8,4.8,.8};
    int calls=0;assert(!Predict(box,{0,-.8,0},[&](auto,auto,auto& h){++calls;h={};return true;},l)&&calls==90);
    assert(!Predict(box,{0,-.8,0},[](auto,auto,auto&){return false;},l));
    assert(!Predict(box,{0,-.8,0},[](auto a,auto b,auto& h){h.kind=java::HitKind::Block;h.hasBlockPos=true;h.direction=2;h.location=a.Add((b.x-a.x)*.5,(b.y-a.y)*.5,(b.z-a.z)*.5);return true;},l));
    assert(!SafeToRecover(true,true,false,4,-1)&&!SafeToRecover(false,false,true,0,0)&&SafeToRecover(true,true,false,0,0));
    WaterSearch search;java::BlockPos found;int reads=0;
    assert(!search.Step({.5,1.62,.5},[&](auto){++reads;return false;},[](auto,auto){return true;},found));
    assert(reads<=64); // World/JNI work is bounded per tick, including empty areas.
    auto m=module::MakeAutoMLGModule();m->subs[0].p->value=150;Start(*m);
    mock::y=4.2;mock::vy=-.86;Tick();assert(mock::selected==0); // No early bucket switch.
    NearLanding();Interact();assert(mock::uses==0); // Must publish rotation first.
    Publish();Interact();assert(mock::uses==0); // Still outside vanilla reach.
    mock::y=2.2;Tick();Publish();
    mock::itemUseAvailable=false;Interact();assert(mock::uses==0&&IsWorking());
    mock::itemUseAvailable=true;Interact();assert(mock::uses==1&&mock::water);
    Tick(200);Publish();Interact();assert(mock::uses==1); // Water exists but player still falling.
    mock::y=.05;mock::vy=0;mock::fall=0;mock::inWater=true;Tick();
    Tick(100);Publish();Interact();assert(mock::uses==1); // Recovery delay starts after safe landing.
    Tick(50);Publish();Interact();assert(mock::uses==2&&!mock::water);Tick();
    assert(mock::selected==0&&!IsWorking()&&!mock::owner&&mock::silentSlot==-1&&mock::switches==0);
    m->Disable();Tick();Reset();Start(*m);NearLanding();mock::selected=7;Tick();assert(mock::selected==7&&!IsWorking());
    m->Disable();Tick();Reset();Start(*m);NearLanding();m->Disable();Tick();assert(mock::selected==0&&!IsWorking());
    Reset();Start(*m);NearLanding();++mock::world;Tick();assert(mock::selected==0&&mock::silentSlot==-1&&!IsWorking()); // Drop the lease without changing the new world's physical slot.
    m->Disable();Tick();Reset();mock::allow=false;m->Enable();Tick();Tick(300);assert(!IsWorking()&&mock::switches==0);
    mock::allow=true;mock::bucket=java::MlgBucket::None;Tick();assert(!IsWorking());
    m->Disable();Tick();Reset();Start(*m);NearLanding();mock::screen=true;Tick();assert(!IsWorking()&&mock::selected==0);
    m->Disable();Tick();Reset();m->speed.value=10;Start(*m);
    assert(std::abs(std::hypot(mock::angle.yaw,mock::angle.pitch)-10)<.001);
    auto previous=mock::angle;NearLanding();
    assert(std::hypot(mock::angle.yaw-previous.yaw,mock::angle.pitch-previous.pitch)<=10.001);
    Publish();Interact();assert(mock::uses==0); // Slow rotation must not bypass the aim/reach gate.
    m->speed.value=180;Tick();Publish(); // Live speed change reaches the target.
    mock::y=2.2;Tick();const auto next=mock::angle;
    Interact();assert(mock::uses==1); // Modern USE_ITEM must use the upcoming motion's exact angle.
    assert(mock::angle.yaw==next.yaw&&mock::angle.pitch==next.pitch);
    m->Disable();Tick();Reset();m->collect.value=false;Start(*m);NearLanding();
    mock::y=2.2;Tick();Publish();Interact();assert(mock::uses==1);
    mock::y=.05;mock::vy=0;mock::fall=0;mock::ground=true;mock::inWater=false;
    Tick();Tick(150);Publish();Interact();assert(mock::uses==2);Tick();assert(!IsWorking());
    m->collect.value=true;
    CollectSetup(*m);m->recovery.value=false; // Collection does not require a preceding rescue.
    Tick(300);assert(IsWorking()&&mock::silentSlot==-1);Tick();Publish();Interact();
    assert(mock::uses==1&&mock::bucket==java::MlgBucket::Water&&mock::selected==0);Tick();assert(!IsWorking());
    CollectSetup(*m);mock::blocked=true;for(int i=0;i<20;++i)Tick();assert(!IsWorking()&&mock::uses==0);
    CollectSetup(*m);mock::flowing=true;for(int i=0;i<20;++i)Tick();assert(!IsWorking());
    CollectSetup(*m);mock::y=6;for(int i=0;i<20;++i)Tick();assert(!IsWorking());
    CollectSetup(*m);mock::bucket=java::MlgBucket::None;Tick(300);assert(!IsWorking());
    CollectSetup(*m);m->collect.value=false;Tick(300);assert(!IsWorking());m->collect.value=true;
    CollectSetup(*m);Tick(300);Tick();mock::water=false;Publish();Interact();assert(mock::uses==0);Tick();assert(!IsWorking());
    CollectSetup(*m);mock::ackOnly=true;Tick(300);Tick();Publish();Interact();assert(mock::uses==1);
    Tick(701);mock::ackOnly=false;Tick();Publish();Interact();assert(mock::uses==2&&mock::bucket==java::MlgBucket::Water);Tick();assert(!IsWorking());
    CollectSetup(*m);Tick(300);Tick();m->collect.value=false;Tick();assert(!IsWorking()&&mock::silentSlot==-1);
    m->collect.value=true;
    CollectSetup(*m);mock::fall=5;mock::vy=-1;Tick(300);assert(!IsWorking()&&mock::uses==0);
    CollectSetup(*m);m->collectSubs[0].p->value=105;Tick(300);Tick(100);Publish();Interact();
    assert(mock::uses==0&&mock::silentSlot==-1);Tick(5);Publish();Interact();assert(mock::uses==1);Tick();
    m->collectSubs[0].p->value=0;
    auto hazardSetup=[&](bool fire){m->Disable();Tick();Reset();mock::y=0;mock::vy=0;mock::fall=0;mock::ground=true;
        mock::burning=fire;mock::web=!fire;m->fire.value=fire;m->web.value=!fire;m->recovery.value=true;m->subs[0].p->value=0;
        m->Enable();Tick();};
    hazardSetup(true);m->fireSubs[0].p->value=105;Tick(300);Tick(100);assert(!IsWorking());Tick(5);assert(IsWorking());
    Tick();Publish();Interact();assert(mock::uses==1&&mock::water&&mock::switches==0);
    Tick(200);Publish();Interact();assert(mock::uses==1); // Keep water until the fire has actually stopped.
    mock::burning=false;Tick();Publish();Interact();assert(mock::uses==2);Tick();assert(!IsWorking());
    hazardSetup(true);Tick(300);Tick(100);mock::burning=false;Tick();mock::burning=true;Tick();Tick(100);assert(!IsWorking());
    Tick(5);assert(IsWorking());m->fire.value=false;Tick();assert(!IsWorking());
    m->fireSubs[0].p->value=0;hazardSetup(true);mock::allow=false;Tick(300);assert(!IsWorking()&&mock::uses==0);
    hazardSetup(false);m->webSubs[0].p->value=55;Tick(300);Tick(50);assert(!IsWorking());Tick(5);assert(IsWorking());
    Tick();Publish();Interact();assert(mock::uses==1);Tick(200);Publish();Interact();assert(mock::uses==1);
    mock::web=false;Tick();Publish();Interact();assert(mock::uses==2);Tick();assert(!IsWorking());
    m->webSubs[0].p->value=0;hazardSetup(false);Tick(300);assert(IsWorking());
    Tick();mock::web=false;Tick();assert(!IsWorking()&&mock::uses==0); // Released from web before placing.
    hazardSetup(true);mock::bucket=java::MlgBucket::None;Tick(300);assert(!IsWorking());
    hazardSetup(false);mock::headWeb=true;Tick(300);assert(IsWorking());
    Tick();Publish();Interact();assert(mock::uses==1&&mock::angle.pitch<0); // Water ABOVE the head web, never at the feet.
    hazardSetup(false);mock::headWeb=true;mock::headBlocked=true;Tick(300);
    assert(!IsWorking()&&mock::uses==0&&mock::silentSlot==-1); // No valid high placement: no floor fallback.
    hazardSetup(false);mock::headWeb=true;mock::inWater=true;Tick(300);assert(IsWorking());
    Tick();Publish();Interact();assert(mock::uses==1); // Feet already in water does not clear a head-level web.
    hazardSetup(false);mock::headWeb=true;mock::burning=true;m->fire.value=true;Tick(300);Tick();Publish();Interact();
    assert(mock::uses==1&&mock::angle.pitch<0); // Fire must not redirect a head-web rescue toward the feet.
    assert(!CanFlowToWeb({0,0,0},{0,1,0}));
    assert(CanFlowToWeb({0,2,0},{0,1,0})&&CanFlowToWeb({1,1,0},{0,1,0}));
    assert(!CanFlowToWeb({2,1,0},{0,1,0}));
    Landing side;
    assert(FindWebPlacement({.5,1.62,.5},{0,1,0},[](auto,auto,auto& hit){
        hit={};hit.kind=java::HitKind::Block;hit.hasBlockPos=true;hit.blockPos={0,1,0};hit.direction=5;hit.location={1,1.62,.5};return true;
    },[](auto pos){return pos.x==1&&pos.y==1;},side));
    assert(Adjacent(side.hit).x==1&&Adjacent(side.hit).y==1);
    m->Disable();Tick();Reset();m->fire.value=m->web.value=false;Start(*m);NearLanding();
    assert(YieldForMace()&&!IsWorking()&&mock::silentSlot==-1&&!mock::owner);
    mock::maceWorking=true;Tick();Publish();Interact();assert(!IsWorking()&&mock::uses==0);
    mock::maceWorking=false;Tick();assert(IsWorking()); // No 250ms cooldown on handback.
    NearLanding();mock::y=2.2;Tick();Publish();
    mock::maceWorking=true;Interact();assert(mock::uses==0); // Recheck at submission too.
    mock::maceWorking=false;mock::itemUseAvailable=false;
    assert(!YieldForMace()&&IsWorking()); // Never roll back an already submitted use.
    mock::itemUseAvailable=true;
    m->Disable();Tick();m.reset();assert(!mock::owner);
    std::puts("PASS: mace preempts MLG preparation, blocks bucket submission, immediate fallback and submitted-use preservation");
    std::puts("PASS: head web upward placement, no lower fallback, side flow, feet-water coexistence and web-over-fire priority");
    std::puts("PASS: all delay defaults/5ms steps, collection timing, fire/web delays and reset, no early recovery, live disable, no water/Nether and zero-delay activation");
    std::puts("PASS: independent auto collection, no rescue prerequisite, source-only/visibility/reach, no bucket, live toggle, source disappearance, rollback retry and fall safety");
    std::puts("PASS: configurable AutoMLG smoothing, per-tick angular budget, live speed change, unsent-angle gate and interaction rotation restore");
    std::puts("PASS: production AutoMLG tick timing, published rotation/reach, safe recovery/delay, restore, cancellation, Nether/no bucket, prediction bounds and water states");
}
