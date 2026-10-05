#include "minecraft/modules/player/blockin/BlockInLogic.h"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <utility>

namespace nova::module::blockin {
namespace mock {
bool enabled=true, lunar=true, screen=false, gui=false, foreground=true, held=true, mining=false;
bool physicalAttack=true, physicalUse=true, attack=false, use=false, success=true;
int player=1, world=2, slot=4, writes=0, clicks=3, places=0, swings=0, releases=0;
struct State { bool direct=true; Rotation rotations{10,20}; } state;
}
namespace compat { bool IsLegacyLunar() {return mock::lunar;} }
namespace clickgui { bool IsOpen() {return mock::gui;} }
namespace input { bool IsProcessForeground() {return mock::foreground;} }
namespace bind { bool IsPhysicalBindingDown(int) {return mock::held;} }
namespace breaker { bool OwnsMining() {return mock::mining;} }
namespace automlg { bool IsWorking() {return false;} }
namespace lunar {
bool PhysicalMouseDown(int) {return mock::physicalAttack;}
bool PhysicalUseDown() {return mock::physicalUse;}
void SetAttackDown(bool v) {mock::attack=v;}
void SetUseKeyDown(bool v) {mock::use=v;}
void ClearCombatClicks() {mock::clicks=0;}
}
namespace rotation {
const mock::State& State() {return mock::state;}
void ReleaseDirectRotations() {mock::state.direct=false;++mock::releases;}
}
namespace java {
struct EntityHandle {
    int id=0;
    EntityHandle()=default;
    explicit EntityHandle(int id):id(id){}
    EntityHandle(const EntityHandle&)=delete;
    EntityHandle& operator=(const EntityHandle&)=delete;
    EntityHandle(EntityHandle&& rhs):id(std::exchange(rhs.id,0)){}
    EntityHandle& operator=(EntityHandle&& rhs) {id=std::exchange(rhs.id,0);return *this;}
    explicit operator bool() const {return id!=0;}
    bool IsSame(const EntityHandle& rhs) const {return id==rhs.id;}
    void Reset() {id=0;}
};
bool ScreenOpen() {return mock::screen;}
EntityHandle PlayerEntity() {return EntityHandle(mock::player);}
EntityHandle LevelEntity() {return EntityHandle(mock::world);}
bool SelectedSlot(int& out) {out=mock::slot;return true;}
bool SetSelectedSlot(int v) {mock::slot=v;++mock::writes;return true;}
bool UseItemOnBlock(Pos,int,Vec3) {++mock::places;return mock::success;}
void SwingMainHand() {++mock::swings;}
}
std::atomic<bool> placing{false};
struct Runtime {
    struct Key { int keybind() {return 1;} } key_;
    bool isEnabled() {return mock::enabled;}
    java::EntityHandle player_,world_;
    int savedSlot_=-1,selectedSlot_=-1;
    bool ownsRotation_=false;
    Rotation applied_{10,20};
    std::optional<Hit> queued_;
    std::atomic<int> filled_{0};
    // @PRODUCTION_RUNTIME@
    void Arm() {
        mock::enabled=mock::lunar=mock::foreground=mock::held=true;
        mock::screen=mock::gui=mock::mining=false;
        mock::player=1;mock::world=2;mock::slot=4;
        mock::places=mock::swings=mock::writes=mock::releases=0;
        mock::clicks=3;mock::success=true;
        mock::attack=mock::use=false;
        mock::state={true,{10,20}};
        placing=true;player_=java::PlayerEntity();world_=java::LevelEntity();
        savedSlot_=1;selectedSlot_=4;ownsRotation_=true;
        queued_=Hit{{1,0,0},1,{1.5,1,.5}};
    }
};
}
int main() {
    using namespace nova::module::blockin;
    Runtime r;
    r.Arm();r.Place();assert(mock::places==1&&mock::swings==1&&!r.queued_&&mock::clicks==0);
    r.Place();assert(mock::places==1); // A queued placement can be consumed only once.
    r.Arm();mock::success=false;r.Place();assert(mock::places==1&&mock::swings==0);
    for(bool* gate:{&mock::enabled,&mock::lunar,&mock::foreground,&mock::held}) {
        r.Arm();*gate=false;r.Place();
        assert(!placing&&!r.queued_&&mock::places==0&&mock::slot==1&&mock::releases==1);
    }
    for(bool* gate:{&mock::screen,&mock::gui,&mock::mining}) {
        r.Arm();*gate=true;r.Place();assert(!placing&&!r.queued_&&mock::places==0);
    }
    r.Arm();mock::world=3;r.Place();assert(!placing&&mock::places==0&&mock::writes==0&&!mock::attack&&!mock::use);
    r.Arm();mock::player=0;r.Place();assert(!placing&&mock::places==0&&mock::writes==0);
    r.Arm();mock::slot=7;r.Place();assert(!placing&&mock::places==0&&mock::slot==7);
    r.Arm();mock::state.rotations.yaw=15;r.Place();
    assert(!r.queued_&&mock::places==0);r.Reset();assert(mock::releases==0&&mock::state.direct);
    r.Arm();r.queued_.reset();r.Place();assert(mock::clicks==0&&mock::places==0); // Clear input even while turning.
    r.Arm();r.Reset();assert(mock::slot==1&&mock::attack&&mock::use&&mock::releases==1);
    r.Reset();assert(mock::writes==1&&mock::releases==1); // Inactive cleanup has no side effects.
    r.Arm();mock::screen=true;r.Reset();assert(!mock::attack&&!mock::use);
    std::puts("BlockIn runtime: hold gates, one-shot queue, world/slot changes, rotation ownership, input and cleanup passed");
}
