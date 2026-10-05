#include "minecraft/modules/combat/keepsprint/KeepSprint.h"
#include <atomic>
#include <cassert>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <string>
#include <stdexcept>
#define JNICALL
using jdouble = double;
using jboolean = bool;
using jclass = int;
constexpr bool JNI_TRUE=true,JNI_FALSE=false;
void LogTo(const char*,...){ }
struct Object {
    double x=2,y=3,z=4;
    bool ground=false,sprint=true;
    int hurt=0,attacks=0;
} self,other,vector;
using jobject=Object*;
struct JNIEnv {
    bool IsSameObject(jobject a,jobject b){return a==b;}
    double GetDoubleField(jobject p,int field){return field==1?p->x:p->z;}
    void SetDoubleField(jobject p,int field,double v){(field==1?p->x:p->z)=v;}
    void CallVoidMethod(jobject p,int,bool value){p->sprint=value;}
} env;
namespace nova {
namespace java {
bool interruptCriticalSprint=false;
bool CriticalSprintInterrupted(){return interruptCriticalSprint;}
}
namespace compat {
int profile=0;
bool IsLegacyLunar(){return profile==0;}
bool IsMinecraft263(){return profile==2;}
bool UsesModernRuntime(){return IsMinecraft263();}
namespace mc263::access {
struct Member { const char *owner=nullptr,*name=nullptr,*descriptor=nullptr; };
struct Frame {
    JNIEnv* env;
    Frame(JNIEnv* e=nullptr):env(e?e:&::env){}
    bool check(){return true;}
    jobject player(){return &self;}
    jobject stat(Member){return &other;}
    template<class T> T get(jobject p,Member){
        if constexpr(std::is_same_v<T,bool>)return p->ground;
        else return p->hurt;
    }
    template<class T=jobject,class... A>T call(jobject p,Member m,A... a){
        if constexpr(std::is_same_v<T,bool>)return p->ground;
        else {
            if constexpr(sizeof...(a)==3){auto values=std::tuple{a...};vector=*p;vector.x*=std::get<0>(values);vector.y*=std::get<1>(values);vector.z*=std::get<2>(values);return &vector;}
            else return p;
        }
    }
    template<class A>bool invoke(jobject p,Member m,A a){
        if constexpr(std::is_same_v<A,bool>)p->sprint=a;
        else if(std::string(m.name)=="setDeltaMovement"||std::string(m.name)=="m_20256_"){p->x=a->x;p->y=a->y;p->z=a->z;}
        else ++p->attacks;
        return true;
    }
};
}
}
namespace event {
bool enabled=true;
int posted=0;
module::keepsprint::State state;
module::keepsprint::Settings settings;
struct AttackSlowdownEvent {
    double slowdown=.6;bool sprint=false,onGround;int hurtTime;
    AttackSlowdownEvent(bool ground,int hurt):onGround(ground),hurtTime(hurt){}
};
struct EventBus {
    static EventBus& Get(){static EventBus bus;return bus;}
    bool Post(AttackSlowdownEvent& e){++posted;if(enabled){auto r=state.Hit(settings,e.onGround,e.hurtTime);e.slowdown=r.slowdown;e.sprint=r.sprint;}return false;}
};
}
}
// @GAME_PRODUCTION@
namespace nova::compat::lunar {
struct Handles {int MotionX=1,MotionZ=2,SetSprinting=3;} h;
bool Local(JNIEnv*,jobject p){return p==&self;}
bool IsCircuitOpen(){return false;}
std::atomic<bool> predictionBlockLag=false;
// @LUNAR_PRODUCTION@
}
int main(){
    using namespace nova;
    event::settings.normal=1;
    event::settings.normalSprint=true;
    event::settings.buffer=true;
    compat::lunar::AttackMotionXHook(&env,0,&self,1.2);
    compat::lunar::AttackMotionZHook(&env,0,&self,2.4);
    compat::lunar::AttackSprintingHook(&env,0,&self,false);
    assert(self.x==2&&self.z==4&&self.sprint&&event::posted==1&&event::state.combo==1);
    compat::lunar::AttackMotionXHook(&env,0,&other,1.2);
    compat::lunar::AttackMotionZHook(&env,0,&other,2.4);
    compat::lunar::AttackSprintingHook(&env,0,&other,false);
    assert(other.x==1.2&&other.z==2.4&&!other.sprint&&event::posted==1);
    // Existing Prediction suppression takes precedence over hit modifications.
    compat::lunar::predictionBlockLag=true;
    event::settings.normal=0;
    compat::lunar::AttackMotionXHook(&env,0,&self,1.2);
    compat::lunar::AttackMotionZHook(&env,0,&self,2.4);
    compat::lunar::AttackSprintingHook(&env,0,&self,false);
    assert(self.x==2&&self.z==4&&self.sprint);
    compat::lunar::predictionBlockLag=false;
    for(int version=1;version<=2;++version){
        compat::profile=version;self={};event::settings.buffer=false;
        Object vanilla;vanilla.x=1.2;vanilla.z=2.4;
        java::AttackVelocityHook(&env,0,&self,&vanilla);
        java::AttackSprintHook(&env,0,&self,false);
        assert(self.x==0&&self.y==3&&self.z==0&&self.sprint);
        java::interruptCriticalSprint=true;
        java::AttackVelocityHook(&env,0,&self,&vanilla);
        java::AttackSprintHook(&env,0,&self,false);
        assert(!self.sprint && self.x==0 && self.z==0); // Keep slowdown, defer sprint.
        java::interruptCriticalSprint=false;
        java::AttackVelocityHook(&env,0,&self,&vanilla);
        java::AttackSprintHook(&env,0,&self,false);
        assert(self.sprint); // Scope ended: KeepSprint behavior is restored.
        event::enabled=false;self={};
        java::AttackVelocityHook(&env,0,&self,&vanilla);
        java::AttackSprintHook(&env,0,&self,false);
        assert(self.x==1.2&&self.y==3&&self.z==2.4&&!self.sprint);
        event::enabled=true;
    }
    std::cout<<"PASS: production hooks; one event per hit, XYZ, sprint, Prediction and remote player isolation\n";
}
