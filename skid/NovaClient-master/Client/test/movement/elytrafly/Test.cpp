#include "minecraft/modules/movement/elytrafly/EquipLogic.h"
#include "minecraft/modules/movement/elytrafly/FireworkLogic.h"
#include <cassert>
#include <cstdio>
using namespace nova::module::elytrafly;
struct IO {
    int current = 0, syncs = 0;
    bool Selected(int& slot) { slot = current; return true; }
    bool Select(int slot) { current = slot; return true; }
    void Sync() { ++syncs; }
};
int main() {
    Trigger t;
    auto input = [&](bool down, bool hold=true, bool enabled=true, bool focused=true, bool screen=false, int key=-5) {
        t.Tick(enabled, key, hold, down, focused, screen);
    };
    input(false); input(true); assert(t.desired);
    input(true); assert(t.desired);
    input(false); assert(!t.desired);
    input(false, false); input(true, false); input(false, false); assert(t.desired);
    input(true, false); assert(!t.desired);
    input(false); input(true); input(false, true, true, true, true); assert(!t.desired);
    input(true, true, true, true, true); input(true); assert(!t.desired);
    input(false); input(true); input(true, true, false); assert(!t.desired);
    input(true); assert(!t.desired);
    input(false); input(true); input(true, true, true, false); assert(!t.desired);
    input(false); input(true); input(true, true, true, true, false, 65); assert(!t.desired);
    for (int old=0; old<9; ++old) for (int used=0; used<9; ++used) {
        Selection s; IO io; io.current=used;
        s.Capture(old, used);
        assert(io.current==used && io.syncs==0); // no same-tick restore
        s.Restore(io); assert(io.current==old && io.syncs==(old!=used));
        s.Restore(io); assert(io.syncs==(old!=used)); // only once
        s.Capture(old, used); io.current=(used+1)%9;
        s.Restore(io); assert(io.current==(used+1)%9); // preserve manual scrolling
        s.Capture(old, used); s.Reset(); io.current=used;
        s.Restore(io); assert(io.current==used); // no stale slot across worlds
    }
    std::puts("ElytraFly: Hold/Click gates and 81 hotbar handoff pairs passed");
    FireworkPolicy p;
    p.Observe(0,true,false,false,0); assert(!p.Ready(0));
    p.Observe(1,true,true,false,0); assert(p.Ready(1));
    p.Submitted(1);
    for(int tick=2;tick<11;++tick){p.Observe(tick,true,true,false,0);assert(!p.Ready(tick));}
    p.Observe(11,false,true,false,0);assert(!p.Ready(11));
    p.Observe(11,true,true,false,0);assert(p.Ready(11)); // fresh no-booster read after 500 ms
    p.Submitted(53);
    for(int tick=54;tick<90;++tick){p.Observe(tick,true,true,true,0);assert(!p.Ready(tick));}
    p.Observe(90,true,true,false,1.66);assert(!p.Ready(90)); // coast at speed
    p.Observe(91,true,true,false,1.60);assert(!p.Ready(91));
    p.Observe(92,true,true,false,1.44);assert(p.Ready(92));
    p.Observe(92,false,true,false,0);assert(!p.Ready(92)); // unknown telemetry
    p.Observe(93,true,true,true,0);assert(!p.Ready(93)); // manually launched booster
    p.Observe(94,true,false,false,0);assert(!p.Ready(94)); // landing
    p.Observe(95,true,true,false,2.5);assert(!p.Ready(95)); // dive: already faster than boost

    // A simplified straight-flight model using the decompiled boost recurrence.
    // Vary duration, server lifespan randomness and spawn lag; zero overlapping
    // launches and no click spam, while maintaining near single-rocket speed.
    int scenarios=0,totalOld=0,totalNew=0;
    for(int flight=0;flight<=3;++flight)for(int random=0;random<=11;++random)for(int latency:{0,3,10}){
        auto simulate=[&](bool old){
        FireworkPolicy policy;
        int spawn=-1,end=-1,last=-1000,shots=0,samples=0;double speed=0,sum=0;
        for(int tick=0;tick<1200;++tick){
            bool attached=spawn>=0&&tick>=spawn&&tick<end;
            policy.Observe(tick,true,true,attached,speed);
            const bool ready=old ? policy.valid && policy.gliding && !policy.boosting &&
                !policy.awaitingEntity && tick>=policy.nextAttempt && speed<1.62 : policy.Ready(tick);
            if(ready){
                assert(!attached && tick>=end && tick-last>=10);
                policy.Submitted(tick);last=tick;++shots;
                spawn=tick+latency;end=spawn+10*(flight+1)+random+1;
                attached=latency==0;
            }
            speed*=.99;
            if(attached)speed=.5*speed+.85;
            if(tick>100){sum+=speed;++samples;}
        }
        assert(shots<110);
        assert(sum/samples>1.40);
        return std::pair{shots,sum/samples};
        };
        const auto old=simulate(true),current=simulate(false);
        assert(current.first<old.first);
        assert(current.second>old.second*.94);
        totalOld+=old.first;totalNew+=current.first;
        ++scenarios;
    }
    std::printf("Auto Fire: pending/entity/speed gates and %d flight simulations passed\n",scenarios);
    std::printf("Straight-flight model rockets: old=%d new=%d saved=%.1f%%; average speed >=94%% of old\n",
        totalOld,totalNew,100.*(totalOld-totalNew)/totalOld);
}
