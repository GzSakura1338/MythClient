#include <cassert>
#include <cstdio>
#include <functional>
#include <memory>
#include <atomic>
#include <format>
#include <utility>
#include <chrono>
#include <map>
#include <thread>
#include "minecraft/java/api/game_crystal.h"
#include "minecraft/java/api/game_mace.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include "minecraft/utils/rotation/RotationUtils.h"
#include "minecraft/utils/rotation/RayCastUtil.h"
#include "minecraft/utils/rotation/AttackTransaction.h"
#include "minecraft/modules/combat/legitcrystalaura/CrystalRules.h"
#include "minecraft/modules/combat/legitcrystalaura/PlacementPrediction.h"
#include "minecraft/modules/combat/legitcrystalaura/ExplosionExposure.h"
#include "minecraft/modules/combat/legitcrystalaura/PlacementEsp.h"
#include "minecraft/modules/combat/shared/ExplosionCoordinator.h"

namespace mock {
using namespace nova;
bool extraEnemy=false;
int world=2,selected=0,oldIdentity=100,uses=0,attacks=0,swings=0;
int spoof=-1,slotWrites=0,swingPackets=0;
bool yieldable=false,physicalLease=false;
bool quantized=false;
bool selfGrounded=true,targetGrounded=true;
bool projectedWall=false;int projectedReads=0,virtualBaseReads=0;
bool fallingWorld=false;
int baseQueries=0,exposureQueries=0;
double attackReach=3;
int baseUses=0;
bool baseExists=true,obsidian=true,baseSpace=true,supportExists=true;
bool extraBase=false,extraExists=true;
java::BlockPos lastPlaced;
bool crystal=false,target=true,team=false,bot=false,teammate=false,wall=false,rescue=false,screen=false;
bool supported=true,stock=true,defenseOK=true,selfDanger=false,dropOnAim=false,syncOK=true;
bool used=false,motion=false,locked=false,disableAfterUse=false;
double targetY=64,targetX=3.4,targetZ=1;
rotation::Vec3 observedVelocity{};
rotation::Vec3 selfPosition{0,64,0},selfVelocity{};
const void* owner=nullptr;const void* slotOwner=nullptr;
rotation::Rotation angle{};
rotation::AttackTransaction attack;
java::EntityHandle Handle(int id){return java::AdoptEntity(reinterpret_cast<void*>(static_cast<intptr_t>(id)));}
int Id(const java::EntityHandle& e){return int(reinterpret_cast<intptr_t>(e.raw()));}
java::BlockPos base{2,63,0};
java::Box cube{2,63,0,3,64,1},crystalBox{1.5,64,-.5,3.5,66,1.5};
struct TestBase {java::BlockPos pos;bool built;double exposure=1,selfExposure=0;bool occluded=false;};
std::vector<TestBase> bases;
std::vector<java::BlockPos> evaluated;
bool Same(const java::BlockPos& a,const java::BlockPos& b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
const TestBase* FindBase(const java::BlockPos& p){for(const auto& base:bases)if(Same(base.pos,p))return &base;return nullptr;}
bool Ray(const rotation::Vec3& from,const rotation::Vec3& to,const java::Box& box,double& distance,int& face) {
    auto d=to.Subtract(from);double low=0,high=1;face=1;
    const double p[]{from.x,from.y,from.z},v[]{d.x,d.y,d.z},a[]{box.minX,box.minY,box.minZ},b[]{box.maxX,box.maxY,box.maxZ};
    for(int i=0;i<3;++i) {
        if(std::abs(v[i])<1e-10){if(p[i]<a[i]||p[i]>b[i])return false;continue;}
        double u=(a[i]-p[i])/v[i],w=(b[i]-p[i])/v[i];int side=i==0?4:i==1?0:2;
        if(u>w){std::swap(u,w);++side;}if(u>low){low=u;face=side;}high=std::min(high,w);if(low>high)return false;
    }
    distance=low*from.DistanceTo(to);return high>=0&&low<=1;
}
}
namespace Properties {
struct NumberProp{double value;double get()const{return value;}};
struct BoolProperty{bool value;bool get()const{return value;}};
struct PropDesc {
    std::string label;void* prop=nullptr;std::vector<PropDesc> children;
    PropDesc(const char* n,void* p,std::vector<PropDesc> c={}):label(n),prop(p),children(std::move(c)){}
    void* getProp()const{return prop;}
};
}
namespace nova::event {
enum class EventType{Pre,Post};struct GameTickEvent{EventType type=EventType::Pre;};struct PrePlayerInteractEvent{};struct RespawnEvent{};
struct MotionEvent{EventType type;float yaw,pitch;};
namespace Priority{constexpr int Highest=0,High=5,Lowest=20;}
template<class E>std::map<std::pair<int,size_t>,std::function<void(E&)>> callbacks;
template<class E>size_t callbackSerial=0;
struct Subscription{std::function<void()> reset;void Reset(){if(reset){reset();reset={};}}};
struct EventBus {
    static EventBus& Get(){static EventBus bus;return bus;}
    template<class E,class F>Subscription Subscribe(F f,int priority){
        const auto key=std::make_pair(priority,++callbackSerial<E>);
        callbacks<E>[key] = f;return {[key]{callbacks<E>.erase(key);}};
    }
};
template<class E>void Post(E e={}){for(auto& [priority,callback]:callbacks<E>)callback(e);}
}
namespace nova::java {
EntityHandle::~EntityHandle()=default;
EntityHandle::EntityHandle(EntityHandle&& e)noexcept:obj(std::exchange(e.obj,nullptr)){}
EntityHandle& EntityHandle::operator=(EntityHandle&& e)noexcept{obj=std::exchange(e.obj,nullptr);return *this;}
EntityHandle AdoptEntity(void* p){EntityHandle e;e.obj=p;return e;}
void EntityHandle::Reset(){obj=nullptr;}
bool EntityHandle::IsSame(const EntityHandle& e)const{return obj==e.obj;}
bool EntityHandle::IsAlive()const{return mock::Id(*this)==4?mock::crystal:true;}
bool EntityHandle::IsSpectator()const{return false;}
bool EntityHandle::IsPlayer()const{return mock::Id(*this)==1||mock::Id(*this)==3||mock::Id(*this)==5||mock::Id(*this)==6;}
bool EntityHandle::Position(rotation::Vec3& p)const{
    switch(mock::Id(*this)){case 1:p=mock::selfPosition;break;case 3:p={mock::targetX,mock::targetY,mock::targetZ};break;case 4:p={2.5,64,.5};break;default:p={3,64,2};}return true;
}
bool EntityHandle::PositionPair(rotation::Vec3& p,rotation::Vec3& before)const{Position(p);before=mock::Id(*this)==3?p.Subtract(mock::observedVelocity):mock::Id(*this)==1?p.Subtract(mock::selfVelocity):p;return true;}
bool EntityHandle::OnGround(bool& out)const{out=mock::Id(*this)==1?mock::selfGrounded:mock::targetGrounded;return true;}
bool CrystalMotionClear(const EntityHandle&,const Box& box){
    if(!mock::fallingWorld)return true;
    return box.minY>=63&&(!mock::baseExists||!nova::module::crystal::Overlaps(box,mock::cube));
}
bool EntityHandle::EyePosition(float,rotation::Vec3& p)const{Position(p);p.y+=1.62;return true;}
bool EntityHandle::BoundingBox(Box& box)const{rotation::Vec3 p;Position(p);box=mock::Id(*this)==4?mock::crystalBox:Box{p.x-.3,p.y,p.z-.3,p.x+.3,p.y+1.8,p.z+.3};return true;}
bool EntityHandle::DistanceTo(const EntityHandle& e,double& d)const{rotation::Vec3 a,b;Position(a);e.Position(b);d=a.DistanceTo(b);return true;}
EntityHandle PlayerEntity(){return mock::Handle(1);}EntityHandle LevelEntity(){return mock::Handle(mock::world);}
EntityHandle EntityById(int id){return (id==3&&mock::target)||(id==4&&mock::crystal)||(id==6&&mock::extraEnemy)?mock::Handle(id):EntityHandle{};}
bool EntityId(const EntityHandle& e,int& id){id=mock::Id(e);return true;}
bool WorldPlayers(std::vector<EntityHandle>& out){out.push_back(mock::Handle(1));if(mock::target)out.push_back(mock::Handle(3));if(mock::teammate)out.push_back(mock::Handle(5));if(mock::extraEnemy)out.push_back(mock::Handle(6));return true;}
bool WorldEntities(const EntityHandle&,const Box&,std::vector<EntityHandle>& out){if(mock::crystal)out.push_back(mock::Handle(4));return true;}
bool CrystalSupported(){return mock::supported;}bool CrystalContextReady(){return mock::supported;}
bool IsEndCrystal(const EntityHandle& e){return mock::Id(e)==4;}
bool CrystalHotbarSlot(int slot){return mock::stock&&slot==4;}
bool CrystalBlockReach(double& reach){reach=4.5;return true;}
bool CrystalBase(const BlockPos& p){++mock::baseQueries;if(!mock::bases.empty()){auto base=mock::FindBase(p);return base&&base->built;}
    return p.y==63&&p.z==0&&((mock::baseExists&&p.x==2)||(mock::extraBase&&mock::extraExists&&p.x==-3));}
bool PlacementEntitiesClear(const BlockPos& p,bool building){
    const auto volume=CrystalPlacementVolume(p,building);
    std::vector<EntityHandle> players;WorldPlayers(players);
    for(const auto& player:players){Box bounds;player.BoundingBox(bounds);
        if(nova::module::crystal::Overlaps(volume,bounds))return false;}
    return !mock::crystal;
}
bool CrystalSpaceClear(const BlockPos& p){return PlacementEntitiesClear(p,false);}
bool CrystalPlaceable(const BlockPos& p){return CrystalBase(p)&&CrystalSpaceClear(p);}
bool CrystalObsidianSlot(int slot){return mock::obsidian&&slot==5;}
bool CrystalBaseBuildable(const BlockPos& p){if(!mock::bases.empty()){auto base=mock::FindBase(p);return base&&!base->built&&PlacementEntitiesClear(p,true);}
    return mock::baseSpace&&PlacementEntitiesClear(p,true)&&p.y==63&&p.z==0&&
    ((!mock::baseExists&&p.x==2)||(mock::extraBase&&!mock::extraExists&&p.x==-3));}
bool CrystalBaseSupport(const BlockPos& p){if(!mock::bases.empty())return mock::FindBase({p.x,p.y+1,p.z})!=nullptr;
    return mock::supportExists&&(p.x==2||(mock::extraBase&&p.x==-3))&&p.y==62&&p.z==0;}
bool PlaceCrystalBase(const BlockPos& base,const BlockHitResultInfo& hit,int slot,const void* owner,bool& entered){
    assert(CrystalBaseBuildable(base)&&CrystalSpaceClear(base)&&CrystalBaseSupport(hit.blockPos)&&hit.direction==1);
    assert(slot==5&&(mock::spoof<0?mock::selected:mock::spoof)==5&&mock::slotOwner==owner&&mock::owner==owner&&!mock::used&&!mock::motion);
    if(!mock::attack.Acquire(owner)){entered=false;return false;}
    mock::used=mock::locked=entered=true;mock::lastPlaced=base;++mock::baseUses;return true;
}
bool ReadCrystalDefense(const EntityHandle& e,CrystalDefense& d){d={};d.health=20;
    if(mock::fallingWorld&&mock::Id(e)==3){d.armor=20;d.toughness=8;}
    return mock::defenseOK;}
bool CrystalExposure(const EntityHandle& e,const rotation::Vec3& point,double& exposure){
    ++mock::exposureQueries;
    if(const auto* base=mock::FindBase({int(std::floor(point.x)),int(std::floor(point.y))-1,int(std::floor(point.z))})){
        exposure=mock::Id(e)==1?base->selfExposure:base->exposure;
        if(mock::Id(e)==3&&std::none_of(mock::evaluated.begin(),mock::evaluated.end(),[&](auto p){return mock::Same(p,base->pos);}))mock::evaluated.push_back(base->pos);
        return true;
    }
    exposure=mock::Id(e)==1&&!mock::selfDanger?0:(mock::extraEnemy&&mock::Id(e)==3?.05:1);return true;
}
bool CrystalExposureAt(const EntityHandle& e,const rotation::Vec3& point,const Box& bounds,const BlockPos* base,double& exposure){
    ++mock::projectedReads;if(base)++mock::virtualBaseReads;
    if(mock::fallingWorld&&mock::Id(e)==3) {
        const Box solid=base?CrystalPlacementVolume(*base,true):mock::cube;
        return nova::module::crystal::SampleExposure(bounds,point,base||mock::baseExists?&solid:nullptr,
            [](auto,auto,bool& clear){clear=true;return true;},exposure);
    }
    if(mock::projectedWall&&mock::Id(e)==3&&std::abs((bounds.minX+bounds.maxX)*.5-mock::targetX)>.05){exposure=0;return true;}
    return CrystalExposure(e,point,exposure);
}
bool PlaceEndCrystal(const BlockHitResultInfo& hit,int slot,const void* owner,bool& entered){
    assert(hit.hasBlockPos&&CrystalPlaceable(hit.blockPos)&&mock::owner&&mock::slotOwner);
    assert(slot==4&&(mock::spoof<0?mock::selected:mock::spoof)==4&&!mock::used&&!mock::motion&&!mock::attack.Reserved());
    if(!mock::attack.Acquire(owner)){entered=false;return false;}
    mock::used=mock::locked=true;entered=true;mock::lastPlaced=hit.blockPos;++mock::uses;assert(!mock::attack.CanCall());return true;
}
bool SelectedSlot(int& slot){slot=mock::selected;return true;}
bool EffectiveSelectedSlot(int& slot){slot=mock::spoof<0?mock::selected:mock::spoof;return true;}
EntityHandle MaceSlotIdentity(int slot){return mock::Handle(slot==0?mock::oldIdentity:100+slot);}
bool MaceSlotMatches(int slot,const EntityHandle& e){return MaceSlotIdentity(slot).IsSame(e);}
bool MaceReach(int,double& reach){reach=mock::attackReach;return true;}
bool MaceCanReach(int,const EntityHandle&){return true;}
bool AcquirePhysicalSlot(const void* p){if(mock::slotOwner&&mock::slotOwner!=p&&!mock::yieldable)return false;
    if(mock::slotOwner!=p)mock::spoof=-1;if(mock::spoof>=0)return false;
    mock::slotOwner=p;mock::physicalLease=true;mock::yieldable=false;return true;}
bool OwnsPhysicalSlot(const void* p){return p==mock::slotOwner&&mock::physicalLease;}
bool OwnsSlot(const void* p){return p==mock::slotOwner;}
void YieldSlot(const void* p){if(p==mock::slotOwner)mock::yieldable=true;}
bool SetOwnedPhysicalSlot(const void* p,int slot){assert(OwnsPhysicalSlot(p));mock::selected=slot;++mock::slotWrites;return true;}
void ReleaseSilentSlot(const void* p){if(p==mock::slotOwner){mock::slotOwner=nullptr;mock::spoof=-1;mock::physicalLease=mock::yieldable=false;}}
bool SetSilentSlot(const void* p,int slot){if(mock::slotOwner&&mock::slotOwner!=p&&!mock::yieldable)return false;
    if(mock::slotOwner==p&&mock::physicalLease)return false;mock::physicalLease=false;mock::yieldable=false;mock::slotOwner=p;mock::spoof=slot;
    if(!mock::syncOK){ReleaseSilentSlot(p);return false;}return true;}
bool SyncSelectedItem(){return mock::syncOK;}
bool PlayerRotation(float& yaw,float& pitch){yaw=pitch=0;return true;}
bool PlayerSensitivity(double& v){v=.5;return mock::quantized;}
bool ScreenOpen(){return mock::screen;}bool FreeCamActive(){return false;}bool FreeLookAngles(float&,float&){return false;}
bool SwingMainHand(){++mock::swings;return true;}
bool AttackEntity(const EntityHandle& e){assert(mock::Id(e)==4&&mock::crystal&&mock::owner&&mock::attack.EnterNative());++mock::attacks;return true;}
bool SwingHand(){++mock::swings;return true;}
bool SendSwingPacket(){++mock::swingPackets;return true;}
bool Vec3DirectionFromRotation(float pitch,float yaw,rotation::Vec3& out){const double k=3.141592653589793/180;out={-std::sin(yaw*k)*std::cos(pitch*k),-std::sin(pitch*k),std::cos(yaw*k)*std::cos(pitch*k)};return true;}
bool WorldClip(const rotation::Vec3& from,const rotation::Vec3& to,const EntityHandle&,BlockHitResultInfo& hit,ClipMode){
    double d=0;int face=1;hit={};if(mock::wall)return true;
    if(!mock::bases.empty()) {
        double nearest=1e9;
        for(const auto& entry:mock::bases){
            if(entry.occluded)continue;
            auto block=entry.pos;if(!entry.built)--block.y;
            const Box shape{double(block.x),double(block.y),double(block.z),double(block.x+1),double(block.y+1),double(block.z+1)};
            if(mock::Ray(from,to,shape,d,face)&&d<nearest){
                nearest=d;hit.kind=HitKind::Block;hit.hasBlockPos=true;hit.blockPos=block;hit.direction=face;
                const auto delta=to.Subtract(from);hit.location=from.Add(delta.x*d/from.DistanceTo(to),delta.y*d/from.DistanceTo(to),delta.z*d/from.DistanceTo(to));
            }
        }
        return true;
    }
    const Box shape=mock::baseExists?mock::cube:Box{2,62,0,3,63,1};
    if(mock::Ray(from,to,shape,d,face)){hit.kind=HitKind::Block;hit.hasBlockPos=true;hit.blockPos=mock::base;if(!mock::baseExists)--hit.blockPos.y;hit.direction=face;auto delta=to.Subtract(from);hit.location=from.Add(delta.x*d/from.DistanceTo(to),delta.y*d/from.DistanceTo(to),delta.z*d/from.DistanceTo(to));}return true;
}
}
namespace nova::rotation {
Rotation GetRotation(){return mock::angle;}
Rotation ApplySensitivityPatch(const Rotation& a,const Rotation& from,double,bool){return {
    from.yaw+float(std::floor((a.yaw-from.yaw)/.15+.5)*.15),
    from.pitch+float(std::floor((a.pitch-from.pitch)/.15+.5)*.15)};}
Rotation Calculate(const Vec3& a,const Vec3& b){auto d=b.Subtract(a);return {float(std::atan2(d.z,d.x)*180/3.141592653589793-90),float(-std::atan2(d.y,std::hypot(d.x,d.z))*180/3.141592653589793)};}
Vec3 ClosestPointOnAabb(const java::Box& b,const Vec3& p){return {std::clamp(p.x,b.minX,b.maxX),std::clamp(p.y,b.minY,b.maxY),std::clamp(p.z,b.minZ,b.maxZ)};}
HitResult InteractionRayCast(const Rotation& r,double reach){
    HitResult h;if(mock::wall)return h;Vec3 eye,d;java::PlayerEntity().EyePosition(1,eye);java::Vec3DirectionFromRotation(r.pitch,r.yaw,d);
    auto to=eye.Add(d.x*reach,d.y*reach,d.z*reach);double distance=0;int face=0;
    if(mock::crystal&&mock::Ray(eye,to,mock::crystalBox,distance,face)){h.type=HitResultType::Entity;h.entity=mock::Handle(4);}
    else if(!mock::bases.empty()) {
        java::BlockHitResultInfo block;java::WorldClip(eye,to,java::PlayerEntity(),block,java::ClipMode{});
        if(block.kind==java::HitKind::Block)h.type=HitResultType::Block;
    }
    else if(mock::Ray(eye,to,mock::baseExists?mock::cube:java::Box{2,62,0,3,63,1},distance,face))h.type=HitResultType::Block;
    return h;
}
bool SetExclusiveRotations(const void* p,Rotation a){if(mock::owner&&mock::owner!=p)return false;mock::owner=p;mock::angle=a;if(mock::dropOnAim)mock::targetY=63;return true;}
bool OwnsExclusiveRotations(const void* p){return mock::owner==p;}
void ReleaseExclusiveRotations(const void* p){if(p==mock::owner)mock::owner=nullptr;}
bool CanSubmitItemUse(){return !mock::used&&!mock::locked&&!mock::motion&&!mock::attack.Reserved();}
bool AcquireAttack(const void* p){return !mock::locked&&!mock::motion&&mock::attack.Acquire(p);}
void ReleaseAttack(const void* p){mock::attack.Release(p);}
bool CanSubmitAttack(const void* p){return !mock::locked&&!mock::motion&&mock::attack.Ready(p);}
bool BeginAttackSubmission(const void* p){if(!CanSubmitAttack(p)||!mock::attack.Begin(p))return false;mock::locked=true;return true;}
bool EndAttackSubmission(){return mock::attack.End();}
}
namespace nova::module::teams {bool IsTeammate(const java::EntityHandle&,const java::EntityHandle& e){return mock::Id(e)==5||mock::team;}}
namespace nova::module::antibot {bool IsBot(const java::EntityHandle&){return mock::bot;}}
namespace nova::module::scaffold {bool IsEnabled(){return false;}}
namespace nova::module::automace {bool IsWorking(){return mock::rescue;}}
namespace nova::module::automlg {bool IsWorking(){return mock::rescue;}}
namespace nova::module::autocharge {bool IsWorking(){return mock::rescue;}}
namespace nova::module::refill {bool IsWorking(){return false;}}
namespace nova::module::autoblock {bool OwnsItemUse(){return false;}}
namespace nova::module::autorod {bool OwnsItemUse(){return false;}}
namespace nova::module::velocity {bool IsDelaying(){return false;}}
namespace nova::module::antikb {bool IsDelaying(){return false;}}
namespace nova::module::backtrack {bool IsLagging(){return false;}}
namespace nova::clickgui {bool IsOpen(){return false;}}
namespace nova::input {bool IsProcessForeground(){return true;}}
namespace nova::notifications {enum class Type{Warning};void Post(const char*,Type){}}
namespace nova::module::crystal {
PlacementEsp::Snapshot rendered;
void PlacementEsp::Draw(){rendered=Read();}
}
namespace nova::module {
enum class Category{Combat};
class Module {
public:
    Module(const char*,Category,const char*,const char*){}virtual ~Module()=default;
    void Enable(){onEnable();}void Disable(){onDisable();}void setEnabled(bool v){if(!v)onDisable();}
    void Render(){onRender();}
    std::vector<Properties::PropDesc> rows{{"Enabled",nullptr},{"Bind",nullptr}};
    std::vector<std::shared_ptr<void>> props;
    void* Find(const std::string& path) {
        auto* level=&rows;size_t start=0;
        while(start<path.size()) {
            const auto dot=path.find('.',start);
            auto it=std::find_if(level->begin(),level->end(),[&](const auto& row){return row.label==path.substr(start,dot-start);});
            if(it==level->end())break;
            if(dot==std::string::npos)return it->prop;
            level=&it->children;start=dot+1;
        }
        assert(false);return nullptr;
    }
    void Number(const char* name,double v){static_cast<Properties::NumberProp*>(Find(name))->value=v;}
    void Bool(const char* name,bool v){static_cast<Properties::BoolProperty*>(Find(name))->value=v;}
protected:
    virtual void onEnable(){}virtual void onDisable(){}virtual void onRender(){}
    void enableOverlay(){}
    Properties::PropDesc subBool(const char* name,bool v){auto p=std::make_shared<Properties::BoolProperty>(v);props.push_back(p);return {name,p.get()};}
    Properties::PropDesc subNumber(const char* name,double,double,double,double v,std::function<std::string(double)>){auto p=std::make_shared<Properties::NumberProp>(v);props.push_back(p);return {name,p.get()};}
    Properties::BoolProperty& addBool(const char* name,bool v,std::vector<Properties::PropDesc> children={}){auto p=subBool(name,v);p.children=std::move(children);rows.push_back(p);return *static_cast<Properties::BoolProperty*>(p.prop);}
    void addSettings(std::vector<Properties::PropDesc> more){for(auto& row:more)rows.push_back(std::move(row));}
};
}
template<class... Args>void LogTo(const char*,Args...){}
// @PRODUCTION_MODULE@

void Tick(){
#ifdef NOVA_TEST_PLACEMENT_WIRE
    const auto before=mock::angle;const int uses=mock::uses+mock::baseUses;
#endif
    mock::used=mock::motion=mock::locked=false;mock::attack.BeginTick();nova::event::Post<nova::event::GameTickEvent>();nova::event::Post<nova::event::PrePlayerInteractEvent>();
#ifdef NOVA_TEST_PLACEMENT_WIRE
    if(mock::uses+mock::baseUses>uses)nova::rotation::ObservePlacementPacket();
    nova::rotation::ObservePlacementMovement(mock::angle.yaw!=before.yaw||mock::angle.pitch!=before.pitch,mock::angle.yaw,mock::angle.pitch);
#endif
    nova::event::Post(nova::event::MotionEvent{nova::event::EventType::Post,mock::angle.yaw,mock::angle.pitch});}
void Scan(){for(int i=0;i<24&&mock::uses==0&&mock::attacks==0;++i)Tick();}
void ScanBase(){for(int i=0;i<100&&mock::baseUses==0&&mock::uses==0;++i)Tick();}
std::unique_ptr<nova::module::Module> Start(){
    mock::world=2;mock::selected=0;mock::oldIdentity=100;mock::uses=mock::attacks=mock::swings=0;
    mock::selfPosition={0,64,0};mock::selfVelocity={};
    mock::quantized=false;
    mock::selfGrounded=mock::targetGrounded=true;
    mock::projectedWall=false;mock::projectedReads=mock::virtualBaseReads=0;
    mock::fallingWorld=false;
    mock::baseQueries=mock::exposureQueries=0;mock::attackReach=3;
    mock::bases.clear();mock::evaluated.clear();
    mock::spoof=-1;mock::slotWrites=mock::swingPackets=0;mock::yieldable=mock::physicalLease=false;
    mock::baseUses=0;mock::baseExists=mock::obsidian=mock::baseSpace=mock::supportExists=true;
    mock::extraBase=false;mock::extraExists=true;mock::lastPlaced={};
    mock::crystal=mock::team=mock::bot=mock::teammate=mock::wall=mock::rescue=mock::screen=false;
    mock::selfDanger=mock::dropOnAim=mock::used=mock::motion=mock::locked=false;
    mock::supported=mock::stock=mock::defenseOK=mock::syncOK=mock::target=true;
    mock::targetY=64;mock::targetX=3.4;mock::targetZ=1;mock::observedVelocity={};mock::owner=mock::slotOwner=nullptr;
    mock::angle=nova::rotation::Calculate({0,65.62,0},{2.5,64,.5});mock::attack={};
#ifdef NOVA_TEST_PLACEMENT_WIRE
    nova::rotation::ResetPlacementWire();
    nova::rotation::ObservePlacementMovement(true,mock::angle.yaw,mock::angle.pitch);
    nova::rotation::ObservePlacementMovement(true,mock::angle.yaw,mock::angle.pitch);
#endif
    auto m=nova::module::MakeLegitCrystalAuraModule();m->Number("Rotation.Speed",180);m->Number("Target.FOV",360);
    assert(static_cast<Properties::NumberProp*>(m->Find("Place.Delay"))->value==0);
    assert(static_cast<Properties::NumberProp*>(m->Find("Place.Auto Base.Place Delay"))->value==0);
    assert(static_cast<Properties::NumberProp*>(m->Find("Break.Delay"))->value==0);
    assert(!static_cast<Properties::BoolProperty*>(m->Find("Target.Through Walls"))->value);
    assert(!static_cast<Properties::BoolProperty*>(m->Find("Safety.Ignore Self Damage"))->value);
    assert(!static_cast<Properties::BoolProperty*>(m->Find("Place.Auto Base"))->value);
    assert(!static_cast<Properties::BoolProperty*>(m->Find("Place.Silent"))->value);
    assert(static_cast<Properties::BoolProperty*>(m->Find("Swing"))->value);
    m->Number("Place.Delay",0);m->Number("Break.Delay",0);m->Enable();return m;
}
int main() {
    using namespace nova::module::crystal;
    {auto m=Start();nova::module::explosive::autoCrystalEnabled=true;Scan();assert(mock::uses==0);
     nova::module::explosive::autoCrystalEnabled=false;Scan();assert(mock::uses==1);m->Disable();Tick();}
    {const nova::java::Box above{.2,1,.2,.8,2.8,.8},below{.2,-2,.2,.8,-.2,.8},solid{0,0,0,1,1,1};
     const nova::rotation::Vec3 explosion{.5,1,.5};double exposure=0;int rays=0;
     auto clear=[&](auto,auto,bool& out){++rays;out=true;return true;};
     assert(SampleExposure(above,explosion,&solid,clear,exposure)&&exposure==1&&rays==45);
     rays=0;assert(SampleExposure(below,explosion,&solid,clear,exposure)&&exposure==0&&rays==0);
     assert(SampleExposure(above,explosion,nullptr,[](auto p,auto,bool& out){out=p.x>.4;return true;},exposure));
     assert(exposure>0&&exposure<1);
     assert(!SampleExposure(above,explosion,nullptr,[](auto,auto,bool&){return false;},exposure));}
    {auto m=Start();mock::observedVelocity={.25,0,0};mock::projectedWall=true;
     Tick();assert(mock::projectedReads>0&&mock::uses==0);
     mock::projectedWall=false;mock::observedVelocity={};Scan();assert(mock::uses==1);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;ScanBase();
     assert(mock::baseUses==1&&mock::virtualBaseReads>0);}
    // A medium-priority aura must see the crystal reservation before it can
    // rotate or attack; idle/disabled/unsafe crystal work must yield immediately.
    {auto m=Start();int yielded=0,available=0;
     auto aura=nova::event::EventBus::Get().Subscribe<nova::event::GameTickEvent>([&](auto&){
         if(IsWorking()){++yielded;assert(mock::owner);return;}
         ++available;
     },10);
     mock::angle={};m->Number("Rotation.Speed",1);Tick();assert(yielded==1&&available==0&&mock::uses==0);
     mock::stock=false;Tick();assert(available==1&&!IsWorking());
     mock::stock=true;mock::selfDanger=true;Tick();assert(available==2&&!IsWorking());
     mock::selfDanger=false;m->Disable();Tick();assert(available==3&&!IsWorking());aura.Reset();}
    assert(Above(true,64,64)&&!Above(true,63.999,64)&&Above(false,63,64));
    {auto m=Start();m->Bool("Place.Auto Base",true);m->Number("Place.Delay",150);
     mock::baseExists=false;ScanBase();assert(mock::baseUses==1);
     int auraTurns=0;
     auto aura=nova::event::EventBus::Get().Subscribe<nova::event::GameTickEvent>([&](auto&){
         if(IsWorking()){assert(mock::owner);return;}
         ++auraTurns;mock::angle={0,-20};
     },10);
     Tick();assert(IsWorking()&&auraTurns==0&&mock::baseUses==1);
     mock::baseExists=true;Tick();assert(IsWorking()&&auraTurns==0&&mock::uses==0);
     mock::rescue=true;Tick();assert(!IsWorking()&&auraTurns==1);
     mock::rescue=false;mock::stock=false;Tick();assert(!IsWorking()&&auraTurns==2);
     aura.Reset();}
    assert(Damage(0,1,2,0,0,-1,0)==85&&Damage(13,1,2,0,0,-1,0)==0&&Damage(0,1,0,0,0,-1,0)==0);
    assert(Damage(0,1,3,0,0,-1,0)==127.5&&Damage(0,1,1,0,0,-1,0)==43.5);
    assert(Damage(0,1,2,0,0,4,0)==0&&Damage(6,1,2,20,8,0,20)<Damage(6,1,2,20,8,-1,0));
    assert(!Safe(7,10,0,6,4)&&!Safe(6,9,0,6,4)&&Safe(6,9,1,6,4));
    Confirmation c;c.Start(Confirmation::Kind::Place,100);assert(c.Waiting(299)&&!c.Waiting(300));
    c.Start(Confirmation::Kind::Base,100);assert(c.Waiting(299)&&!c.Waiting(300));
    {PlacementTurnGate gate;gate.Seed({0,0});assert(gate.Ready({10,0}));
     assert(gate.Ready({1,0}));gate.Submitted();gate.Observe({1,0});
     assert(gate.Ready({11,0}));gate.Submitted();gate.Observe({11,0});
     assert(!gate.Ready({21,0}));gate.Observe({21,0});assert(!gate.Ready({21,0}));
     assert(gate.Ready({21,1}));gate.Submitted();gate.Observe({21,1});
     assert(gate.Ready({21,1})&&gate.Ready({33,1}));}
    // Circle the base at sprint-like speed without pausing, in both slot modes.
    for(bool quantized:{false,true})for(bool silent:{false,true})for(bool building:{false,true}) {
        auto m=Start();m->Bool("Place.Silent",silent);m->Bool("Place.Auto Base",building);
        mock::quantized=quantized;
        m->Number("Rotation.Speed",45);mock::baseExists=!building;mock::selfPosition={-.2,64,.5};
        mock::angle=nova::rotation::Calculate(mock::selfPosition.Add(0,1.62,0),{2.5,64,.5});
        auto previous=mock::angle;double delta=0,lastPlaced=-1;bool rotated=false;
        for(int tick=0;tick<100;++tick) {
            const auto old=mock::selfPosition;const double a=3.141592653589793+tick*.1;
            mock::selfPosition={2.5+2.7*std::cos(a),64,.5+2.7*std::sin(a)};
            mock::selfVelocity=mock::selfPosition.Subtract(old);
            const int uses=mock::uses,attacks=mock::attacks,bases=mock::baseUses;Tick();
            if(mock::angle.yaw!=previous.yaw||mock::angle.pitch!=previous.pitch){delta=std::abs(mock::angle.yaw-previous.yaw);rotated=true;}
            if(mock::uses>uses||mock::baseUses>bases){if(rotated){assert(delta<=2||lastPlaced<0||std::abs(delta-lastPlaced)>=.0001);lastPlaced=delta;rotated=false;}}
            if(mock::baseUses>bases)mock::baseExists=true;
            if(mock::uses>uses)mock::crystal=true;
            if(mock::attacks>attacks){mock::crystal=false;if(building)mock::baseExists=false;}
            previous=mock::angle;
        }
        assert(mock::uses>=10&&mock::attacks>=10);
        if(building)assert(mock::baseUses>=10);
        if(silent)assert(mock::selected==0);
    }
    {auto m=Start();Scan();assert(mock::slotWrites==1);
     for(int i=0;i<20;++i){mock::crystal=true;Tick();mock::crystal=false;Tick();}
     assert(mock::slotWrites==1&&mock::selected==4);mock::target=false;Tick();assert(mock::slotWrites==2&&mock::selected==0);}
    {auto m=Start();m->Bool("Place.Silent",true);m->Bool("Swing",false);Scan();
     assert(mock::selected==0&&mock::spoof==4&&mock::slotWrites==0&&mock::swings==0&&mock::swingPackets==1);
     for(int i=0;i<10;++i){mock::crystal=true;Tick();mock::crystal=false;Tick();}
     assert(mock::attacks==10&&mock::uses==11&&mock::swings==0&&mock::swingPackets==21&&mock::selected==0&&mock::spoof==4);
     m->Disable();Tick();assert(mock::spoof==-1&&mock::selected==0);}
    {auto m=Start();m->Bool("Place.Silent",true);m->Bool("Place.Auto Base",true);mock::baseExists=false;ScanBase();
     assert(mock::selected==0&&mock::spoof==5);mock::baseExists=true;Scan();assert(mock::selected==0&&mock::spoof==4);
     mock::target=false;Tick();assert(mock::spoof==-1&&mock::slotWrites==0);}
    {auto m=Start();m->Bool("Place.Silent",true);Scan();mock::selected=7;Tick();assert(mock::selected==7&&mock::spoof==-1);}
    {auto m=Start();Scan();m->Bool("Place.Silent",true);mock::crystal=true;Tick();assert(mock::selected==0&&mock::spoof==-1);
     mock::crystal=false;Tick();assert(mock::selected==0&&mock::spoof==4);}
    {auto m=Start();Scan();int urgent=0;nova::event::Post<nova::event::GameTickEvent>();
     assert(nova::java::AcquirePhysicalSlot(&urgent));nova::java::SetOwnedPhysicalSlot(&urgent,5);
     mock::rescue=true;nova::event::Post<nova::event::PrePlayerInteractEvent>();
     assert(mock::slotOwner==&urgent&&mock::selected==5);nova::java::SetOwnedPhysicalSlot(&urgent,4);
     nova::java::ReleaseSilentSlot(&urgent);Tick();assert(mock::selected==0&&mock::slotOwner==nullptr);}
    {auto m=Start();m->Bool("Place.Silent",true);Scan();int urgent=0;nova::event::Post<nova::event::GameTickEvent>();
     assert(nova::java::SetSilentSlot(&urgent,5));mock::rescue=true;nova::event::Post<nova::event::PrePlayerInteractEvent>();
     assert(mock::spoof==5&&mock::slotOwner==&urgent&&mock::selected==0);nova::java::ReleaseSilentSlot(&urgent);}
    // Model Grim's retained delta: unchanged motion does not clear a previous large turn.
    {auto m=Start();mock::angle={};auto previous=mock::angle;double lastLookYaw=0,lastPlaced=-1;bool rotated=false;
     for(int tick=0;tick<45;++tick) {
         const int uses=mock::uses;Tick();const auto current=mock::angle;
         if(current.yaw!=previous.yaw||current.pitch!=previous.pitch){lastLookYaw=std::abs(current.yaw-previous.yaw);rotated=true;}
         if(mock::uses>uses&&rotated){assert(lastLookYaw<=2||lastPlaced<0||std::abs(lastLookYaw-lastPlaced)>=.0001);lastPlaced=lastLookYaw;rotated=false;}
         previous=current;if(mock::uses>0)mock::crystal=!mock::crystal;
     }
     assert(mock::uses>10&&mock::attacks>10);}
    // Prediction uses current movement, discards teleports, and stops at world collisions.
    {MotionSample sample;sample.Update(1,{0,64,0},{.25,0,0},true);
     const nova::java::Box bounds{-.3,64,-.3,.3,65.8,.3};
     auto path=Forecast(sample,bounds,true,[](const auto&){return true;});
     assert(std::abs(path.At(4).x-1)<1e-6&&path.Crosses({.7,64,-.3,1.3,66,.3},4));
     auto wall=Forecast(sample,bounds,true,[](const auto& b){return b.maxX<=.81;});
     assert(wall.At(8).x<=.51&&wall.At(8).x>=.49);
     sample.Update(2,{0,64,0},{},true);assert(sample.velocity.x==0);
     sample.Update(3,{-.25,64,0},{},true);assert(sample.velocity.x<0);
     sample.Update(4,{100,64,0},{},true);assert(sample.velocity.DistanceTo({})==0);
     sample.Update(8,{100,64,0},{},true);assert(sample.velocity.DistanceTo({})==0);}
    {MotionSample sample;sample.Update(1,{0,65,0},{0,-.3,0},false);
     auto landing=Forecast(sample,{-.3,65,-.3,.3,66.8,.3},false,[](const auto& b){return b.minY>=64;});
     assert(landing.At(8).y>=64&&landing.At(8).y<64.25);}
    {MotionSample sample;sample.Update(1,{2.5,64.25,.5},{0,-.4,0},false);
     const nova::java::Box bounds{2.2,64.25,.2,2.8,66.05,.8},base{2,63,0,3,64,1};
     auto clear=[](const auto& box){return box.minY>=63;};
     const auto world=Forecast(sample,bounds,false,clear);
     const auto planned=ForecastWithBase(sample,world,base,clear);
     assert(world.At(1).y<64&&planned.At(1).y>=64&&planned.At(8).y<64.01);
     assert(!planned.Crosses(base,8));
     double exposure=0;
     assert(SampleExposure(planned.BoxAt(8),{2.5,64,.5},&base,
         [](auto,auto,bool& clear){clear=true;return true;},exposure)&&exposure==1);
     int queries=0;
     const auto unchanged=ForecastWithBase(sample,world,{8,63,0,9,64,1},[&](const auto&){++queries;return true;});
     assert(queries==0&&unchanged.At(8).DistanceTo(world.At(8))==0);}
    // Equal-distance bases must follow movement direction rather than enumeration order.
    {auto m=Start();m->Bool("Place.Auto Base",true);m->Bool("Target.Through Walls",true);
     mock::targetX=0;mock::targetZ=2;
     for(int x=-2;x<=2;++x)for(int z=0;z<=3;++z)mock::bases.push_back({{x,63,z},false,.25});
     const nova::java::BlockPos best{-2,63,-1};mock::bases.push_back({best,false,1});
     mock::angle=nova::rotation::Calculate({0,65.62,0},{-1.5,63,-.5});
     for(int tick=0;tick<100&&mock::baseUses==0;++tick){
         mock::baseQueries=mock::exposureQueries=0;Tick();
         assert(mock::baseQueries<=145&&mock::exposureQueries<=32);
         if(tick==0)assert(mock::baseUses==0);
     }
     assert(mock::baseUses==1&&mock::Same(mock::lastPlaced,best));
     const auto winner=std::find_if(mock::evaluated.begin(),mock::evaluated.end(),[&](auto p){return mock::Same(p,best);});
     assert(std::distance(mock::evaluated.begin(),winner)>=8);}
    {auto m=Start();m->Bool("Place.Auto Base",true);Tick();
     // No complete-grid requirement when a usable crystal base is already known.
     assert(mock::uses==1&&mock::baseUses==0);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;ScanBase();
     assert(mock::baseUses==1);mock::baseExists=true;
     mock::targetX=2.5;mock::targetZ=.5;mock::targetY=64;
     mock::bases={{{2,63,0},true,1},{{-3,63,0},true,1}};
     m->Bool("Target.Through Walls",true);Scan();
     assert(mock::uses==1&&mock::baseUses==1&&mock::lastPlaced.x==-3);}
    // Ready crystals are not starved by a higher-scoring construction plan.
    {auto m=Start();m->Bool("Place.Auto Base",true);m->Bool("Target.Through Walls",true);
     m->Number("Place.Auto Base.Place Delay",1000);mock::baseExists=false;
     ScanBase();assert(mock::baseUses==1);mock::baseExists=true;
     // Base cooldown does not delay the following crystal.
     Scan();assert(mock::uses==1);
     mock::crystal=true;Tick();mock::crystal=false;
     mock::targetX=2.5;mock::targetZ=.5;
     mock::bases={{{2,63,0},true,1},{{-3,63,0},false,1}};
     for(int i=0;i<30;++i)Tick();assert(mock::baseUses==1);
     std::this_thread::sleep_for(std::chrono::milliseconds(1010));
     for(int i=0;i<100&&mock::baseUses<2;++i)Tick();
     assert(mock::baseUses==2&&mock::lastPlaced.x==-3);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;ScanBase();
     mock::baseExists=true;mock::targetX=2.5;mock::targetZ=.5;
     mock::bases={{{2,63,0},true,1},{{-3,63,0},false,1}};
     m->Bool("Target.Through Walls",true);
     for(int i=0;i<3;++i)Tick();assert(mock::baseUses==1&&mock::uses==0);
     mock::targetX=3.4;Scan();assert(mock::baseUses==1&&mock::uses==1&&mock::lastPlaced.x==2);}
    for(bool silent:{false,true}) {
        auto m=Start();m->Bool("Place.Auto Base",true);m->Bool("Place.Silent",silent);
        mock::baseExists=false;ScanBase();assert(mock::baseUses==1);
        mock::baseExists=true;mock::targetX=2.5;mock::targetZ=.5;
        mock::bases={{{2,63,0},true,1},{{-3,63,0},false,1}};
        m->Bool("Target.Through Walls",true);
        // Standing inside the first crystal cell must not permanently disable
        // construction elsewhere. No manual placement or camera change.
        for(int i=0;i<100&&mock::baseUses<2;++i)Tick();
        assert(mock::baseUses==2&&mock::lastPlaced.x==-3&&mock::uses==0);
        mock::bases[1].built=true;
        Scan();assert(mock::uses==1&&mock::lastPlaced.x==-3);
    }
    // Direct interaction outranks a higher-damage occluded crystal; an occluded
    // crystal still outranks new construction. Both Auto Base states share this.
    for(bool autoBase:{false,true}) {
        auto m=Start();m->Bool("Place.Auto Base",autoBase);m->Bool("Target.Through Walls",true);
        mock::targetX=0;mock::targetZ=2;mock::angle={};m->Number("Rotation.Speed",1);
        mock::bases={{{-3,63,0},true,1,0,true},{{2,63,0},true,.5}};
        for(int i=0;i<16;++i)Tick();m->Render();
        assert(mock::uses==0&&rendered.world&&rendered.block.x==2);
    }
    {auto m=Start();m->Bool("Place.Auto Base",true);m->Bool("Target.Through Walls",true);
     mock::targetX=0;mock::targetZ=2;mock::angle={};m->Number("Rotation.Speed",1);
     mock::bases={{{-3,63,0},true,.5,0,true},{{2,63,0},false,1}};
     for(int i=0;i<16;++i)Tick();m->Render();assert(rendered.world&&rendered.block.x==-3&&mock::baseUses==0);}
    // Keep scanning after finding a usable location. A better, later candidate
    // must replace it even though the old position never becomes invalid.
    for(bool autoBase:{false,true}) {
        auto m=Start();m->Bool("Place.Auto Base",autoBase);m->Bool("Target.Through Walls",true);
        mock::angle={};m->Number("Rotation.Speed",1);mock::targetX=0;mock::targetZ=2;
        mock::bases={{{1,63,2},true,.3},{{-2,63,2},true,.3},{{2,63,0},true,1}};
        for(int i=0;i<8;++i){Tick();m->Render();if(rendered.world)break;}
        assert(rendered.world&&rendered.block.z==2);
        for(int i=0;i<16;++i)Tick();m->Render();
        assert(mock::uses==0&&rendered.world&&rendered.block.x==2&&rendered.block.z==0);
    }
    // Damage ranking still applies within each stage, with safety as a hard gate.
    {auto m=Start();m->Bool("Target.Through Walls",true);mock::angle={0,-80};m->Number("Rotation.Speed",1);
     mock::targetX=0;mock::targetZ=2;
     const auto exposure=[](double damage){
         const double impact=(std::sqrt(1+4*(damage-1)/42.)-1)*.5;
         return impact/(1-std::sqrt(8.5)/12.);
     };
     mock::bases={{{2,63,0},true,exposure(30.7)},{{-3,63,0},true,exposure(30.6)}};
     for(int i=0;i<12;++i)Tick();m->Render();assert(rendered.world&&rendered.block.x==2);
     mock::bases[1].exposure=exposure(30.9);
     for(int i=0;i<16;++i)Tick();m->Render();assert(rendered.world&&rendered.block.x==2);
     mock::bases[1].exposure=exposure(31.2);
     for(int i=0;i<16;++i)Tick();m->Render();assert(rendered.world&&rendered.block.x==-3&&mock::uses==0);}
    for(int variant=0;variant<3;++variant) {
        auto m=Start();m->Bool("Place.Auto Base",true);m->Bool("Target.Through Walls",true);
        mock::targetX=0;mock::targetZ=2;
        mock::bases={{{-3,63,0},false,1,variant==2?1.:0.},{{2,63,0},true,variant==1?1.:.995}};
        const bool build=false;
        mock::angle=nova::rotation::Calculate({0,65.62,0},{build?-2.5:2.5,build?63.:64.,.5});
        ScanBase();assert(mock::baseUses==(build?1:0)&&mock::uses==(build?0:1));
        assert(mock::lastPlaced.x==(build?-3:2));
    }
    for(double direction:{-1.,1.}) {
        auto m=Start();mock::extraBase=true;mock::targetX=0;mock::targetZ=2;
        mock::observedVelocity={direction*.3,0,0};m->Bool("Target.Through Walls",true);
        mock::angle=nova::rotation::Calculate({0,65.62,0},{direction*2.5,64,.5});
        Tick();assert(mock::uses==1&&mock::lastPlaced.x==(direction>0?2:-3));
    }
    for(double direction:{-1.,1.}) {
        auto m=Start();mock::extraBase=true;mock::baseExists=mock::extraExists=false;
        mock::targetX=0;mock::targetZ=2;mock::observedVelocity={direction*.3,0,0};
        m->Bool("Target.Through Walls",true);m->Bool("Place.Auto Base",true);
        mock::angle=nova::rotation::Calculate({0,65.62,0},{direction*2.5,63,.5});
        ScanBase();assert(mock::baseUses==1&&mock::lastPlaced.x==(direction>0?2:-3)&&mock::uses==0);
        mock::observedVelocity={};mock::baseExists=mock::extraExists=true;
        // The opposite base may become usable meanwhile; finish the committed one.
        Scan();assert(mock::uses==1&&mock::lastPlaced.x==(direction>0?2:-3));
    }
    {auto m=Start();mock::targetZ=.5;mock::observedVelocity={-.3,0,0};Tick();assert(mock::uses==0);
     mock::observedVelocity={};Tick();assert(mock::uses==1);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;mock::targetZ=.5;mock::observedVelocity={-.3,0,0};
     ScanBase();assert(mock::baseUses==1&&mock::uses==0);}
    // Airborne targets allow adjacent construction, never a crystal cell already
    // occupied by their body. Height gating remains specific to actual crystals.
    for(bool walls:{false,true}) {auto m=Start();m->Bool("Place.Auto Base",true);m->Bool("Target.Through Walls",walls);
     m->Number("Rotation.Speed",45);
     // Flat floor y=63, new obsidian y=64, future crystal y=65.
     // Ordinary knockback lifts feet less than one block: only the adjacent
     // base is legal, and Only Above Crystal must not prevent its construction.
     mock::targetX=2.5;mock::targetZ=1.5;mock::targetY=64;
     mock::bases={{{2,64,0},false,1}};
     for(int i=0;i<3;++i)Tick();
     mock::targetGrounded=false;mock::observedVelocity={0,.4,0};
     mock::angle=nova::rotation::Calculate({0,65.62,0},{2.5,65.8,1.5});
     for(int i=0;i<9&&mock::baseUses==0;++i){
         mock::targetY+=mock::observedVelocity.y;
         mock::observedVelocity.y=(mock::observedVelocity.y-.08)*.98;Tick();
     }
     assert(mock::baseUses==1&&mock::lastPlaced.y==64&&mock::uses==0);
     mock::bases[0].built=true;mock::targetY=64;mock::targetGrounded=true;mock::observedVelocity={};
     for(int i=0;i<5;++i)Tick();assert(mock::uses==0); // Actual crystal height gate remains.
    }
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;mock::fallingWorld=true;
     mock::targetX=2.5;mock::targetZ=.5;mock::targetY=64.7;mock::targetGrounded=false;
     mock::observedVelocity={0,-.2,0};
     mock::angle=nova::rotation::Calculate({0,65.62,0},{2.5,63,.5});
     for(int tick=0;tick<3&&mock::baseUses==0;++tick){
         Tick();
         if(mock::baseUses==0){mock::targetY+=mock::observedVelocity.y;mock::observedVelocity.y=(mock::observedVelocity.y-.08)*.98;}
     }
     assert(mock::baseUses==0&&mock::uses==0);}
    {auto m=Start();m->Bool("Place.Auto Base",true);m->Bool("Target.Through Walls",true);
     mock::targetX=2.5;mock::targetZ=.5;mock::targetY=64.1;mock::targetGrounded=false;
     mock::observedVelocity={0,.42,0};
     for(int x=-3;x<=3;++x)for(int z=-3;z<=3;++z)mock::bases.push_back({{x,63,z},false,1});
     mock::angle=nova::rotation::Calculate({0,65.62,0},{2.5,63,.5});
     for(int tick=0;tick<8&&mock::baseUses==0;++tick){
         Tick();mock::targetY+=mock::observedVelocity.y;mock::observedVelocity.y=(mock::observedVelocity.y-.08)*.98;
     }
     assert(mock::baseUses==1&&mock::uses==0);}
    for(bool silent:{false,true}) {
        auto m=Start();m->Bool("Place.Auto Base",true);m->Bool("Place.Silent",silent);
        m->Number("Safety.Min Target Damage",20);
        mock::baseExists=false;mock::fallingWorld=true;
        mock::targetX=2.5;mock::targetZ=.5;mock::targetY=66.4;mock::targetGrounded=false;
        mock::observedVelocity={0,-.08,0};
        mock::angle=nova::rotation::Calculate({0,65.62,0},{2.5,63,.5});
        // Advance actual positions every tick and sample real virtual-block
        // exposure. A frozen target with constant fake exposure hides this bug.
        for(int tick=0;tick<9&&mock::baseUses==0;++tick) {
            Tick();
            if(mock::baseUses==0){mock::targetY+=mock::observedVelocity.y;mock::observedVelocity.y=(mock::observedVelocity.y-.08)*.98;}
        }
        assert(mock::baseUses==1&&mock::targetY>=64&&mock::uses==0);
        mock::baseExists=true;mock::targetY=64;mock::targetGrounded=true;mock::observedVelocity={};
        for(int tick=0;tick<3;++tick)Tick();assert(mock::uses==0); // Still standing in crystal volume.
        mock::targetX=3.4;Scan();assert(mock::uses==1&&mock::baseUses==1);
    }
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;
     mock::targetY=65;mock::targetGrounded=false;mock::attackReach=1;
     ScanBase();assert(mock::baseUses==1&&mock::uses==0);
     mock::baseExists=true;for(int i=0;i<5;++i)Tick();assert(mock::uses==0);
     mock::attackReach=3;Scan();assert(mock::uses==1);}
    for(double x:{2.5,3.4}) {
        auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;
        mock::targetX=x;mock::targetZ=.5;mock::targetY=64.25;
        mock::targetGrounded=false;mock::observedVelocity={0,-.4,0};
        assert(nova::java::CrystalBaseBuildable(mock::base));
        ScanBase();assert(mock::baseUses==(x==2.5?0:1)&&mock::uses==0);
        if(x==2.5){mock::targetX=3.4;ScanBase();assert(mock::baseUses==1);}
        mock::baseExists=true;
        mock::targetX=3.4;mock::targetY=64;mock::targetGrounded=true;mock::observedVelocity={};
        Scan();assert(mock::uses==1&&mock::baseUses==1);
    }
    // A local player in crystal space rejects construction too; moving out
    // re-enables the same base without manual placement.
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;
     mock::selfPosition={2.5,64.25,.5};mock::selfVelocity={0,-.4,0};mock::selfGrounded=false;
     ScanBase();assert(mock::baseUses==0&&mock::uses==0);
     mock::selfPosition={0,64,0};mock::selfVelocity={};mock::selfGrounded=true;
     ScanBase();assert(mock::baseUses==1);mock::baseExists=true;Scan();assert(mock::uses==1);}
    {auto m=Start();m->Bool("Place.Auto Base",true);m->Bool("Safety.Only Above Crystal",false);
     mock::baseExists=false;mock::targetX=2.5;mock::targetZ=.5;mock::targetY=63.9;mock::targetGrounded=false;
     assert(!nova::java::CrystalBaseBuildable(mock::base));ScanBase();assert(mock::baseUses==0);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=mock::supportExists=false;
     mock::targetX=2.5;mock::targetZ=.5;mock::targetY=64.25;mock::targetGrounded=false;
     ScanBase();assert(mock::baseUses==0);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;ScanBase();assert(mock::baseUses==1);
     mock::baseExists=true;mock::targetX=2.5;mock::targetZ=.5;Tick();assert(mock::uses==0&&mock::baseUses==1);
     mock::targetX=3.4;mock::targetZ=2.5;Scan();assert(mock::uses==1&&mock::baseUses==1);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;ScanBase();assert(mock::baseUses==1);
     mock::targetX=20;mock::baseExists=true;Tick();assert(mock::uses==0);m->Render();assert(!rendered.world);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;ScanBase();assert(mock::baseUses==1);
     std::this_thread::sleep_for(std::chrono::milliseconds(210));Tick();assert(mock::baseUses==1&&mock::uses==0&&mock::selected==0);}
    {auto m=Start();mock::baseExists=false;ScanBase();assert(mock::baseUses==0&&mock::uses==0);}
    {auto m=Start();m->Bool("Place.Auto Base",true);ScanBase();assert(mock::uses==1&&mock::baseUses==0);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;ScanBase();assert(mock::baseUses==1&&mock::selected==5&&mock::uses==0);
     Tick();assert(mock::selected==5&&mock::baseUses==1&&mock::uses==0);m->Render();assert(rendered.world);
     mock::baseExists=true;Scan();assert(mock::uses==1&&mock::selected==4);mock::crystal=true;Tick();assert(mock::attacks==1&&mock::selected==4);
     mock::crystal=false;Tick();assert(mock::uses==2&&mock::baseUses==1);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=mock::obsidian=false;ScanBase();assert(mock::baseUses==0);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=mock::stock=false;ScanBase();assert(mock::baseUses==0);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=mock::baseSpace=false;ScanBase();assert(mock::baseUses==0);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=mock::supportExists=false;ScanBase();assert(mock::baseUses==0);}
    {auto m=Start();m->Bool("Place.Auto Base",true);m->Bool("Place.Auto Switch",false);mock::baseExists=false;ScanBase();assert(mock::baseUses==0);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;mock::wall=true;ScanBase();assert(mock::baseUses==0);
     m->Bool("Target.Through Walls",true);ScanBase();assert(mock::baseUses==1);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;mock::selfDanger=true;ScanBase();assert(mock::baseUses==0);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;ScanBase();m->Disable();Tick();assert(mock::selected==0&&mock::slotOwner==nullptr);}
    {auto m=Start();m->Bool("Place.Auto Base",true);mock::baseExists=false;ScanBase();mock::selected=7;Tick();assert(mock::selected==7);}
    // Six entry faces, exact reach, miss/behind, parallel and invalid rays.
    {const nova::java::Box box{0,0,0,1,1,1};nova::rotation::Vec3 hit;int face=-1;
     const nova::rotation::Vec3 origins[]={{.5,-1,.5},{.5,2,.5},{.5,.5,-1},{.5,.5,2},{-1,.5,.5},{2,.5,.5}};
     const nova::rotation::Vec3 directions[]={{0,1,0},{0,-1,0},{0,0,1},{0,0,-1},{1,0,0},{-1,0,0}};
     for(int i=0;i<6;++i){assert(RayBox(origins[i],directions[i],box,1,hit,face)&&face==i);assert(!RayBox(origins[i],directions[i],box,.99,hit,face));}
     assert(!RayBox({2,2,2},{1,0,0},box,5,hit,face));
     assert(!RayBox({2,.5,.5},{1,0,0},box,5,hit,face));
     assert(!RayBox({2,.5,.5},{0,0,0},box,5,hit,face));}
    {auto m=Start();Scan();assert(mock::uses==1&&mock::selected==4);Tick();assert(mock::selected==4&&mock::uses==1&&mock::attacks==0);
     mock::crystal=true;Tick();assert(mock::attacks==1);Tick();assert(mock::attacks==1);mock::crystal=false;Tick();assert(mock::uses==2);
     for(int i=0;i<20;++i){mock::crystal=true;Tick();assert(mock::attacks==i+2);mock::crystal=false;Tick();assert(mock::uses==i+3);}}
    {auto m=Start();mock::targetY=63;Scan();assert(mock::uses==0&&mock::attacks==0);}
    {auto m=Start();mock::crystal=true;mock::targetY=63;Scan();assert(mock::attacks==0);m->Bool("Safety.Only Above Crystal",false);Tick();assert(mock::attacks==1);}
    {auto m=Start();mock::dropOnAim=true;Scan();assert(mock::uses==0);}
    {auto m=Start();mock::crystal=true;mock::dropOnAim=true;Tick();assert(mock::attacks==0);}
    {auto m=Start();mock::team=true;Scan();assert(mock::uses==0&&mock::attacks==0);}
    {auto m=Start();mock::bot=true;Scan();assert(mock::uses==0);}
    {auto m=Start();mock::selfDanger=true;Scan();assert(mock::uses==0);}
    {auto m=Start();mock::teammate=true;Scan();assert(mock::uses==0);}
    {auto m=Start();mock::defenseOK=false;Scan();assert(mock::uses==0);}
    {auto m=Start();mock::wall=true;Scan();assert(mock::uses==0);}
    {auto m=Start();mock::wall=mock::crystal=true;Scan();assert(mock::attacks==0);}
    {auto m=Start();mock::wall=true;m->Bool("Target.Through Walls",true);Scan();assert(mock::uses==1);
     mock::crystal=true;Tick();assert(mock::attacks==1);mock::crystal=false;m->Bool("Target.Through Walls",false);Tick();assert(mock::uses==1);}
    {auto m=Start();mock::angle={};mock::wall=true;m->Bool("Target.Through Walls",true);m->Number("Rotation.Speed",1);Scan();assert(mock::uses==0);}
    {auto m=Start();mock::angle={};mock::wall=mock::crystal=true;m->Bool("Target.Through Walls",true);m->Number("Rotation.Speed",1);Tick();assert(mock::attacks==0);}
    {auto m=Start();mock::wall=true;m->Bool("Target.Through Walls",true);m->Number("Target.Range",1);Scan();assert(mock::uses==0&&mock::attacks==0);}
    {auto m=Start();mock::wall=mock::selfDanger=true;m->Bool("Target.Through Walls",true);Scan();assert(mock::uses==0);}
    {auto m=Start();mock::selfDanger=true;m->Bool("Safety.Ignore Self Damage",true);Scan();assert(mock::uses==1);
     mock::crystal=true;Tick();assert(mock::attacks==1);mock::crystal=false;m->Bool("Safety.Ignore Self Damage",false);Tick();assert(mock::uses==1);}
    {auto m=Start();mock::wall=mock::selfDanger=true;m->Bool("Target.Through Walls",true);m->Bool("Safety.Ignore Self Damage",true);
     Scan();assert(mock::uses==1);mock::crystal=true;Tick();assert(mock::attacks==1);}
    {auto m=Start();mock::selfDanger=mock::teammate=true;m->Bool("Safety.Ignore Self Damage",true);Scan();assert(mock::uses==0);
     mock::crystal=true;Scan();assert(mock::attacks==0);}
    {auto m=Start();mock::selfDanger=true;mock::targetY=63;m->Bool("Safety.Ignore Self Damage",true);Scan();assert(mock::uses==0);}
    {auto m=Start();m->Bool("Safety.Ignore Self Damage",true);m->Number("Safety.Min Target Damage",100);Scan();assert(mock::uses==0);}
    {auto m=Start();mock::stock=false;Scan();assert(mock::uses==0);}
    {auto m=Start();m->Bool("Place.Auto Switch",false);Scan();assert(mock::uses==0);}
    {auto m=Start();m->Number("Target.FOV",1);Scan();assert(mock::uses==0);}
    {auto m=Start();mock::rescue=true;Scan();assert(mock::uses==0&&mock::owner==nullptr);}
    {auto m=Start();Scan();mock::selected=7;Tick();assert(mock::selected==7&&mock::slotOwner==nullptr);}
    {auto m=Start();Scan();mock::world=9;Tick();assert(mock::selected==4&&mock::slotOwner==nullptr);}
    {auto m=Start();Scan();m->Disable();Tick();assert(mock::selected==0&&mock::slotOwner==nullptr&&mock::owner==nullptr);}
    {auto m=Start();Scan();mock::syncOK=false;m->Disable();Tick();assert(mock::selected==0&&mock::slotOwner!=nullptr);mock::syncOK=true;Tick();assert(mock::slotOwner==nullptr);}
    {auto m=Start();Scan();mock::rescue=true;Tick();assert(mock::selected==0&&mock::slotOwner==nullptr&&mock::owner==nullptr);}
    {auto m=Start();mock::crystal=true;Tick();assert(mock::attacks==1);nova::event::Post<nova::event::PrePlayerInteractEvent>();assert(mock::attacks==1&&mock::uses==0);}
    {auto m=Start();Scan();m->Render();assert(rendered.world&&rendered.block.x==2&&rendered.block.y==63&&rendered.block.z==0);
     Tick();m->Render();assert(rendered.world);mock::target=false;Tick();m->Render();assert(!rendered.world);}
    {auto m=Start();Scan();mock::rescue=true;Tick();m->Render();assert(!rendered.world);}
    {auto m=Start();Scan();mock::crystal=true;Tick();mock::crystal=false;mock::stock=false;Tick();m->Render();assert(!rendered.world&&mock::uses==1);}
    {auto m=Start();Scan();mock::crystal=true;Tick();mock::crystal=false;mock::selfDanger=true;Tick();assert(mock::uses==1);m->Render();assert(!rendered.world);}
    {auto m=Start();Scan();mock::crystal=true;Tick();mock::crystal=false;mock::wall=true;Tick();assert(mock::uses==1);m->Render();assert(!rendered.world);}
    {auto m=Start();Scan();m->Bool("Place ESP",false);rendered={};m->Render();assert(!rendered.world);}
    std::puts("PASS: predictive base/crystal selection, motion/collisions, staged placement, live safety, target loss, retry cooldown, slot restore and continuous cycles");
}
