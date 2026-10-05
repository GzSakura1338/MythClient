#include "minecraft/events/GameEvents.h"
#include "minecraft/java/api/game_speed.h"
#include "minecraft/modules/combat/killaura/CriticalSprint.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/utils/rotation/RotationUtils.h"
#include "minecraft/utils/rotation/Mth.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <string_view>

namespace mock {
bool menu=false,gui=false,foreground=true,freecam=false,scaffold=false,suppressed=false;
bool valid=true,ground=true,blocked=false,sprintKey=false,sprintModule=false,physicalSprint=false;
bool auraTargets=false;
int jumpResets=0;
float yaw=0,pitch=10;
}
namespace nova::java {
bool ScreenOpen(){return mock::menu;}
bool FreeCamActive(){return mock::freecam;}
bool PlayerRotation(float& yaw,float& pitch){yaw=mock::yaw;pitch=mock::pitch;return true;}
bool PlayerPreviousRotation(float& yaw,float& pitch){return PlayerRotation(yaw,pitch);}
bool SetPlayerRotation(float yaw,float pitch){mock::yaw=yaw;mock::pitch=pitch;return true;}
bool SetPlayerCameraRotation(float yaw,float pitch,double){return SetPlayerRotation(yaw,pitch);}
bool PlayerSensitivity(double& value){value=.5;return true;}
int ClientFps(){return 120;}
int PlayerTickCount(){return 100;}
void PickCursor(float){}
bool ReadSpeedEnvironment(SpeedEnvironment& out){out={mock::ground,mock::blocked};return mock::valid;}
bool SetSprintKeyDown(bool down){mock::sprintKey=down;return true;}
bool RestoreSprintKey(bool resume){mock::sprintKey=resume||mock::physicalSprint;return true;}
bool ResetPlayerJumpDelay(){++mock::jumpResets;return true;}
EntityHandle::~EntityHandle()=default;
bool EntityHandle::IsSame(const EntityHandle& other)const{return raw()==other.raw();}
EntityHandle PlayerEntity(){return {};}
}
namespace nova::clickgui {bool IsOpen(){return mock::gui;}}
namespace nova::input {bool IsProcessForeground(){return mock::foreground;}}
namespace nova::rotation {double Random(){return .5;}}
namespace nova::module::scaffold {bool IsEnabled(){return mock::scaffold;}}
namespace nova::module::invmove {bool SuppressSprint(){return mock::suppressed;}}
namespace nova::module::autototem {bool SuppressMovement(){return mock::suppressed;}}
namespace nova::module::killaura {bool HasTargets(){return mock::auraTargets;}}
namespace nova::module::autoreplenish {bool SuppressMovement(){return mock::suppressed;}}
namespace nova::module {
enum class Category {Movement};
class Module {
public:
    bool enabled=false;
    std::string name;
    Category category;
    Module(const char* name,Category category,const char*,const char*):name(name),category(category){}
    virtual ~Module()=default;
    virtual void onDisable(){}
    bool isEnabled()const{return enabled;}
};
struct ModuleManager {
    Module sprint{"Sprint",Category::Movement,"General","General"};
    static ModuleManager& Get(){static ModuleManager manager;return manager;}
    Module* Find(std::string_view){sprint.enabled=mock::sprintModule;return &sprint;}
};
}
// @PRODUCTION_MODULE@

namespace {
using namespace nova;
constexpr double radians=3.141592653589793/180.0;
rotation::Vec3 Acceleration(float forward,float strafe,float yaw,bool modern){
    if(modern){const float len=std::hypot(forward,strafe);if(len>1){forward/=len;strafe/=len;}}
    forward*=.98f;strafe*=.98f;
    const float len=std::hypot(forward,strafe);
    const float scale=.026f/std::max(1.f,len);
    forward*=scale;strafe*=scale;
    const float sin=rotation::mth::Sin(static_cast<float>(yaw*radians));
    const float cos=rotation::mth::Cos(static_cast<float>(yaw*radians));
    return {strafe*cos-forward*sin,0,forward*cos+strafe*sin};
}
void Near(double a,double b,double tolerance=.0001){assert(std::abs(a-b)<tolerance);}
event::MoveInputEvent Move(float forward,float strafe,bool manualJump=false,bool sneak=false){
    event::MoveInputEvent e(forward,strafe,manualJump,sneak,.3);
    (void)event::EventBus::Get().Post(e);return e;
}
}
int main(){
    using namespace nova;
    auto speed=module::MakeSpeedModule();
    // Install the actual movement bridge after the module, as Registry does.
    auto bridge=event::EventBus::Get().Subscribe<event::MoveInputEvent>([](auto& e){
        if(e.skipRotationFix)return;
        rotation::MoveInput input{e.forward,e.strafe};rotation::OnMoveInput(input);
        e.forward=input.forward;e.strafe=input.strafe;
    },event::Priority::High);
    event::EventBus::Get().Seal();
    assert(speed->name=="Speed"&&speed->category==module::Category::Movement&&!speed->isEnabled());
    rotation::UpdateGlobalYaw();
    auto e=Move(1,0);assert(!e.jump&&!e.requestSprint&&!rotation::IsActive());
    speed->enabled=true;
    for(float yaw:{-720.f,-181.f,0.f,179.f,1080.f}){
        mock::yaw=yaw;
        for(auto dir: {rotation::MoveInput{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}}){
            mock::ground=true;
            e=Move(dir.forward,dir.strafe);
            assert(e.jump&&e.requestSprint&&mock::sprintKey&&e.forward==1&&e.strafe==0);
            const float heading=rotation::Direction(dir.forward,dir.strafe,yaw);
            auto angle=rotation::GetRotation();
            // Shared sensitivity quantization can move a desired heading by
            // half a mouse step (.075 degrees at sensitivity .5).
            Near(std::remainder(angle.yaw-heading,360.f),0,.08);
            // The full vanilla 0.2 sprint-jump impulse remains along travel.
            const double projected=.2*std::cos((angle.yaw-heading)*radians);
            Near(projected,.2);
            event::JumpEvent jump(mock::yaw);rotation::OnJump(jump.yaw);
            Near(std::remainder(jump.yaw-heading,360.f),0,.08);
            float outYaw=yaw,outPitch=mock::pitch;rotation::OnMotionPre(outYaw,outPitch);
            mock::ground=false;
            e=Move(dir.forward,dir.strafe);
            assert(!e.jump&&e.requestSprint&&e.forward==1&&e.strafe==1);
            angle=rotation::GetRotation();
            Near(std::remainder(angle.yaw-heading,360.f),45,.08);
            auto actual=Acceleration(e.forward,e.strafe,angle.yaw,false);
            const double length=std::hypot(actual.x,actual.z);
            Near(length,.026,1e-5);
            const double effectiveHeading=angle.yaw-45.f;
            Near(actual.x/length,-std::sin(effectiveHeading*radians),.0002);
            Near(actual.z/length,std::cos(effectiveHeading*radians),.0002);
            const auto straight=Acceleration(1,0,heading,false);
            Near(length/std::hypot(straight.x,straight.z),1/.98,.0002);
            const auto modern=Acceleration(e.forward,e.strafe,angle.yaw,true);
            Near(std::hypot(modern.x,modern.z),.026*.98,1e-5);
            Near(std::remainder(mock::yaw-yaw,360.f),0);
            assert(mock::pitch==10);
            rotation::OnMotionPre(outYaw,outPitch);
        }
    }
    e=Move(1,0,true);assert(e.jump); // Held manual jump is never cleared in flight.
    e=Move(0,0);assert(!e.jump&&!e.requestSprint&&!rotation::IsActive()&&!mock::sprintKey);
    mock::ground=true;
    for(bool* flag:{&mock::menu,&mock::gui,&mock::freecam,&mock::scaffold,&mock::suppressed,&mock::blocked}){
        Move(1,0);*flag=true;e=Move(1,0);assert(!e.jump&&!e.requestSprint&&!rotation::IsActive());*flag=false;
    }
    for(bool* flag:{&mock::foreground,&mock::valid}){
        Move(1,0);*flag=false;e=Move(1,0);assert(!e.jump&&!rotation::IsActive());*flag=true;
    }
    Move(1,0);e=Move(1,0,false,true);assert(e.sneak&&!e.jump&&!rotation::IsActive());
    Move(1,0);e=Move(std::numeric_limits<float>::quiet_NaN(),0);assert(!e.jump&&!rotation::IsActive());
    for(int i=0;i<4;++i){
        Move(1,0);event::MoveInputEvent guarded(1,0,false,false,.3);
        if(i==0)guarded.suppressSprint=true;if(i==1)guarded.skipRotationFix=true;
        if(i==2)guarded.hasFinalForward=true;if(i==3)guarded.hasFinalStrafe=true;
        (void)event::EventBus::Get().Post(guarded);assert(!guarded.jump&&!guarded.requestSprint&&!rotation::IsActive());
    }
    Move(1,0);
    rotation::SetExclusiveRotations(&mock::blocked,{100,70});
    e=Move(1,0);assert(!e.jump&&!e.requestSprint&&rotation::OwnsExclusiveRotations(&mock::blocked));
    rotation::ReleaseExclusiveRotations(&mock::blocked);rotation::OnRespawn();
    rotation::SetLinearRotations({30,20},360,360);
    e=Move(1,0);assert(!e.jump&&!e.requestSprint&&rotation::State().linear);
    rotation::OnRespawn();
    mock::auraTargets=true;
    const int before=mock::jumpResets;
    for(int i=0;i<30;++i){
        // An ordinary KillAura request, running through the real manager.
        rotation::SetRotations({90,20},180,nullptr,rotation::RotationPriority::Medium,false);
        const auto target=rotation::GetRotation();
        mock::ground=true;e=Move(1,0);
        assert(e.jump&&!e.requestSprint&&rotation::IsActive());
        Near(rotation::GetRotation().yaw,target.yaw);Near(rotation::GetRotation().pitch,target.pitch);
        mock::ground=false;e=Move(1,0);
        assert(!e.jump&&!e.requestSprint);Near(rotation::GetRotation().yaw,target.yaw);
    }
    assert(mock::jumpResets==before+30);
    mock::ground=true;
    module::killaura::CriticalSprint criticalState;
    criticalState.Tick(.25,true,0);
    auto critical=event::EventBus::Get().Subscribe<event::MoveInputEvent>([&](auto& e){criticalState.Input(e.forward);});
    e=Move(1,0);assert(e.jump&&e.forward==0&&!e.requestSprint);critical.Reset();
    rotation::SetExclusiveRotations(&mock::blocked,{100,70});
    e=Move(1,0);assert(!e.jump&&!e.requestSprint&&rotation::OwnsExclusiveRotations(&mock::blocked));
    rotation::ReleaseExclusiveRotations(&mock::blocked);rotation::OnRespawn();
    rotation::SetRotations({90,20},180,nullptr,rotation::RotationPriority::Highest,false);
    e=Move(1,0);assert(!e.jump&&!e.requestSprint);
    rotation::OnRespawn();mock::auraTargets=false;
    Move(1,0);mock::sprintModule=true;Move(0,0);assert(mock::sprintKey);mock::sprintModule=false;
    Move(1,0);mock::physicalSprint=true;Move(0,0);assert(mock::sprintKey);
    Move(1,0);mock::suppressed=true;Move(1,0);assert(!mock::sprintKey);
    mock::suppressed=false;mock::physicalSprint=false;
    Move(1,0);speed->enabled=false;speed->onDisable();assert(!rotation::IsActive()&&!mock::sprintKey);
    std::puts("PASS: actual Speed module + RotationManager + movement bridge: eight directions, straight takeoff, 45 degree flight, vanilla legacy/modern normalization, camera, manual jump, arbitration and cleanup");
    std::puts("PASS: repeated KillAura-owned ground hops clear cooldown without changing aim, sprint requests or critical input; emergency/exclusive rotations remain isolated");
}
