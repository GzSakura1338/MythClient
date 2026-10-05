#include "minecraft/modules/combat/displace/DisplaceLogic.h"
#include <cassert>
#include <cstdio>
#include <functional>
#include <set>
using namespace nova::module::displace;
struct IO {
    bool ready=true,kb=false,wl=false,attacking=true,moving=true;
    float base=30;int clicks=0;
    std::set<int> ids{1,2};
    std::optional<Target> target=Target{1,{0,64,0},{-.3,64,-.3,.3,65.8,.3},.6,1.62};
    std::function<bool(int,int,int)> air=[](int,int,int){return false;};
    std::function<bool(const Box&)> collision=[](const Box&){return false;};
    bool Ready(){return ready;}
    bool Trackable(int id){return ids.contains(id);}
    bool Knockback(){return kb;}
    bool Whitelist(){return wl;}
    bool Attacking(){return attacking;}
    bool Movement(){return moving;}
    auto Closest(bool){return target;}
    void AttackKeyTick(){++clicks;}
    float ServerYaw(){return base;}
    Vec3 SelfPosition(){return {0,64,-2};}
    Vec3 Eye(){return {0,65.62,-2};}
    bool Air(int x,int y,int z){return air(x,y,z);}
    bool Collision(int,const Box& b){return collision(b);}
};
// Independent transcription of Raven's non-render onClientRotation delay branch,
// used over all delay settings, target switches and condition transitions.
struct Reference {
    bool active=false,displace=false,previous=false,compensate=false;
    int tick=0,clicks=0;std::unordered_map<int,int> windows;
    void Clear(){active=displace=previous=compensate=false;}
    void Rotation(IO& io,const Settings& s) {
        if(!io.ready){Clear();return;}
        ++tick;std::erase_if(windows,[&](auto p){return !io.ids.contains(p.first);});
        if(s.requireKnockback||s.whitelist){
            bool kbPass=!s.requireKnockback||io.kb;
            bool wlPass=!s.whitelist||io.wl;
            if(!(kbPass||wlPass)){Clear();return;}
        }
        auto target=io.attacking?io.target:std::optional<Target>{};
        active=target.has_value()&&(io.kb||io.moving);
        if(!active){Clear();return;}
        displace=!displace;
        if(displace){
            int id=target->id;auto it=windows.find(id);bool allowed=true;
            if(it==windows.end()||tick-it->second>=10)windows[id]=tick;
            else if(s.delay>0)allowed=tick-it->second>=int(std::ceil(s.delay/50.));
            if(!allowed){displace=compensate=previous=false;return;}
        }
        if(!displace&&previous)++clicks;
        previous=displace;
    }
};
int main(){
    Settings settings;State state;IO io;
    state.Rotation(io,settings);assert(state.active&&state.displacing&&*state.yaw==-60);
    float forward=0,strafe=0;state.Input(forward,strafe,true);
    assert(forward==1&&state.compensate);
    state.Rotation(io,settings);assert(!state.displacing&&io.clicks==1);
    state.Input(forward,strafe,true);assert(strafe==-1&&!state.compensate);
    settings.left=false;state.Rotation(io,settings);assert(*state.yaw==120);
    state.Input(forward,strafe,true);state.Rotation(io,settings);state.Input(forward,strafe,true);assert(strafe==1);
    io.kb=true;state.Rotation(io,settings);forward=.3f;state.Input(forward,strafe,true);assert(forward==.3f);
    for(int delay=0;delay<=500;delay+=50){
        State s;Reference r;IO test;Settings cfg;cfg.delay=delay;
        for(int tick=0;tick<400;++tick){
            test.kb=tick%11==0;test.wl=tick%13==0;test.moving=tick%7!=0;test.attacking=tick%17!=0;
            cfg.requireKnockback=tick%9<4;cfg.whitelist=tick%9>=2;
            test.target->id=tick%23<12?1:2;
            if(tick%41==0)test.ids.erase(1);else test.ids.insert(1);
            test.ready=tick%61!=0;
            s.Rotation(test,cfg);r.Rotation(test,cfg);
            assert(s.active==r.active&&s.displacing==r.displace&&s.wasDisplacing==r.previous&&test.clicks==r.clicks);
        }
    }
    // Delays 50..500 do not suppress the first displacement of each window.
    state={};io=IO{};settings=Settings{};settings.delay=500;
    std::vector<int> active;
    for(int tick=1;tick<=22;++tick){state.Rotation(io,settings);if(state.displacing)active.push_back(tick);}
    assert((active==std::vector<int>{1,11,21}));
    // Eleven inclusive cells: y=floor(targetY)-1 down to start-10.
    int checked=0;io.air=[&](int,int,int){++checked;return true;};
    assert(VoidColumn(io,-.01,64,-.01)&&checked==11);
    io.air=[](int x,int y,int z){return !(x==-1&&y==53&&z==-1);};
    assert(!VoidColumn(io,-.01,64,-.01));
    checked=0;io.air=[&](int,int,int){++checked;return true;};assert(VoidColumn(io,0,0,0)&&checked==0);
    // Static mode chooses the first ring with void, dynamic evaluates all paths.
    state={};settings={};settings.findVoid=true;
    io.air=[](int x,int,int){return x>=1;};state.Rotation(io,settings);
    assert(state.active&&state.yaw&&*state.yaw<0&&*state.yaw> -45);
    settings.dynamic=true;state={};state.Rotation(io,settings);assert(state.active&&state.yaw&&*state.yaw<=0);
    // A wall at the first quarter-block invalidates the entire path.
    io.collision=[](const Box&){return true;};state={};state.Rotation(io,settings);assert(!state.active&&!state.yaw);
    io.collision=[](const Box&){return false;};io.air=[](int,int,int){return false;};
    state={};state.Rotation(io,settings);assert(!state.active);
    // Scan scoring: all three void lanes plus increasing center-run bonus.
    io.air=[](int,int,int){return true;};assert(std::abs(state.Score(io,*io.target,1,0)-288.6)<1e-9);
    float f=.3f,s=0;FixInput(f,s,90,0,true);assert(f==0&&s==.3f);
    f=0;s=0;FixInput(f,s,90,0,false);assert(f==0&&s==0);
    assert(std::abs(FixYaw(-179,179,.5f)-180.95f)<.001f);
    std::puts("PASS: Raven static/dynamic scans, walls, void depth, 4400 reference ticks, target windows, item OR, key ticks, movement and yaw quantization");
}
