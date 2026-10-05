#include <cassert>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>
#include "minecraft/utils/rotation/Vec3.h"
namespace rotation = nova::rotation;
namespace event {
struct ClickEvent {
    bool cancelled = false;
    bool IsCancelled() const { return cancelled; }
    void Cancel() { cancelled = true; }
};
}
namespace java {
struct BlockPos { int x=0,y=0,z=0; } used;
struct Box { double minX,minY,minZ,maxX,maxY,maxZ; };
struct ChestBox : Box { int x,y,z; };
std::vector<ChestBox> chests;
bool key=true, valid=true, loaded=true, useResult=true;
int uses=0, swings=0, facing=-1;
rotation::Vec3 eye{.5,.5,0}, clicked;
float cameraYaw=0;
struct EntityHandle {
    bool Valid() const { return valid; }
    bool EyePosition(float partial, rotation::Vec3& out) const {
        assert(partial==1); out=eye; return valid;
    }
};
EntityHandle PlayerEntity() { return {}; }
bool UseKeyDown() { return key; }
bool PlayerRotation(float& yaw,float& pitch) { yaw=cameraYaw; pitch=0; return valid; }
bool Vec3DirectionFromRotation(float,float yaw,rotation::Vec3& out) {
    const double rad=yaw*3.141592653589793/180;
    out={-std::sin(rad),0,std::cos(rad)}; return true;
}
bool LoadedChestBoxes(std::vector<ChestBox>& out) { out=chests; return loaded; }
bool BoxClip(const Box& b,const rotation::Vec3& from,const rotation::Vec3& to,bool& hit,rotation::Vec3& out) {
    double enter=0,leave=1;
    const double start[]{from.x,from.y,from.z}, end[]{to.x,to.y,to.z};
    const double lo[]{b.minX,b.minY,b.minZ}, hi[]{b.maxX,b.maxY,b.maxZ};
    hit=false;
    for(int i=0;i<3;++i) {
        const double delta=end[i]-start[i];
        if(std::abs(delta)<1e-12) { if(start[i]<lo[i]||start[i]>hi[i]) return true; continue; }
        double a=(lo[i]-start[i])/delta, c=(hi[i]-start[i])/delta;
        if(a>c)std::swap(a,c);
        enter=std::max(enter,a); leave=std::min(leave,c);
        if(enter>leave)return true;
    }
    hit=true; out=from.Add((to.x-from.x)*enter,(to.y-from.y)*enter,(to.z-from.z)*enter);return true;
}
bool UseItemOnBlock(BlockPos pos,int face,rotation::Vec3 hit) { used=pos;facing=face;clicked=hit;++uses;return useResult; }
bool SwingMainHand() { ++swings;return true; }
}
struct Fixture {
    struct Mode { int value=0; int get() const { return value; } } mode;
    Mode* mode_ = &mode;
    bool enabled=true;
    bool isEnabled() const { return enabled; }
    // @METHODS@
};
int main() {
    Fixture f;
    auto click=[&] { event::ClickEvent e; f.OnClick(e); return e.cancelled; };
    java::chests={{{.0625,0,3.0625,.9375,.875,3.9375},0,0,3},
                  {{.0625,0,2.0625,.9375,.875,2.9375},0,0,2}};
    assert(click() && java::used.z==2 && java::uses==1 && java::swings==1);
    assert(java::facing==1 && java::clicked.z==2.0625 && java::clicked.y==.5);
    std::reverse(java::chests.begin(),java::chests.end());
    assert(click() && java::used.z==2); // nearest, independent of traversal order
    java::useResult=false;
    assert(click() && java::uses==3 && java::swings==3); // match source result handling
    java::useResult=true;
    java::chests={{{.0625,0,4.5001,.9375,.875,5.4375},0,0,5}};
    assert(!click()); // 4.5 reach, no creative/attribute extension
    java::chests={{{2.0625,0,2.0625,2.9375,.875,2.9375},2,0,2}};
    assert(!click()); // nearby but not under the crosshair
    java::chests={{{.0625,0,-3.9375,.9375,.875,-3.0625},0,0,-4}};
    assert(!click()); // behind the player
    java::cameraYaw=180;
    assert(click() && java::used.z==-4);
    java::cameraYaw=0;
    // The merged double chest retains the right-half owner even when aiming at
    // the other half; hit coordinates remain the exact outline intersection.
    java::chests={{{.0625,0,2.0625,1.9375,.875,2.9375},1,0,2}};
    assert(click() && java::used.x==1 && java::clicked.x==.5);
    const int before=java::uses;
    f.mode.value=1; assert(!click()); f.mode.value=0;
    f.enabled=false; assert(!click()); f.enabled=true;
    java::key=false; assert(!click()); java::key=true;
    java::valid=false; assert(!click()); java::valid=true;
    java::loaded=false; assert(!click()); java::loaded=true;
    event::ClickEvent cancelled{true}; f.OnClick(cancelled);
    assert(java::uses==before && java::swings==before);
    java::chests.clear(); assert(!click());
    std::cout<<"PASS: GhostHand nearest ray target, exact hit/UP face, 4.5 reach, double chest owner, result handling and input guards\n";
}
