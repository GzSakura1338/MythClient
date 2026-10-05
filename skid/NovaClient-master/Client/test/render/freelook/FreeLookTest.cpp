#include "minecraft/modules/render/freelook/FreeLookLogic.h"
#include "minecraft/java/api/class_edit.h"
#include "minecraft/hooks/Edits.h"
#include "../../bytecode/audit.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace nova::module::freelook;
struct IO {
    int view=2, changes=0;
    float fov=83, yaw=47, pitch=-12;
    bool valid=true;
    bool Capture(int& v,float& f,float& y,float& p) {
        v=view;f=fov;y=yaw;p=pitch;return valid;
    }
    bool SetView(int v) {view=v;++changes;return true;}
    bool SetFov(float v) {fov=v;return true;}
};
void Logic() {
    State state;Settings s;IO io;
    Input in{false,true,true,false};
    state.Tick(s,in,io);assert(!state.active&&io.view==2);
    s.enabled=true;state.Tick(s,in,io);in.down=true;state.Tick(s,in,io);
    assert(state.active&&io.view==1&&state.yaw==47&&state.pitch==-12);
    state.Turn(s,20,40);assert(state.yaw==50&&state.pitch==-6);
    assert(io.yaw==47&&io.pitch==-12); // player orientation stays untouched
    in.down=false;state.Tick(s,in,io);assert(!state.active&&io.view==2);
    s.hold=false;s.customFov=true;s.fov=150;
    in.down=true;state.Tick(s,in,io);assert(state.active&&io.fov==150);
    in.down=false;state.Tick(s,in,io);assert(state.active);
    s.invertPitch=true;state.Turn(s,0,40);assert(state.pitch==-18);
    state.Turn(s,0,10000);assert(state.pitch==-90);
    s.lockPitch=false;state.Turn(s,0,1000);assert(state.pitch==-240);
    state.Turn(s,NAN,3);assert(state.pitch==-240);
    s.customFov=false;state.Tick(s,in,io);assert(io.fov==83);
    in.down=true;state.Tick(s,in,io);assert(!state.active&&io.view==2);
    in.down=false;state.Tick(s,in,io);in.down=true;state.Tick(s,in,io);
    in.screen=true;state.Tick(s,in,io);assert(state.active); // toggle can stay in menu
    s.hold=true;state.Tick(s,in,io);assert(!state.active&&io.view==2);
    in.screen=false;state.Tick(s,in,io);assert(!state.active); // no ghost press after GUI
    in.down=false;state.Tick(s,in,io);in.down=true;state.Tick(s,in,io);assert(state.active);
    in.focused=false;state.Tick(s,in,io);assert(!state.active);
    in.focused=true;state.Tick(s,in,io);assert(!state.active);
    in.down=false;state.Tick(s,in,io);in.down=true;state.Tick(s,in,io);
    s.enabled=false;state.Tick(s,in,io);assert(!state.active&&io.view==2&&io.fov==83);
    s.enabled=true;in.down=false;state.Tick(s,in,io);in.down=true;state.Tick(s,in,io);
    in.ready=false;state.Tick(s,in,io);assert(!state.active);
    in.ready=true;state.Tick(s,in,io);assert(!state.active);
    in.down=false;state.Tick(s,in,io);in.down=true;state.Tick(s,in,io);
    s.key=0x42;state.Tick(s,in,io);assert(!state.active); // rebind clears old hold
    in.down=false;state.Tick(s,in,io);io.valid=false;in.down=true;state.Tick(s,in,io);
    assert(!state.active&&io.view==2); // failed capture does not change camera
    std::cout<<"PASS: Raven hold/toggle, sensitivity-scaled rotation, inversion, clamp, FOV and player-rotation isolation; menu/focus/rebind/disable/world cleanup\n";
}
int main(int argc,char** argv) {
    assert(argc==3);Logic();
    const std::string profile=argv[1];
    nova::hook::SelectLunarEdits(profile=="lunar");nova::hook::SelectMinecraft263Edits(profile=="263");
    int count=0,checked=0;auto edits=nova::hook::AllEdits(count);
    for(int c=0;c<count;++c) {
        const auto& e=edits[c];bool relevant=false;
        for(int i=0;i<e.count;++i)relevant|=std::string(e.redirects[i].hook).starts_with("freeLook");
        for(int i=0;i<e.appendCount;++i)relevant|=std::string(e.appends[i].hook).starts_with("freeLook");
        if(!relevant)continue;
        const auto path=std::filesystem::path(argv[2])/(std::string(e.internalName)+".class");
        std::ifstream in(path,std::ios::binary);std::vector<u1> bytes{std::istreambuf_iterator<char>(in),{}};
        assert(!bytes.empty());ClassFileEditor editor(bytes.data(),bytes.size());
        for(int i=0;i<e.count;++i) {
            const auto& r=e.redirects[i];if(!std::string(r.hook).starts_with("freeLook"))continue;
            if(!editor.redirectCalls(r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc,
                "nova/test/FreeLookBridge",r.hook,r.hookDesc,r.opcode,r.limit,r.ordinal)) {
                std::cerr<<"FAIL: "<<e.internalName<<" "<<r.method<<" "<<r.hook<<"\n";return 1;
            }
            ++checked;
        }
        for(int i=0;i<e.appendCount;++i) {
            const auto& a=e.appends[i];
            // Exercise pre-existing FOV return hooks on the same class too.
            const auto ref=editor.addMethodRef("nova/test/FreeLookBridge",a.hook,a.hookDesc);
            const auto code=nova::hook::AppendCode(ref,a.loads,a.loadCount);
            assert(a.atHead||a.allVoidReturns||a.allValueReturns);
            const bool ok=a.atHead ? editor.prependToMethodCode(a.method,a.methodDesc,code,a.loadCount) : a.allVoidReturns ? editor.appendBeforeVoidReturns(a.method,a.methodDesc,code,a.loadCount)
                : editor.appendBeforeValueReturns(a.method,a.methodDesc,code,a.loadCount);
            assert(ok);++checked;
        }
        std::string error;
        if(!audit::AuditClass(editor.serialize(),error)) {std::cerr<<error;return 2;}
    }
    assert(checked>0);
    std::cout<<"PASS: "<<profile<<" "<<checked<<" actual FreeLook/FOV redirects/appends; branch and stack-map structure\n";
}
