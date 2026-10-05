#include "minecraft/modules/player/blockin/BlockInLogic.h"
#include <cassert>
#include <cstdio>
#include <chrono>
using namespace nova::module::blockin;
struct FakeWorld {
    Vec3 eye{.5,1.62,.5};
    std::map<Pos,Cell> blocks;
    std::array<Stack,9> items{};
    std::optional<Vec3> enemy;
    std::map<Pos,int> reads;
    int itemReads=0,traces=0;
    double Random(){return .5;}
    Cell Read(Pos p){++reads[p];return blocks.contains(p)?blocks[p]:Cell{true,true,false};}
    Stack Item(int s){++itemReads;return items[s];}
    bool CanPlace(int,Pos p,int face) {
        auto goal=Offset(p,face);
        return blocks.contains(p) && !blocks.contains(goal) && goal!=Pos{0,0,0} && goal!=Pos{0,1,0};
    }
    std::optional<Vec3> ClosestPlayer(){return enemy;}
    std::optional<Hit> Trace(Rotation r) {
        ++traces;
        constexpr double rad=3.141592653589793/180;
        Vec3 d{-std::sin(r.yaw*rad)*std::cos(r.pitch*rad),-std::sin(r.pitch*rad),std::cos(r.yaw*rad)*std::cos(r.pitch*rad)};
        std::optional<Hit> best; double distance=4.5;
        for(auto& [p,c]:blocks) {
            double low=0,high=4.5; int face=-1;
            double origin[]{eye.x,eye.y,eye.z},dir[]{d.x,d.y,d.z}; int lo[]{p.x,p.y,p.z};
            const int nearFace[]{4,0,2},farFace[]{5,1,3};
            bool valid=true;
            for(int axis=0;axis<3;++axis) {
                if(std::abs(dir[axis])<1e-10) {if(origin[axis]<lo[axis]||origin[axis]>lo[axis]+1) valid=false;continue;}
                double a=(lo[axis]-origin[axis])/dir[axis],b=(lo[axis]+1-origin[axis])/dir[axis];
                int side=nearFace[axis];
                if(a>b){std::swap(a,b);side=farFace[axis];}
                if(a>low){low=a;face=side;} high=std::min(high,b);
                if(high<low) valid=false;
            }
            if(valid && face>=0 && low<distance) {distance=low;best=Hit{p,face,eye.Add(d.x*low,d.y*low,d.z*low)};}
        }
        return best;
    }
    void Ground(){for(int x=-4;x<=4;++x)for(int z=-4;z<=4;++z)blocks[{x,-1,z}]={};}
};
int main(){
    FakeWorld w; w.Ground(); w.items[1]={true,30}; w.items[7]={true,100}; w.items[8]={true,100};
    Planner planner(w,{.5,0,.5},w.eye,{0,80}); auto found=planner.Select();
    assert(found && found->adjacent && found->slot==8); // Equal scores scan hotbar right to left.
    assert(w.itemReads==9 && found->aim.hit.side==1);
    for(auto& [p,count]:w.reads) assert(count==1); // Snapshot caching, even with enemy/BFS retries.
    auto trace=w.Trace(found->aim.rotation); assert(trace && trace->block==found->aim.hit.block);
    auto previous=found->aim.rotation;
    assert(QueuePlacement(found->aim,previous,previous,25,trace));
    assert(!QueuePlacement(found->aim,{previous.yaw+26,previous.pitch},previous,25,trace));
    auto wrong=trace;wrong->side=0;assert(!QueuePlacement(found->aim,previous,previous,25,wrong));
    w.enemy=Vec3{3,0,.5}; Planner enemy(w,{.5,0,.5},w.eye,{0,80}); auto nearest=enemy.Select();
    assert(nearest && Offset(nearest->aim.hit.block,nearest->aim.hit.side).x==1);
    w.blocks[{0,3,0}]={}; Planner roof(w,{.5,0,.5},w.eye,{0,-80}); auto top=roof.Select();
    assert(top && top->aim.hit.block==Pos(0,3,0) && top->aim.hit.side==0 && top->slot==8);
    w.blocks.erase({0,3,0});w.blocks[{2,3,0}]={};
    Planner support(w,{.5,0,.5},w.eye,{-90,-50});auto remote=support.Select();
    assert(remote && !remote->adjacent && remote->slot==1);
    w.blocks[{2,3,0}].roofExcluded=true;
    Planner exclude(w,{.5,0,.5},w.eye,{0,80});auto side=exclude.Select();assert(side && side->adjacent);
    for(int f:{5,3,4,2}){w.blocks[Offset({0,0,0},f)]={};w.blocks[Offset({0,1,0},f)]={};}
    w.blocks[{0,2,0}]={};Planner sealed(w,{.5,0,.5},w.eye,{0,80});assert(sealed.Filled()==9 && !sealed.Select());
    FakeWorld empty;empty.items[0]={true,10};
    const auto begin=std::chrono::steady_clock::now();
    Planner noSupport(empty,{.5,0,.5},empty.eye,{0,0});assert(!noSupport.Select());
    for(auto& [p,count]:empty.reads) assert(count==1);
    double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
    auto sm=Smooth({0,0},{90,60},30,10,[]{return .5;});assert(sm.yaw==90 && sm.pitch==60);
    sm=Smooth({0,0},{90,60},10,10,[]{return .5;});assert(sm.yaw>0 && sm.yaw<20 && sm.pitch>0);
    auto fixed=Fix(sm,{0,0},.5);assert(std::abs(fixed.yaw/.15-std::round(fixed.yaw/.15))<.0001);
    std::printf("BlockIn: roof/side priority, strong/weak ties, enemy, rays, tolerance, fill, cache and smoothing passed; empty search %.2f ms\n",ms);
}
