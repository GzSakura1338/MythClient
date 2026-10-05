#include <cassert>
#include <cstdio>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <string>
#include <utility>
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_crystal.h"
#include "minecraft/modules/misc/fakeplayer/Damage.h"
using jobject=void*;
struct JNIEnv {
    bool IsSameObject(jobject a,jobject b){return a==b;}
    jobject NewStringUTF(const char*){return reinterpret_cast<void*>(55);}
};
namespace nova::java {
struct FakePlayerSettings{bool enabled=false,copyEquipment=true,damage=true,autoReset=true;float health=20;};
enum class FakePlayerStatus{Off,Waiting,Active,Failed,Finished};
}
namespace mock {
std::thread::id game=std::this_thread::get_id();
JNIEnv env;int world=2,id=0,added=0,removed=0,equipment=0,resets=0,packetId=0;
bool alive=true,present=false,circuit=false,failAdd=false,failRemove=false,collision=false;
float health=20,absorption=0,strength=1,radius=6;double attack=7;
void* Obj(int n){return reinterpret_cast<void*>(static_cast<intptr_t>(n));}
int Id(void* o){return int(reinterpret_cast<intptr_t>(o));}
void Mutate(){assert(std::this_thread::get_id()==game);}
}
bool IsCircuitOpen(){return mock::circuit;}void LogTo(const char*,...){}
namespace nova::compat {
bool IsLunar12111(){return true;}bool IsMinecraft263(){return false;}
namespace mc263 {
namespace names {
struct Member {const char* owner;const char* name;const char* descriptor;};
#define NOVA_CLASS(id,name) inline constexpr const char* id=name;
#define NOVA_FIELD(id,owner,name,desc) inline constexpr Member id{owner,name,desc};
#define NOVA_STATIC_FIELD NOVA_FIELD
#define NOVA_METHOD NOVA_FIELD
#define NOVA_STATIC_METHOD NOVA_FIELD
#include "minecraft/compat/mappings/minecraft263.inc"
#undef NOVA_CLASS
#undef NOVA_FIELD
#undef NOVA_STATIC_FIELD
#undef NOVA_METHOD
#undef NOVA_STATIC_METHOD
}
namespace access {
using names::Member;
struct Frame {
    JNIEnv* env=&mock::env;explicit Frame(JNIEnv* e=nullptr){if(e)env=e;}
    explicit operator bool()const{return true;}bool check()const{return true;}
    jobject player(){return mock::Obj(1);}
    jobject stat(Member){return mock::Obj(10);}
    template<class... A>jobject staticCall(Member,A...){return mock::Obj(20);}
    template<class... A>jobject create(Member m,A...){mock::Mutate();return mock::Obj(std::string(m.owner)=="com.mojang.authlib.GameProfile"?30:40);}
    bool instance(jobject o,const char* type){return mock::Id(o)==15?std::string(type).find("InteractPacket")!=std::string::npos:
        mock::Id(o)==16&&std::string(type).find("ExplodePacket")!=std::string::npos;}
    template<class T=jobject,class... A>T call(jobject,Member m,A...){
        if constexpr(std::is_same_v<T,double>)return mock::attack;
        else if constexpr(std::is_same_v<T,float>)return mock::strength;
        else if constexpr(std::is_same_v<T,jobject>)return mock::Obj(std::string(m.name)=="copy"?101:100);
        else return T{};
    }
    template<class T=jobject>T get(jobject,Member m){
        if constexpr(std::is_same_v<T,int>)return mock::packetId;
        else if constexpr(std::is_same_v<T,float>)return mock::radius;
        else if constexpr(std::is_same_v<T,double>)return std::string(m.name)=="y"?64.:0.;
        else return mock::Obj(70);
    }
    template<class T>bool set(jobject,Member,T){mock::Mutate();return true;}
    bool invoke(jobject,Member m){
        mock::Mutate();const std::string name=m.name;
        if(name=="discard"){if(mock::failRemove)return false;mock::present=false;++mock::removed;}
        if(name=="resetAttackStrengthTicker"){++mock::resets;mock::strength=0;}
        return true;
    }
    bool invoke(jobject,Member m,int v){mock::Mutate();if(std::string(m.name)=="setId")mock::id=v;return true;}
    bool invoke(jobject,Member m,float v){mock::Mutate();if(std::string(m.name)=="setHealth")mock::health=v;else mock::absorption=v;return true;}
    bool invoke(jobject,Member,jobject,jobject copy){mock::Mutate();assert(mock::Id(copy)==101);++mock::equipment;return true;}
    bool invoke(jobject,Member m,jobject){mock::Mutate();if(std::string(m.name)=="addEntity"){++mock::added;mock::present=true;return !mock::failAdd;}return true;}
    bool invoke(jobject,Member,double,double,double){mock::Mutate();return true;}
};
}
}
}
namespace nova::java {
EntityHandle::~EntityHandle()=default;
EntityHandle::EntityHandle(EntityHandle&& e)noexcept:obj(std::exchange(e.obj,nullptr)){}
EntityHandle& EntityHandle::operator=(EntityHandle&& e)noexcept{obj=std::exchange(e.obj,nullptr);return *this;}
EntityHandle AdoptEntity(void* o){EntityHandle e;e.obj=o;return e;}
void EntityHandle::Reset(){obj=nullptr;}bool EntityHandle::IsSame(const EntityHandle& e)const{return obj==e.obj;}
bool EntityHandle::IsAlive()const{return mock::alive;}
bool EntityHandle::IsSpectator()const{return false;}
bool EntityHandle::Position(rotation::Vec3& p)const{p={0,64,0};return true;}
EntityHandle PlayerEntity(){return AdoptEntity(mock::Obj(1));}EntityHandle LevelEntity(){return AdoptEntity(mock::Obj(mock::world));}
EntityHandle EntityById(int id){return mock::collision&&id==-900001?AdoptEntity(mock::Obj(88)):
    mock::present&&id==mock::id?AdoptEntity(mock::Obj(40)):EntityHandle{};}
bool PlayerRotation(float& yaw,float& pitch){yaw=40;pitch=10;return true;}
bool ReadCrystalDefense(const EntityHandle&,CrystalDefense& d){d={};d.health=mock::health;return true;}
bool CrystalExposure(const EntityHandle&,const rotation::Vec3&,double& e){e=1;return true;}
}
// @PRODUCTION_ADAPTER@
void Start(bool reset=true,bool copy=true){
    using namespace nova::java;ConfigureFakePlayer({});mock::failRemove=false;TickFakePlayer();
    mock::world=2;mock::added=mock::removed=mock::equipment=mock::resets=0;
    mock::alive=true;mock::circuit=mock::failAdd=false;mock::strength=1;mock::attack=7;
    ConfigureFakePlayer({true,copy,true,reset,20});TickFakePlayer();
}
int main(){
    using namespace nova::java;using namespace nova::module::fakeplayer;
    assert(Melee(7,1)==7&&std::abs(Melee(7,0)-1.4)<1e-6);
    assert(nova::module::fakeplayer::Explosion(0,1,6,2)==85&&nova::module::fakeplayer::Explosion(13,1,6,2)==0&&nova::module::fakeplayer::Explosion(0,1,6,0)==0);
    assert(Mitigate(10,20,8,-1,0)<10&&Mitigate(10,20,8,4,20)==0);
    HitState hp;assert(hp.Hit(7,false,20)&&hp.health==13);assert(!hp.Hit(7,false,20));
    assert(hp.Hit(9,false,20)&&hp.health==11);for(int i=0;i<10;++i)hp.Tick();
    assert(hp.Hit(30,true,20)&&hp.health==20);
    ::Start();assert(GetFakePlayerStatus()==FakePlayerStatus::Active&&mock::added==1&&mock::equipment==6);
    assert(IsFakePlayer(mock::Obj(40))&&!IsFakePlayer(mock::Obj(1)));
    assert(!AttackFakePlayer(&mock::env,mock::Obj(1))&&AttackFakePlayer(&mock::env,mock::Obj(40)));
    assert(mock::health==13&&mock::resets==1);
    assert(AttackFakePlayer(&mock::env,mock::Obj(40))&&mock::health==13);
    mock::packetId=mock::id;assert(BlocksFakePlayerPacket(&mock::env,mock::Obj(15)));
    mock::packetId=99;assert(!BlocksFakePlayerPacket(&mock::env,mock::Obj(15)));
    std::thread ui([]{ConfigureFakePlayer({});});ui.join();assert(mock::removed==0);
    TickFakePlayer();assert(mock::removed==1&&WaitFakePlayerStopped(0));
    mock::packetId=mock::id;assert(BlocksFakePlayerPacket(&mock::env,mock::Obj(15))); // Queued old-id interactions stay local.
    Start(false);ObserveFakePlayerExplosion(&mock::env,mock::Obj(16));assert(mock::health==20);TickFakePlayer();
    assert(GetFakePlayerStatus()==FakePlayerStatus::Finished&&mock::removed==1);
    ::Start();ObserveFakePlayerExplosion(&mock::env,mock::Obj(16));TickFakePlayer();assert(mock::health==20&&mock::present);
    Start(false,false);assert(mock::equipment==0);mock::attack=100;AttackFakePlayer(&mock::env,mock::Obj(40));assert(mock::removed==1);
    ::Start();mock::world=9;TickFakePlayer();assert(GetFakePlayerStatus()==FakePlayerStatus::Finished&&mock::removed==1);
    ::Start();mock::failRemove=true;mock::circuit=true;TickFakePlayer();assert(!WaitFakePlayerStopped(0));
    mock::failRemove=false;TickFakePlayer();assert(WaitFakePlayerStopped(0));mock::circuit=false;
    mock::collision=true;::Start();assert(mock::id==-900002&&mock::present);mock::collision=false;
    ConfigureFakePlayer({});TickFakePlayer();mock::failAdd=true;ConfigureFakePlayer({true});TickFakePlayer();
    assert(GetFakePlayerStatus()==FakePlayerStatus::Failed&&!mock::present);
    std::puts("PASS: production FakePlayer lifecycle, deep equipment copies, local hits/cooldown, explosion queue, health reset, id isolation, world change, cleanup retry and game-thread-only mutation");
}
