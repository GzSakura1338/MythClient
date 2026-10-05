#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cassert>
#include <cmath>
#include <format>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "minecraft/utils/rotation/Vec3.h"
#include "minecraft/utils/rotation/Rotation.h"
#include "minecraft/utils/rotation/MathConst.h"
#include "minecraft/utils/rotation/MovementFix.h"
#include "minecraft/utils/rotation/RotationUtils.h"
#include "minecraft/utils/rotation/LinearAim.h"
#include "minecraft/utils/rotation/Mth.h"
#include "minecraft/modules/combat/throwableaura/Trajectory.h"
#include "minecraft/modules/combat/throwableaura/Prediction.h"
namespace Properties {
struct BoolProperty { bool value; bool get() const{return value;} };
struct NumberProp { double value; std::function<std::string(double)> formatter; double get() const{return value;} };
}
namespace nova {
namespace clickgui { bool open=false;bool IsOpen(){return open;} }
namespace event { namespace Priority {constexpr int Lowest=20;} struct ClientRotationEvent{};struct PrePlayerInteractEvent{}; struct Subscription{}; struct EventBus {
static EventBus& Get(){static EventBus b;return b;}
template<class E,class F> Subscription Subscribe(F,int=0){return {};}
}; }
namespace module {
namespace automlg {bool IsWorking(){return false;}}
namespace autorod { bool owns=false; bool OwnsItemUse(){return owns;} }
namespace autoblock { bool owns=false; bool OwnsItemUse(){return owns;} }
namespace scaffold {bool enabled=false;bool IsEnabled(){return enabled;}}
namespace blockin {bool placing=false;bool IsPlacing(){return placing;}}
namespace breaker {bool mining=false;bool OwnsMining(){return mining;}}
enum class Category {Combat};
struct Module {
 bool enabled=true;
 std::map<std::string,std::unique_ptr<Properties::BoolProperty>> bs;
 std::map<std::string,std::unique_ptr<Properties::NumberProp>> ns;
 Module(std::string,Category,std::string,std::string){}
 virtual ~Module()=default;
 virtual void onEnable(){} virtual void onDisable(){}
 virtual void onRender(){} void enableOverlay(){}
 bool isEnabled() const{return enabled;}
 auto& addBool(std::string n,bool v){auto p=std::make_unique<Properties::BoolProperty>();p->value=v;auto& r=*p;bs[n]=std::move(p);return r;}
 auto& addNumber(std::string n,double,double,double,double v,std::vector<int> = {},std::function<std::string(double)> formatter = {}){auto p=std::make_unique<Properties::NumberProp>();p->value=v;p->formatter=std::move(formatter);auto& r=*p;ns[n]=std::move(p);return r;}
};
Module blocker("AimBacktrack",Category::Combat,"","");
Module rod("AutoRod",Category::Combat,"","");
struct ModuleManager {static ModuleManager& Get(){static ModuleManager m;return m;} Module* Find(std::string name){if(name=="AutoRod")return &rod;return name=="AimBacktrack" && blocker.enabled?&blocker:nullptr;}};
namespace killaura {int id=0;bool active=false;bool IsEnabled(){return active;}bool HasTargets(){return id!=0;}}
}
namespace java {
bool FreeCamActive(){return false;}
using rotation::Vec3;
enum KnownItem {Nothing,Snowball,Egg};
struct ItemFacts {bool empty=true;KnownItem item=Nothing;};
unsigned kTypeLiving=1,kTypePlayer=2,kTypeMonster=4;
struct Entity {int id=1;Vec3 pos{0,0,4},motion{};double eye=1.62;unsigned type=3;bool alive=true,visible=true,team=false,bot=false,mob=false;};
struct Box {double minX,minY,minZ,maxX,maxY,maxZ;};
int gameTick=0;
enum class HitKind {Miss,Block};
struct BlockHitResultInfo {HitKind kind=HitKind::Miss;Vec3 location;};
struct EntityHandle {
 Entity* p=nullptr;
 explicit operator bool()const{return p;} bool Valid()const{return p;}
 void Reset(){p=nullptr;}void* raw()const{return p;}
 bool IsSame(const EntityHandle& b)const{return p==b.p;}
 bool IsAlive()const{return p->alive;}
 bool Position(Vec3& out)const{out=p->pos;return true;}
 bool PositionPair(Vec3& now,Vec3& before)const{now=p->pos;before=p->pos.Subtract(p->motion);return true;}
 bool TickCount(int& tick)const{tick=gameTick;return true;}
 bool OnGround(bool& ground)const{ground=std::abs(p->motion.y)<1e-9;return true;}
 bool EyeHeight(double& out)const{out=p->eye;return true;}
 bool BoundingBox(Box& b)const{b={p->pos.x-.3,p->pos.y,p->pos.z-.3,p->pos.x+.3,p->pos.y+1.8,p->pos.z+.3};return true;}
 bool EyePosition(float,Vec3& out)const{out=p->pos.Add(0,p->eye,0);return true;}
 bool DistanceTo(const EntityHandle& b,double& out)const{out=static_cast<float>(p->pos.DistanceTo(b.p->pos));return true;}
 bool DistanceSqTo(const EntityHandle& b,double& out)const{out=p->pos.DistanceToSqr(b.p->pos);return true;}
};
Entity self{0,{0,0,0}}, enemy;
std::vector<EntityHandle> world;
Entity level{100},otherLevel{101};bool worldAvailable=true;
Entity* currentWorld=&level;
EntityHandle PlayerEntity(){return worldAvailable?EntityHandle{&self}:EntityHandle{};}EntityHandle LevelEntity(){return worldAvailable?EntityHandle{currentWorld}:EntityHandle{};}
EntityHandle EntityById(int id){for(auto e:world)if(e.p->id==id)return e;return {};}
bool EntityTypes(EntityHandle e,unsigned& out){out=e.p->type;return true;}
bool EntityId(EntityHandle e,int& id){id=e.p->id;return true;}
KnownItem hotbar[9]{};int selected=0;float yaw=0,pitch=0;
bool failRotationSend=false,failLocalRotation=false;
bool usingItem=false;bool PlayerUsingItem(){return usingItem;}
bool screen=false;bool ScreenOpen(){return screen;}
float serverYaw=0,serverPitch=0;
std::string throwOrder;
bool PlayerOnGround(){return true;}
bool SendMoveRotation(float y,float p,bool){throwOrder+='R';if(failRotationSend)return false;serverYaw=y;serverPitch=p;return true;}
bool SelectedSlot(int& v){v=selected;return true;}
bool SetSelectedSlot(int v){selected=v;return true;}
int syncs=0;bool syncOk=true;
bool SyncSelectedItem(){++syncs;return syncOk;}
bool PlayerRotation(float& y,float& p){y=yaw;p=pitch;return true;}
bool SetPlayerRotation(float y,float p){if(failLocalRotation)return false;yaw=y;pitch=p;return true;}
bool SetPlayerCameraRotation(float y,float p,double){return SetPlayerRotation(y,p);}
bool PlayerPreviousRotation(float& y,float& p){y=yaw;p=pitch;return true;}
bool PlayerSensitivity(double& value){value=.5;return true;}
int PlayerTickCount(){return 100;}
int ClientFps(){return 60;}
void PickCursor(float){}
bool InventorySlot(int i,void*& out){out=&hotbar[i];return true;}
bool ReadItemFacts(void* obj,ItemFacts& out){out.item=*static_cast<KnownItem*>(obj);out.empty=out.item==Nothing;return true;}
EntityHandle hovered;
bool AttackTargetEntity(EntityHandle& out){out=hovered;return bool(out);}
}
namespace rotation {
double Random(){return .5;}
}
namespace module::antibot {bool IsBot(java::EntityHandle e){return e.p->bot;}}
namespace module::teams {
bool enabled=false;
bool IsTeammate(const java::EntityHandle&,const java::EntityHandle& other){return enabled && other.p->team;}
}
namespace compat {
bool lunarProfile=true;bool IsLegacyLunar(){return lunarProfile;}
namespace lunar {
int uses=0;int usedSlot=-1,auraDuringUse=-1;float usedYaw=0,usedPitch=0;
bool failUse=false;
bool blockLag=false,hypixelLag=false,slinkyLag=false;
std::uint64_t movement=0;
bool slotReady=true,attackClick=false,left=false,right=false,reportedOk=true;
std::uint64_t BlockInputMovementSequence(){return movement;}
bool HeldSlotChangeReady(){return slotReady;}
bool PendingAttackClick(){return attackClick;}
bool PhysicalMouseDown(int){return left;}
bool PhysicalUseDown(){return right;}
bool PlayerReportedRotation(float& y,float& p){y=java::serverYaw;p=java::serverPitch;return reportedOk;}
bool worldTraceOk=true,wall=false;
bool WorldClip(rotation::Vec3 from,rotation::Vec3 to,const java::EntityHandle&,java::BlockHitResultInfo& hit){
 hit.kind=wall?java::HitKind::Block:java::HitKind::Miss;
 hit.location=from.Add((to.x-from.x)*.2,(to.y-from.y)*.2,(to.z-from.z)*.2);return worldTraceOk;
}
bool BlockLagging(){return blockLag;}bool HypixelBlockLagging(){return hypixelLag;}bool SlinkyBlockLagging(){return slinkyLag;}
bool WorldLoadedEntities(std::vector<java::EntityHandle>& out){out=java::world;return true;}
bool EntityMotion(void* p,rotation::Vec3& out){out=static_cast<java::Entity*>(p)->motion;return true;}
bool ThrowableCanSee(void* p){return static_cast<java::Entity*>(p)->visible;}
bool ThrowableMob(void* p){return static_cast<java::Entity*>(p)->mob;}
bool ThrowableLook(rotation::Vec3& out){const double y=java::yaw*rotation::kToRadians,p=java::pitch*rotation::kToRadians;out={-sin(y)*cos(p),-sin(p),cos(y)*cos(p)};return true;}
bool ThrowHeldItem(){assert(slotReady);slotReady=false;java::throwOrder+='U';++uses;usedSlot=java::selected;usedYaw=java::serverYaw;usedPitch=java::serverPitch;auraDuringUse=module::killaura::id;return !failUse;}
}}
}
using jobject=void*;
struct Env {void DeleteLocalRef(void*){}};
struct JniAttach {Env obj;Env* env=&obj;explicit operator bool()const{return true;}};
namespace nova::module::throwableaura {
struct Visuals {
 bool active=false,trajectory=false;int id=-1,updates=0;float yaw=0,pitch=0;
 void Clear(){active=false;trajectory=false;id=-1;}
 void Update(const java::EntityHandle&,const java::EntityHandle& target,float y,float p,bool path){
  active=true;id=target.p->id;yaw=y;pitch=p;trajectory=path;++updates;
 }
 void Render(bool,bool){}
};
}
