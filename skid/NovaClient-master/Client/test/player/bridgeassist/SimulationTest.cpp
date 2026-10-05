#include <cassert>
#include <cstdio>
#include "minecraft/modules/player/bridgeassist/BridgeSimulation.h"
using namespace nova::module::bridgeassist;

struct World {
    std::vector<Box> blocks{{-10,0,-10,10,1,10}};
    bool ok=true;
    float friction=.6f;
    bool Good() { return ok; }
    bool Water(Simulation&) { return ok; }
    bool NormalCube(int,int,int) { return false; }
    float Slipperiness(int,int,int) { return friction; }
    bool Collisions(const Box& area,std::vector<Box>& out) {
        out.clear();
        for (const auto& b:blocks)
            if(b.maxX>area.minX && b.minX<area.maxX && b.maxY>area.minY && b.minY<area.maxY && b.maxZ>area.minZ && b.minZ<area.maxZ)
                out.push_back(b);
        return ok;
    }
};
Simulation Player(double x=.5,double y=1,double z=.5) {
    Simulation s; s.box={x-.3,y,z-.3,x+.3,y+1.8,z+.3};
    s.x=x; s.y=y; s.z=z; s.ground=true; s.motion.y=-.08;
    return s;
}
void Near(double a,double b,double tolerance=1e-6) { assert(std::abs(a-b)<tolerance); }
int main() {
    World w; Box box; double offset;
    auto s=Player(); s.forward=1;
    assert(Predict(s,w,box)); Near(box.minY,1); Near((box.minZ+box.maxZ)/2,.598);
    assert(EdgeOffset(box,w,offset)); Near(offset,0);
    // Raven copies the original movement attribute, then changes its local sprint flag.
    s=Player(); s.forward=1; s.sprint=true; s.speed=.13f;
    assert(Predict(s,w,box)); Near((box.minZ+box.maxZ)/2,.6274);
    s=Player(); s.jump=true; s.forward=1; s.sprint=true;
    assert(Predict(s,w,box)); Near(box.minY,1.42); Near((box.minZ+box.maxZ)/2,.798);
    s.jumpCooldown=2; assert(Predict(s,w,box)); Near(box.minY,1);
    s.jumpCooldown=1; assert(Predict(s,w,box)); Near(box.minY,1.42);
    s=Player(); s.jump=true; s.jumpPower=double(.42f)+.2f;
    assert(Predict(s,w,box)); Near(box.minY,1.62);
    w.blocks={{0,0,0,1,1,1}}; s=Player(.95); s.motion.x=.2;
    assert(Predict(s,w,box) && EdgeOffset(box,w,offset)); Near(offset,.15);
    s.motion.x=.5; assert(Predict(s,w,box) && EdgeOffset(box,w,offset)); assert(std::isnan(offset));
    w.blocks={{-1,0,-1,0,1,0}}; s=Player(-.05,1,-.05); s.motion={.2,-.08,.12};
    assert(Predict(s,w,box) && EdgeOffset(box,w,offset)); Near(offset,.15); // Chebyshev distance, not Euclidean.
    w.blocks={{0,0,0,1,.5,1}}; s=Player(.5,.5);
    assert(Predict(s,w,box) && EdgeOffset(box,w,offset)); Near(offset,0); Near(box.minY,.5);
    w.blocks={{.4375,0,0,.5625,1.5,1}}; s=Player(.5,1.5);
    assert(Predict(s,w,box) && EdgeOffset(box,w,offset)); Near(offset,0); // Fence/narrow collision shape.
    s.motion.x=.1; assert(Predict(s,w,box) && EdgeOffset(box,w,offset)); Near(offset,.0375);
    w.blocks={{-10,0,-10,10,1,10},{1,1,0,2,1.5,1}}; s=Player(); s.motion.x=.5;
    assert(Predict(s,w,box)); Near(box.minY,1.5); Near((box.minX+box.maxX)/2,1);
    w.blocks[1].maxY=2;
    assert(Predict(s,w,box)); Near(box.minY,1); Near(box.maxX,1); // Cannot climb a full block.
    w.blocks[1].maxY=1.5; w.blocks.push_back({0,2.9,0,2,3,1});
    assert(Predict(s,w,box)); Near(box.minY,1); Near(box.maxX,1); // Low ceiling blocks the step.
    w.blocks={{-10,0,-10,10,1,10}}; s=Player(); s.motion.x=.4; s.web=true;
    assert(Predict(s,w,box)); Near((box.minX+box.maxX)/2,.6);
    s=Player(); s.motion.x=.8; s.ladder=true;
    assert(Predict(s,w,box)); Near((box.minX+box.maxX)/2,.65);
    s=Player(); s.water=true; s.jump=true; s.forward=1;
    assert(Predict(s,w,box)); Near((box.minZ+box.maxZ)/2,.5196); Near(box.minY,1);
    s=Player(); s.usingItem=true; s.forward=1;
    assert(Predict(s,w,box)); Near((box.minZ+box.maxZ)/2,.5196);
    s=Player(); s.riding=true; s.motion.x=2;
    assert(Predict(s,w,box)); Near((box.minX+box.maxX)/2,.5);
    s=Player(); w.ok=false; assert(!Predict(s,w,box)); w.ok=true;
    s.motion.x=std::numeric_limits<double>::quiet_NaN(); assert(!Predict(s,w,box));
    std::puts("BridgeAssist simulation: acceleration, sprint jump, cooldown, slabs, fence, steps, ceilings, fluids, web and failures passed");
}
