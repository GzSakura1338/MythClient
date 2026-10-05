#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <deque>
#include <format>
#include <functional>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include "config/settings/Properties.h"
#include "minecraft/modules/player/bridgeassist/BridgeAssistState.h"
#include "minecraft/utils/rotation/Vec3.h"
#include "minecraft/utils/rotation/Mth.h"

namespace mock {
bool valid=true, screen=false, gui=false, ground=true, flying=false, manual=false, key=false;
bool blocks=true, prediction=true, ray=false, rotation=false;
bool lunar=true, mining=false, sensitivityOk=true, support=true;
int raySide=2, rayX=0, rayCalls=0, rotationWrites=0;
float yaw=0, reportedYaw=0, reportedPitch=80, outYaw=0, outPitch=0;
double sensitivity=.5;
int tick=10, identity=1, predicts=0, releases=0;
int selected=0, blockSlot=-1, blockCount=64, slotWrites=0;
bool slotWriteOk=true;
float pitch=80, forward=-1;
double offset=0;
void Reset() {
    valid=ground=blocks=prediction=true;
    screen=gui=flying=manual=key=ray=rotation=false;
    pitch=80; forward=-1; offset=0; tick=10; ++identity;
    selected=0; blockSlot=-1; blockCount=64; slotWrites=0; slotWriteOk=true;
    lunar=sensitivityOk=support=true; mining=false; raySide=2; rayX=rayCalls=rotationWrites=0;
    yaw=reportedYaw=outYaw=outPitch=0; reportedPitch=80; sensitivity=.5;
}
}
namespace nova::java {
struct Box { double minX,minY,minZ,maxX,maxY,maxZ; };
struct BlockPos { int x=0,y=0,z=0; };
enum class HitKind { Miss,Block,Entity };
struct BlockHitResultInfo { HitKind kind=HitKind::Miss; bool hasBlockPos=false; int direction=2; BlockPos blockPos; };
struct EntityHandle {
    int id=0;
    explicit operator bool() const { return id!=0; }
    bool IsSame(const EntityHandle& other) const { return id==other.id; }
    void Reset() { id=0; }
    bool OnGround(bool& out) const { out=mock::ground; return mock::valid; }
    bool TickCount(int& out) const { out=mock::tick; return mock::valid; }
    bool BoundingBox(Box& out) const { out={.2,1,.2,.8,2.8,.8}; return mock::valid; }
    bool EyePosition(float,rotation::Vec3& out) const { out={.5,2.6,.5}; return mock::valid; }
};
struct BridgePlayer { bool flying=false,manualSneak=false; float forward=0; double reach=4.5; };
EntityHandle PlayerEntity() { return {mock::valid?mock::identity:0}; }
bool ReadBridgePlayer(BridgePlayer& out) { out.flying=mock::flying; out.manualSneak=mock::manual; out.forward=mock::forward; return mock::valid; }
bool BridgeEdgeOffset(float,float,bool,double& out) { ++mock::predicts; out=mock::offset; return mock::prediction; }
bool ScreenOpen() { return mock::screen; }
bool SetKeyShiftDown(bool v) { mock::key=v; return true; }
bool HoldingBlockItem() { return mock::blocks; }
bool SelectedSlot(int& out) { out=mock::selected; return mock::valid; }
bool SetSelectedSlot(int slot) {
    if (!mock::valid || !mock::slotWriteOk) return false;
    mock::selected=slot; mock::blocks=slot==mock::blockSlot; ++mock::slotWrites; return true;
}
int FindPlaceableBlockSlot(int reserve) { return mock::blockCount>reserve?mock::blockSlot:-1; }
float PlayerPitch() { return mock::pitch; }
bool PlayerRotation(float& yaw,float& pitch) { yaw=mock::yaw; pitch=mock::pitch; return mock::valid; }
bool PlayerSensitivity(double& out) { out=mock::sensitivity; return mock::sensitivityOk; }
bool PacketIsUseItemOn(void* p) { return p==reinterpret_cast<void*>(1); }
bool BlockStateReplaceable(const BlockPos& p) { return !mock::support || p.x!=0 || p.z!=0; }
int FacingStepX(int s) { return s==4?-1:s==5?1:0; }
int FacingStepZ(int s) { return s==2?-1:s==3?1:0; }
bool WorldClip(const rotation::Vec3&,const rotation::Vec3&,const EntityHandle&,BlockHitResultInfo& out) {
    ++mock::rayCalls;
    out.kind=HitKind::Block; out.hasBlockPos=true; out.direction=mock::raySide; out.blockPos.x=mock::rayX; return mock::ray;
}
}
namespace nova::event {
enum class EventType { Pre,Post,Outgoing,Incoming };
namespace Priority { constexpr int Highest=0; }
struct MoveInputEvent { float forward=-1,strafe=0; bool jump=false,sneak=false; };
struct PacketEvent { EventType type; void* packet; };
struct PrePlayerInteractEvent {};
struct ClientRotationEvent {};
struct GameTickEvent { EventType type=EventType::Pre; };
struct Subscription {};
struct EventBus {
    std::function<void(MoveInputEvent&)> input;
    std::function<void(PacketEvent&)> packet;
    std::function<void(ClientRotationEvent&)> rotation;
    std::function<void(PrePlayerInteractEvent&)> interact;
    std::function<void(GameTickEvent&)> tick;
    static EventBus& Get() { static EventBus b; return b; }
    template<class E,class F> Subscription Subscribe(F callback,int=0) {
        if constexpr(std::is_same_v<E,MoveInputEvent>) input=callback;
        if constexpr(std::is_same_v<E,PacketEvent>) packet=callback;
        if constexpr(std::is_same_v<E,ClientRotationEvent>) rotation=callback;
        if constexpr(std::is_same_v<E,PrePlayerInteractEvent>) interact=callback;
        if constexpr(std::is_same_v<E,GameTickEvent>) tick=callback;
        return {};
    }
};
}
namespace nova::clickgui { bool IsOpen() { return mock::gui; } }
namespace nova::rotation {
struct Rotation { float yaw=0,pitch=0; };
enum class RotationPriority { Medium,Highest };
struct RotationState {
    bool direct=false,active=false,hasLastRotations=false,hasAnimation=false;
    Rotation rotations,lastRotations,animationRotation;
    RotationPriority priority=RotationPriority::Medium;
};
RotationState& State() { static RotationState s; return s; }
Rotation GetRotation() { return State().rotations; }
bool SetDirectRotations(const Rotation& r) {
    auto& s=State();
    if(s.active && s.priority==RotationPriority::Highest) return false;
    s.direct=s.active=true; s.rotations=r;
    mock::rotation=true; mock::outYaw=r.yaw; mock::outPitch=r.pitch; ++mock::rotationWrites; return true;
}
void ReleaseDirectRotations() { State().active=State().direct=false; mock::rotation=false; ++mock::releases; }
}
namespace nova::compat {
bool IsLegacyLunar() { return mock::lunar; }
namespace lunar {
bool PlayerReportedRotation(float& yaw,float& pitch) { yaw=mock::reportedYaw; pitch=mock::reportedPitch; return mock::valid; }
}
}
namespace nova::module::breaker { bool OwnsMining() { return mock::mining; } }
namespace nova::module::automlg { bool IsWorking() { return false; } }
namespace nova::module::autocharge { bool IsWorking() { return false; } }
namespace nova::module {
enum class Category { Player };
struct Module {
    bool enabled=false;
    std::vector<std::unique_ptr<Properties::AbstractProperty>> owned;
    std::vector<Properties::PropDesc> rows;
    Module(const char*,Category,const char*,const char*) {}
    virtual ~Module()=default;
    virtual void onEnable() {}
    virtual void onDisable() {}
    virtual std::string suffix() const { return {}; }
    bool isEnabled() const { return enabled; }
    void setEnabled(bool v) { if(v!=enabled) { enabled=v; if(v) onEnable(); else onDisable(); } }
    Properties::PropDesc subBool(const char* name,bool v) {
        auto p=std::make_unique<Properties::BoolProperty>(v); auto ptr=p.get(); owned.push_back(std::move(p)); return {name,ptr};
    }
    Properties::BoolProperty& addBool(const char* name,bool v,std::vector<Properties::PropDesc> children={}) {
        auto row=subBool(name,v);
        rows.emplace_back(name,row.getProp(),std::move(children));
        return *static_cast<Properties::BoolProperty*>(rows.back().getProp());
    }
    Properties::PropDesc subNumber(const char* name,double min,double max,double step,double v,std::function<std::string(double)> fmt) {
        auto p=std::make_unique<Properties::NumberProp>(min,max,step,v,std::move(fmt)); auto ptr=p.get(); owned.push_back(std::move(p)); return {name,ptr};
    }
    void addSettings(std::vector<Properties::PropDesc> v) { for(auto& row:v) rows.push_back(std::move(row)); }
    Properties::AbstractProperty* Find(const char* name) {
        for(auto& row:rows) {
            if(std::string(row.getName())==name) return row.getProp();
            for(auto& sub:row.getSubProps()) if(std::string(sub.getName())==name) return sub.getProp();
        }
        assert(false); return nullptr;
    }
    void Bool(const char* name,bool v) { static_cast<Properties::BoolProperty*>(Find(name))->set(v); }
    void Number(const char* name,double v) { static_cast<Properties::NumberProp*>(Find(name))->set(v); }
};
}
