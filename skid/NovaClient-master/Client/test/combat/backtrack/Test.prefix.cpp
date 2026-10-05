#include <algorithm>
#include <array>
#include <chrono>
#include <deque>
#include <format>
#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <map>
#include <string>
#include <functional>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include "minecraft/events/EventBus.h"
#include "minecraft/utils/rotation/Vec3.h"
#include "minecraft/modules/combat/backtrack/EspClip.h"
using jobject=void*;
namespace mock {
long long now=10000;
struct Clock {static auto now(){return std::chrono::steady_clock::time_point(std::chrono::milliseconds(mock::now));}};
void Check(bool b,const char* message){if(!b){std::fprintf(stderr,"%s\n",message);std::exit(1);}}
int refs=0,wanted=0,ticks=50,world=1;
bool player=true,owns=false,blocks=false,failRef=false,listed=true,sameTeam=false;
std::vector<int> played;
std::string wire;
}
struct MockEnv {jobject NewGlobalRef(jobject p){if(mock::failRef)return nullptr;++mock::refs;return p;}void DeleteGlobalRef(jobject){--mock::refs;}bool ExceptionCheck(){return false;}void ExceptionClear(){}};
struct JniAttach {MockEnv store;MockEnv* env=&store;explicit operator bool()const{return true;}};
namespace Properties {
struct AbstractProperty {virtual ~AbstractProperty()=default;};
struct NumberProp:AbstractProperty {double value;NumberProp(double v):value(v){}double get()const{return value;}};
struct BoolProperty:AbstractProperty {bool value;BoolProperty(bool v):value(v){}bool get()const{return value;}};
struct ModeProperty:AbstractProperty {int value;ModeProperty(int v):value(v){}int get()const{return value;}};
struct StringProperty:AbstractProperty {static inline StringProperty* instance;StringProperty(){instance=this;}std::string value;std::string get()const{return value;}};
struct ColorProperty:AbstractProperty {unsigned color;ColorProperty(unsigned c,bool):color(c){}unsigned valueAbgr()const{return color;}};
struct PropDesc {const char* name;AbstractProperty* prop;PropDesc(const char* n,AbstractProperty* p,std::vector<PropDesc> = {},std::function<bool()> = {}):name(n),prop(p){}const char* getName()const{return name;}AbstractProperty* getProp()const{return prop;}};
}
namespace nova::module {
enum class Category{Combat};
class Module {
bool enabled=false;
public:
std::map<std::string,std::unique_ptr<Properties::AbstractProperty>> props;
Module(const char*,Category,const char*,const char*){}virtual ~Module()=default;
virtual std::string suffix()const{return {};}
bool isEnabled()const{return enabled;}
void setEnabled(bool v){if(enabled==v)return;enabled=v;if(v)onEnable();else onDisable();}
void n(const char* k,double v){dynamic_cast<Properties::NumberProp&>(*props.at(k)).value=v;}
void b(const char* k,bool v){dynamic_cast<Properties::BoolProperty&>(*props.at(k)).value=v;}
void team(int v){dynamic_cast<Properties::ModeProperty&>(*props.at("Teams")).value=v;}
void mode(int v){dynamic_cast<Properties::ModeProperty&>(*props.at("Target Mode")).value=v;}
protected:
virtual void onEnable(){}virtual void onDisable(){}virtual void onRender(){}void enableOverlay(){}
Properties::NumberProp& addNumber(const char* n,double,double,double,double v,std::vector<Properties::PropDesc> = {},std::function<std::string(double)> = {}){auto p=std::make_unique<Properties::NumberProp>(v);auto& r=*p;props[n]=std::move(p);return r;}
Properties::PropDesc subNumber(const char* n,double a,double b,double c,double v,std::function<std::string(double)> format = {}){return {n,&addNumber(n,a,b,c,v,{},std::move(format))};}
Properties::BoolProperty& addBool(const char* n,bool v,std::vector<Properties::PropDesc> = {}){auto p=std::make_unique<Properties::BoolProperty>(v);auto& r=*p;props[n]=std::move(p);return r;}
Properties::PropDesc subBool(const char* n,bool v){return {n,&addBool(n,v)};}
Properties::PropDesc subMode(const char* n,int v,std::vector<std::string> names){return {n,&addMode(n,v,std::move(names),{})};}
Properties::ModeProperty& addMode(const char* n,int v,std::vector<std::string>,std::vector<Properties::PropDesc>){auto p=std::make_unique<Properties::ModeProperty>(v);auto& r=*p;props[n]=std::move(p);return r;}
};
}
namespace nova::java {
using rotation::Vec3;
struct Box{double minX,minY,minZ,maxX,maxY,maxZ;};
enum class BacktrackKind{Player,Mob,Animal,Villager,Ambient,Water,Other,ArmorStand};
struct Entity {int id=0;Vec3 pos{3,0,0},base{3,0,0};bool alive=true,invisible=false;int hurt=0;BacktrackKind kind=BacktrackKind::Player;};
Entity self{1,{0,0,0}},first{2},second{3,{4,0,0},{4,0,0}},worlds[2]{{100},{101}};
struct EntityHandle {
Entity* p=nullptr;explicit operator bool()const{return p;}
bool IsSame(const EntityHandle& o)const{return p==o.p;}void Reset(){p=nullptr;}void* raw()const{return p;}
bool IsAlive()const{return p->alive;}bool IsPlayer()const{return p->kind==BacktrackKind::Player;}bool ScoreboardName(std::string& name)const{name=std::to_string(p->id);return true;}bool DeathTime(int& out)const{out=0;return true;}
bool HurtTime(int& out)const{out=p->hurt;return true;}
bool IsInvisible(bool& out)const{out=p->invisible;return true;}
bool Position(Vec3& out)const{out=p->pos;return true;}
bool EyePosition(float,Vec3& out)const{out=p->pos.Add(0,1.6,0);return true;}
bool BoundingBox(Box& out)const{out={p->pos.x-.3,p->pos.y,p->pos.z-.3,p->pos.x+.3,p->pos.y+1.8,p->pos.z+.3};return true;}
};
EntityHandle PlayerEntity(){return {mock::player?&self:nullptr};}
EntityHandle LevelEntity(){return {mock::world?&worlds[mock::world-1]:nullptr};}
EntityHandle AdoptEntity(void* p){return {static_cast<Entity*>(p)};}
EntityHandle EntityById(int id){return {id==2?&first:id==3?&second:&self};}
constexpr unsigned kTypeLiving=1;
bool EntityTypes(const EntityHandle&,unsigned& types){types=kTypeLiving;return true;}
bool EntityId(const EntityHandle& e,int& id){id=e.p->id;return true;}
int PlayerTickCount(){return mock::ticks;}
bool WorldEntities(const EntityHandle&,const Box&,std::vector<EntityHandle>& out){out={{&first},{&second}};return true;}
bool BacktrackExcluded(const EntityHandle&,bool bots,int teams){return (bots&&!mock::listed)||(teams&&mock::sameTeam);}
bool BacktrackBase(const EntityHandle& e,Vec3& out){out=e.p->base;return true;}
bool BacktrackEntityKind(const EntityHandle& e,BacktrackKind& kind){kind=e.p->kind;return true;}
struct BacktrackPacket {bool flush=false,resetWorld=false,deadHealth=false,movedTarget=false;Vec3 position;};
struct Packet {int serial;bool pass=false,dead=false,move=false,disconnect=false,respawn=false,correction=false;Vec3 pos;int target=2;bool velocity=false,explosion=false,decodeFail=false;};
bool ReadBacktrackPacket(jobject raw,int id,int selfId,const Vec3& base,BacktrackPacket& out){
 auto p=static_cast<Packet*>(raw);
 out.flush=p->correction||p->explosion||(p->velocity&&p->target==selfId);
 out.resetWorld=p->disconnect||p->respawn;out.deadHealth=p->dead;
 out.movedTarget=p->move&&p->target==id;out.position=out.movedTarget?p->pos:base;
 return !p->decodeFail;
}
bool IsDisconnectPacket(jobject p){return static_cast<Packet*>(p)->disconnect;}
bool IsRespawnPacket(jobject p){return static_cast<Packet*>(p)->respawn;}
bool IsPlayerPositionPacket(jobject p){return static_cast<Packet*>(p)->correction;}
void SetIncomingPacketsWanted(bool v){mock::wanted+=v?1:-1;}
bool HandleIncomingPacket(jobject raw){auto p=static_cast<Packet*>(raw);mock::played.push_back(p->serial);if(p->serial==3)mock::wire+='T';if(p->move){EntityById(p->target).p->pos=p->pos;EntityById(p->target).p->base=p->pos;}return true;}
struct CameraView{};float FrameTime(){return 1;}bool ReadCameraView(float,CameraView&){return false;}
}
namespace nova::module::antikb {bool OwnsIncomingDelayQueue(){return mock::owns;}bool BlocksBacktrack(){return mock::owns||mock::blocks;}}
namespace nova::module::velocity {bool IsDelaying(){return false;}int AttackQueue(){return 0;}}
struct ImVec2{float x=0,y=0;};struct ImDrawList{void AddConvexPolyFilled(ImVec2*,int,unsigned){}void AddLine(ImVec2,ImVec2,unsigned,float){}};
namespace ImGui {void* GetCurrentContext(){return nullptr;}struct IO{ImVec2 DisplaySize;};IO& GetIO(){static IO v;return v;}ImDrawList* GetBackgroundDrawList(){static ImDrawList v;return &v;}}
namespace nova::minecraft::projection {struct ScreenPoint{float x,y;};struct CameraBasis{};CameraBasis MakeBasis(java::CameraView,float){return {};}bool Project(CameraBasis,rotation::Vec3,ImVec2,ScreenPoint&){return false;}}
namespace nova::ui {
struct ScreenGeometry {std::vector<std::vector<ImVec2>> faces;std::vector<std::array<ImVec2,2>> edges;};
void DrawGeometry(const ScreenGeometry&,unsigned,unsigned,float){}
}
