#include "minecraft/modules/movement/sprint/MovementResume.h"
#include <array>
#include <cassert>
#include <cstdio>
using namespace nova::module::sprint;
int main() {
    MovementResume state;
    std::array<bool,4> physical{true,false,true,false}, game{};
    int writes=0;
    bool success=true;
    auto sync=[&] { ++writes; if(success) game=physical; return success; };
    state.Tick(true,Screen::None,sync); assert(writes==0);
    for(int i=0;i<20;++i) state.Tick(true,Screen::Container,sync);
    assert(writes==0 && state.pending); // No inventory movement.
    state.Tick(true,Screen::None,sync);
    assert(writes==1 && game==physical && !state.pending); // W/A held throughout close.
    state.Tick(true,Screen::None,sync); assert(writes==1); // Do not override normal movement each tick.
    state.Tick(true,Screen::Container,sync); physical={};
    state.Tick(true,Screen::None,sync); assert(writes==2 && game==physical); // Released inside GUI.
    state.Tick(true,Screen::Container,sync);
    state.Tick(true,Screen::Other,sync); state.Tick(true,Screen::None,sync);
    assert(writes==2); // Chat/settings or another screen cancels recovery.
    for(int i=0;i<3;++i) {
        state.Tick(true,Screen::Container,sync);
        state.Tick(false,Screen::None,sync); // Disabled, lost focus, ClickGUI, or missing world.
        state.Tick(true,Screen::None,sync); assert(writes==2);
    }
    state.Tick(true,Screen::Container,sync); state={};
    state.Tick(true,Screen::None,sync); assert(writes==2); // New world/player.
    state.Tick(true,Screen::Container,sync); success=false;
    state.Tick(true,Screen::None,sync); assert(writes==3 && state.pending);
    success=true; physical={false,true,false,true};
    state.Tick(true,Screen::None,sync); assert(writes==4 && !state.pending && game==physical);
    state.WorldChanged(); physical={true,false,false,false};
    for(int i=0;i<20;++i) state.Tick(true,Screen::Other,sync); // Joining/loading terrain.
    assert(writes==4 && state.worldPending);
    state.Tick(true,Screen::None,sync);
    assert(writes==5 && game==physical && !state.worldPending);
    state.Tick(true,Screen::None,sync); assert(writes==5); // Only once per join.
    state.WorldChanged(); // Respawn or direct server/world replacement.
    state.Tick(true,Screen::Other,sync); physical={}; // W released during loading.
    state.Tick(true,Screen::None,sync); assert(writes==6 && game==physical);
    state.WorldChanged(); success=false;
    state.Tick(true,Screen::None,sync); assert(writes==7 && state.worldPending);
    success=true; state.Tick(true,Screen::None,sync); assert(writes==8 && !state.worldPending);
    state.WorldChanged(); state.Tick(true,Screen::Container,sync);
    assert(writes==8 && state.worldPending); // Even on join, never move inside a container.
    state.Tick(true,Screen::None,sync); assert(writes==9);
    state.WorldChanged(); state.Tick(false,Screen::Other,sync);
    state.Tick(true,Screen::None,sync); assert(writes==9 && !state.worldPending);
    std::puts("PASS: container close, join/loading/respawn recovery, live held keys, one-shot sync, focus/disable gates and retry");
}
