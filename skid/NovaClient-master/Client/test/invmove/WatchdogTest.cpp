#include "minecraft/modules/movement/invmove/WatchdogState.h"
#include "minecraft/modules/movement/invmove/ModeState.h"
#include <cassert>
#include <cstdio>
using nova::module::invmove::WatchdogState;
int main(){
    nova::module::invmove::BufferState b;
    assert(!b.click(3)&&!b.click(3)&&!b.click(3));
    assert(b.holding(3)&&b.click(3)&&b.clicks==3);
    assert(!b.motion(3)); // first PreMotion only waits
    assert(b.motion(3)); // second PreMotion emits Amount then FIFO
    b.flushed=true;
    assert(!b.motion(3)&&!b.holding(3));
    assert(!b.click(3)&&b.clicks==4);
    assert(!b.motion(3)&&!b.flushed&&!b.waited);
    assert(!b.click(3)&&!b.click(3)&&b.holding(3));
    assert(!b.motion(3)&&b.motion(3)); // another independent cycle
    b={};assert(!b.holding(3)&&!b.motion(3));
    nova::module::invmove::GrimState grim;
    grim.clicks=2;for(int i=0;i<11;++i)assert(grim.update(9));
    assert(!grim.update(9));
    grim.clicks=1;assert(grim.update(9));
    grim.clicks=3;for(int i=0;i<12;++i)assert(grim.update(9));
    assert(!grim.update(9));
    WatchdogState s;
    s.Tick(100,true,false,true,false,1);
    assert(s.clicking&&s.BlockInput(100,1000,false,false,false,true));
    assert(s.BlockInput(101,1059,false,false,false,true));
    assert(s.BlockInput(102,1060,false,false,false,true));
    assert(!s.BlockInput(103,1061,false,false,false,true));
    s.Tick(104,true,false,true,false,1);
    assert(!s.BlockInput(104,1070,false,false,false,true)); // not per-click delay
    s.Tick(105,false,false,false,false,1);s.BlockInput(105,1080,false,false,false,true);
    assert(!s.clicking&&!s.delayPassed);
    s.Tick(110,false,false,false,true,1);
    s.Tick(114,false,true,false,false,1);assert(s.latency==4&&!s.confirmed);
    s.Tick(116,false,true,false,false,1);assert(!s.confirmed);
    s.Tick(117,false,true,false,false,1);assert(s.confirmed);
    assert(s.BlockInput(117,2000,true,false,false,true));
    assert(!s.BlockInput(120,2150,true,false,false,true));
    assert(s.BlockInput(120,2150,true,true,true,true));
    assert(s.BlockInput(120,2150,true,true,false,false));
    s.Tick(121,false,false,false,false,1);assert(!s.confirmed&&s.chestTick==-1);
    s={};s.Tick(10,false,false,false,true,1);s.Tick(51,false,false,false,false,1);assert(!s.awaiting&&s.latency==-1);
    s.Tick(60,false,true,false,false,1);assert(!s.confirmed);
    puts("PASS: Buffer Abuse cycles/reset, Grim countdown/extension, Watchdog delay, chest cadence, prediction and timeout");
}
