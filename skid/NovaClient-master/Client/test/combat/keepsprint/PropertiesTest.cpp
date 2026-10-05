#include "minecraft/modules/combat/keepsprint/KeepSprint.h"
#include "config/settings/Properties.h"
#include "minecraft/utils/rotation/MovementFix.h"
#include "minecraft/utils/rotation/Rotation.h"
#include <cassert>
#include <cstdio>
#include <deque>
#include <memory>
#include <iostream>
#include <type_traits>
namespace mock {
bool menu=false,gui=false,foreground=true,freecam=false,scaffold=false,suppressed=false;
bool player=true,world=true,alive=true,sprintKey=false,sprintModule=false,submitted=false;
bool physicalSprint=false;
bool speedModule=false;
int restores=0;
float yaw=0,pitch=10;
}
namespace nova::java {
struct EntityHandle { bool valid; explicit operator bool()const{return valid;} bool IsAlive()const{return mock::alive;} };
EntityHandle PlayerEntity(){return {mock::player};} EntityHandle LevelEntity(){return {mock::world};}
bool ScreenOpen(){return mock::menu;} bool FreeCamActive(){return mock::freecam;}
bool PlayerRotation(float& yaw,float& pitch){yaw=mock::yaw;pitch=mock::pitch;return true;}
bool SetSprintKeyDown(bool down){mock::sprintKey=down;return true;}
bool RestoreSprintKey(bool resume){mock::sprintKey=resume||mock::physicalSprint;++mock::restores;return true;}
}
namespace nova::clickgui { bool IsOpen(){return mock::gui;} }
namespace nova::input { bool IsProcessForeground(){return mock::foreground;} }
namespace nova::rotation {
enum class RotationPriority { Medium,Highest };
struct RotationState { bool active=false,direct=false,linear=false; Rotation rotations; RotationPriority priority=RotationPriority::Medium; } state;
RotationState& State(){return state;}
bool IsActive(){return state.active;}
bool AutomatedItemUseSubmitted(){return mock::submitted;}
bool SetDirectRotations(const Rotation& r){state.active=state.direct=true;state.rotations=r;return true;}
void ReleaseDirectRotations(){state={};}
}
namespace nova::event {
namespace Priority { constexpr int High=5,Low=15; }
enum class EventType { Pre,Post };
struct GameTickEvent { EventType type; };
struct MoveInputEvent {
    float forward=0,strafe=0;bool jump=false,sneak=false,suppressSprint=false,skipRotationFix=false;
    bool hasFinalForward=false,hasFinalStrafe=false,requestSprint=false;
};
struct AttackSlowdownEvent {
    double slowdown=.6;bool sprint=false,onGround;int hurtTime;
    AttackSlowdownEvent(bool ground,int hurt):onGround(ground),hurtTime(hurt){}
};
struct Subscription {};
struct EventBus {
    std::function<void(AttackSlowdownEvent&)> listener;
    std::function<void(MoveInputEvent&)> input;
    std::function<void(GameTickEvent&)> tick;
    static EventBus& Get(){static EventBus bus;return bus;}
    template<class E,class F> Subscription Subscribe(F&& f,int priority=10){
        if constexpr(std::is_same_v<E,AttackSlowdownEvent>) listener=std::forward<F>(f);
        else if constexpr(std::is_same_v<E,MoveInputEvent>) {assert(priority<Priority::High);input=std::forward<F>(f);}
        else tick=std::forward<F>(f);
        return {};
    }
};
}
namespace nova::module {
enum class Category { Combat };
class Module {
    std::deque<std::string> labels;
    std::vector<std::unique_ptr<Properties::AbstractProperty>> values;
public:
    bool enabled=false;
    std::vector<Properties::PropDesc> rows;
    Module(std::string,Category,const char*,const char*){}
    virtual ~Module()=default;
    virtual void onDisable(){}
    bool isEnabled()const{return enabled;}
    template<class T,class... A>T& Own(std::string name,Properties::PropDesc::Visibility visibility,A&&... args){
        auto prop=std::make_unique<T>(std::forward<A>(args)...);auto& ref=*prop;
        values.push_back(std::move(prop));rows.emplace_back(labels.emplace_back(std::move(name)).c_str(),&ref,std::vector<Properties::PropDesc>{},std::move(visibility));return ref;
    }
    Properties::NumberProp& addNumber(std::string name,double min,double max,double step,double value,
        std::vector<Properties::PropDesc>,std::function<std::string(double)> format,Properties::PropDesc::Visibility visibility={}){
        return Own<Properties::NumberProp>(std::move(name),std::move(visibility),min,max,step,value,std::move(format));
    }
    Properties::BoolProperty& addBool(std::string name,bool value=false){return Own<Properties::BoolProperty>(std::move(name),{},value);}
};
struct ModuleManager {
    Module sprint{"Sprint",Category::Combat,"General","General"};
    Module speed{"Speed",Category::Combat,"General","General"};
    static ModuleManager& Get(){static ModuleManager manager;return manager;}
    Module* Find(const char* name){
        if(std::string(name)=="Speed"){speed.enabled=mock::speedModule;return &speed;}
        sprint.enabled=mock::sprintModule;return &sprint;
    }
};
}
namespace nova::module::scaffold { bool IsEnabled(){return mock::scaffold;} }
namespace nova::module::invmove { bool SuppressSprint(){return mock::suppressed;} }
namespace nova::module::autototem { bool SuppressMovement(){return mock::suppressed;} }
namespace nova::module::autoreplenish { bool SuppressMovement(){return mock::suppressed;} }
// @MODULE_PRODUCTION@
int main(){
    auto module=nova::module::MakeKeepSprintModule();
    auto row=[&](const char* name)->Properties::PropDesc&{for(auto& r:module->rows)if(std::string(r.getName())==name)return r;std::abort();};
    auto number=[&](const char* name){return static_cast<Properties::NumberProp*>(row(name).getProp());};
    auto boolean=[&](const char* name){return static_cast<Properties::BoolProperty*>(row(name).getProp());};
    assert(module->rows.size()==9&&!module->enabled);
    for(auto name: {"Hit Slow Down During Velocity","Hit Slow Down Normal"}){
        auto p=number(name);assert(std::abs(p->get()-.6)<1e-15&&p->getMin()==0&&p->getMax()==1&&p->getStep()==.05);
        assert(p->formatValue(.6)=="0.60");
    }
    assert(number("Buffer Decrease")->get()==1&&number("Buffer Decrease")->getMin()==.1&&number("Buffer Decrease")->getMax()==10&&number("Buffer Decrease")->getStep()==.1);
    assert(number("Max Buffer")->get()==5&&number("Max Buffer")->getMin()==1&&number("Max Buffer")->getMax()==10&&number("Max Buffer")->getStep()==1);
    for(auto name:{"Velocity Hit Sprint","Normal Hit Sprint","Buffer Abuse","Only In Air","Rotation"})assert(!boolean(name)->get());
    assert(!row("Buffer Decrease").isVisible()&&!row("Max Buffer").isVisible());
    auto hit=[&](bool ground,int hurt){nova::event::AttackSlowdownEvent e(ground,hurt);nova::event::EventBus::Get().listener(e);return e;};
    module->enabled=true;
    assert(hit(false,0).slowdown==.6); // exact Rise default, not 0.6000000000000001
    number("Hit Slow Down Normal")->set(1);boolean("Normal Hit Sprint")->set(true);
    number("Hit Slow Down During Velocity")->set(.25);boolean("Velocity Hit Sprint")->set(false);
    assert(hit(false,0).slowdown==1&&hit(false,1).slowdown==.25);
    boolean("Buffer Abuse")->set(true);
    assert(row("Buffer Decrease").isVisible()&&row("Max Buffer").isVisible());
    number("Max Buffer")->set(1);
    assert(hit(false,0).sprint);
    module->enabled=false;assert(!hit(false,0).sprint);
    module->enabled=true;assert(!hit(false,0).sprint); // preserved combo, now resetting
    boolean("Only In Air")->set(true);assert(!hit(true,0).sprint);
    assert(hit(false,0).sprint); // zero-buffer transition
    nova::json::Value saved;
    number("Buffer Decrease")->set(.3);number("Buffer Decrease")->save(saved);
    number("Buffer Decrease")->set(5);number("Buffer Decrease")->load(saved);
    assert(number("Buffer Decrease")->formatValue(number("Buffer Decrease")->get())=="0.30");
    std::cout<<"PASS: original Rise settings and attack behavior, defaults/ranges/steps, visibility, persistence and config\n";
    auto move=[&](float forward,float strafe,bool sneak=false){
        nova::event::MoveInputEvent e{forward,strafe,false,sneak};
        nova::event::EventBus::Get().input(e);
        // The production RotationBridge runs next, with real movement-fix math.
        if(nova::rotation::IsActive()&&!e.skipRotationFix){
            nova::rotation::MoveInput fixed{e.forward,e.strafe};
            nova::rotation::FixMovement(fixed,nova::rotation::State().rotations.yaw,mock::yaw);
            e.forward=fixed.forward;e.strafe=fixed.strafe;
        }
        return e;
    };
    auto idle=move(-1,0);assert(idle.forward==-1&&!idle.requestSprint&&!nova::rotation::IsActive());
    boolean("Rotation")->set(true);
    for(float yaw:{-720.f,-181.f,0.f,179.f,1080.f}){
        mock::yaw=yaw;
        for(auto direction:std::vector<std::pair<float,float>>{{1.f,0.f},{-1.f,0.f},{0.f,1.f},{0.f,-1.f},{1.f,1.f},{1.f,-1.f},{-1.f,1.f},{-1.f,-1.f}}){
            auto e=move(direction.first,direction.second);
            const float physical=nova::rotation::Direction(direction.first,direction.second,yaw);
            const float actual=nova::rotation::Direction(e.forward,e.strafe,nova::rotation::State().rotations.yaw);
            assert(e.forward==1&&e.strafe==0&&e.requestSprint&&mock::sprintKey);
            assert(std::abs(std::remainder(actual-physical,360.f))<.001f);
            assert(mock::yaw==yaw&&nova::rotation::State().rotations.pitch==mock::pitch);
        }
    }
    auto unchanged=hit(true,0);assert(!unchanged.sprint&&unchanged.slowdown==.6); // Only In Air still wins.
    idle=move(0,0);assert(!idle.requestSprint&&!mock::sprintKey&&!nova::rotation::IsActive());
    move(-1,0);move(-1,0,true);assert(!nova::rotation::IsActive()&&!mock::sprintKey);
    for(bool* flag:{&mock::menu,&mock::gui,&mock::freecam,&mock::scaffold,&mock::suppressed,&mock::submitted,&mock::speedModule}){
        move(-1,0);*flag=true;auto e=move(-1,0);assert(!e.requestSprint&&!nova::rotation::IsActive());*flag=false;
    }
    move(-1,0);mock::foreground=false;move(-1,0);assert(!nova::rotation::IsActive());mock::foreground=true;
    for(bool* flag:{&mock::player,&mock::world,&mock::alive}){
        move(-1,0);*flag=false;move(-1,0);assert(!nova::rotation::IsActive());*flag=true;
    }
    move(-1,0);
    nova::rotation::state.rotations={45,20};nova::rotation::state.priority=nova::rotation::RotationPriority::Highest;
    idle=move(-1,0);assert(!idle.requestSprint&&nova::rotation::state.active&&nova::rotation::state.rotations.yaw==45);
    nova::rotation::state={};
    for(int i=0;i<4;++i){
        move(-1,0);nova::event::MoveInputEvent e{-1,0};
        if(i==0)e.suppressSprint=true;if(i==1)e.skipRotationFix=true;if(i==2)e.hasFinalForward=true;if(i==3)e.hasFinalStrafe=true;
        nova::event::EventBus::Get().input(e);assert(!nova::rotation::IsActive()&&!e.requestSprint);
    }
    move(-1,0);mock::sprintModule=true;move(0,0);assert(mock::sprintKey);mock::sprintModule=false;
    move(-1,0);mock::physicalSprint=true;move(0,0);assert(mock::sprintKey);
    move(-1,0);mock::suppressed=true;move(-1,0);assert(!mock::sprintKey);
    mock::suppressed=false;mock::physicalSprint=false;
    move(-1,0);boolean("Rotation")->set(false);
    nova::event::GameTickEvent tick{nova::event::EventType::Pre};nova::event::EventBus::Get().tick(tick);
    assert(!nova::rotation::IsActive()&&!mock::sprintKey);
    boolean("Rotation")->set(true);move(-1,0);module->enabled=false;module->onDisable();
    assert(!nova::rotation::IsActive()&&!mock::sprintKey);
    boolean("Rotation")->save(saved);boolean("Rotation")->set(false);boolean("Rotation")->load(saved);assert(boolean("Rotation")->get());
    std::cout<<"PASS: Rotation eight movement directions, camera preservation, movement fix, independent hit logic, arbitration, cleanup and saved switch\n";
}
