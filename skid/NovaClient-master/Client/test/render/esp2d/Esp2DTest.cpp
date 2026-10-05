#include "minecraft/modules/render/esp2d/Esp2DLogic.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace nova::module::esp2d;
struct Command { double x,y,w,h; Value first,second; bool gradient,vertical; };
struct Recorder {
    std::vector<Command> commands;
    void Rect(double x,double y,double w,double h,Value c) {commands.push_back({x,y,w,h,c,c,false,false});}
    void Gradient(double x,double y,double w,double h,Value a,Value b,bool vertical) {commands.push_back({x,y,w,h,a,b,true,vertical});}
};
bool Near(double a,double b) {return std::abs(a-b)<.0001;}
int main() {
    constexpr Value first=0xFFFF7D66,second=0xFFFFEAE5;
    const Bounds bounds{100,50,140,130};
    Recorder draw;
    Render(draw,bounds,BoxMode::Normal,HealthMode::None,20,20,0,false,first,second);
    assert(draw.commands.size()==8);
    assert(Near(draw.commands[1].x,99.5)&&Near(draw.commands[1].w,41.5));
    assert(draw.commands[4].gradient&&!draw.commands[4].vertical&&Near(draw.commands[4].w,.5));
    draw.commands.clear();
    Render(draw,bounds,BoxMode::CS,HealthMode::None,20,20,0,false,first,second);
    assert(draw.commands.size()==16);
    assert(Near(draw.commands[10].w,10)&&Near(draw.commands[8].h,20)); // Quarter-length corners.
    draw.commands.clear();
    Render(draw,bounds,BoxMode::None,HealthMode::Health,20,20,0,false,first,second);
    assert(draw.commands.size()==2&&draw.commands[1].first==0xFF00FF00);
    assert(Near(draw.commands[1].y,50)&&Near(draw.commands[1].h,80.5));
    draw.commands.clear();
    Render(draw,bounds,BoxMode::None,HealthMode::Health,0,20,0,false,first,second);
    assert(draw.commands[1].first==0xFF0000FF);
    draw.commands.clear();
    Render(draw,bounds,BoxMode::None,HealthMode::Standard,10,20,0,false,first,second);
    assert(draw.commands.size()==2&&draw.commands[1].first==first&&Near(draw.commands[1].y,89));
    draw.commands.clear();
    Render(draw,bounds,BoxMode::None,HealthMode::Gradient,10,20,10,true,first,second);
    assert(draw.commands.size()==4&&draw.commands[1].vertical);
    assert(!draw.commands[3].vertical&&Near(draw.commands[3].y,132)&&Near(draw.commands[3].w,20.25));
    draw.commands.clear();
    Render(draw,bounds,BoxMode::None,HealthMode::Health,10,0,0,false,first,second);
    assert(draw.commands.empty());
    Render(draw,bounds,BoxMode::None,HealthMode::None,20,20,0,true,first,second);
    assert(draw.commands.empty());
    Render(draw,bounds,BoxMode::None,HealthMode::None,20,20,100,true,first,second);
    assert(Near(draw.commands.back().w,40.5)); // Modded armor cannot overflow the box.

    Targets t;
    // Modern squid used to fall outside every recognized group. Unknown living
    // entities must not sneak through a Player-only selection.
    assert(!Accept(t,false,false,false,false,false));
    assert(Accept(t,true,false,false,false,true));
    assert(!Accept(t,false,true,false,false,false));
    assert(!Accept(t,false,false,true,false,false));
    assert(!Accept(t,true,false,false,true,false));
    t.invisibles=true;assert(Accept(t,true,false,false,true,false));
    t.teammates=false;assert(!Accept(t,true,false,false,false,true));
    t.players=false;assert(!Accept(t,true,false,false,false,false));
    t.animals=t.mobs=true;assert(Accept(t,false,true,false,false,false)&&Accept(t,false,false,true,false,false));
    assert(!Accept(t,false,false,false,false,false));
    t={false,true,false,false,true};
    assert(!Accept(t,true,false,false,false,false));
    assert(!Accept(t,false,true,false,false,false));
    assert(!Accept(t,false,false,true,false,false));
    assert(!Accept(t,false,false,false,true,false));

    const auto project=[](Vec3 p,double& x,double& y) {
        if(p.z<.05) return false;
        x=400+p.x/p.z*400;y=300-p.y/p.z*400;return true;
    };
    const Vec3 current{2,0,10},previous{0,0,10};
    auto a=ProjectBounds(current,previous,.6,1.8,false,0,800,600,project);
    auto b=ProjectBounds(current,previous,.6,1.8,false,.5,800,600,project);
    auto c=ProjectBounds(current,previous,.6,1.8,false,1,800,600,project);
    assert(a&&b&&c&&a->left<b->left&&b->left<c->left);
    assert(Near(a->left,400-.37/9.63*400)&&Near(a->top,300-2.01/9.63*400));
    auto crouched=ProjectBounds(current,previous,.6,1.8,true,0,800,600,project);
    assert(crouched&&crouched->top>a->top&&Near(crouched->bottom,a->bottom));
    assert(!ProjectBounds({0,0,-10},{0,0,-10},.6,1.8,false,0,800,600,project));
    assert(!ProjectBounds({100,0,10},{100,0,10},.6,1.8,false,0,800,600,project));
    assert(!ProjectBounds(current,previous,.6,1.8,false,std::numeric_limits<float>::quiet_NaN(),800,600,project));
    auto far=ProjectBounds({0,0,200},{0,0,200},.6,1.8,false,0,800,600,project);
    assert(far&&far->right>far->left);
    std::puts("2DESP: box/corner geometry, health colors, armor, target filters, interpolation and visibility passed");
}
