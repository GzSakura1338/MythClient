#include "minecraft/modules/render/freecam/FreeCamLogic.h"
#include "minecraft/java/api/class_edit.h"
#include "minecraft/hooks/Edits.h"
#include "../../bytecode/audit.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>

using nova::module::freecam::Step;
using nova::rotation::Vec3;
bool Near(double a,double b){return std::abs(a-b)<1e-6;}
int main(int argc,char** argv){
    assert(argc==3);
    const Vec3 body{12,64,-3};
    auto p=Step(body,0,1,0,false,false,2.5);
    assert(Near(p.x,12)&&Near(p.y,64)&&Near(p.z,-2.4625));
    p=Step(body,90,1,0,false,false,2.5);
    assert(Near(p.x,11.4625)&&Near(p.z,-3));
    p=Step(body,0,1,1,true,false,2.5);
    assert(Near(p.x,12.5375)&&Near(p.y,64.499875)&&Near(p.z,-2.4625));
    p=Step(body,0,0,0,true,true,2.5);assert(Near(p.y,64));
    p=Step(body,0,0,0,false,true,2.5);assert(Near(p.y,63.500125));
    p=body;for(int i=0;i<20;++i)p=Step(p,0,1,0,false,false,2.5);
    assert(Near(p.z,7.75));
    assert(body.x==12&&body.y==64&&body.z==-3);
    const std::string profile=argv[1];
    nova::hook::SelectLunarEdits(profile=="lunar");
    nova::hook::SelectLunar1710Edits(profile=="1710");
    nova::hook::SelectMinecraft263Edits(profile=="263");
    nova::hook::SelectEaseCationEdits(profile=="easecation");
    int count=0,checked=0,tails=0;auto edits=nova::hook::AllEdits(count);
    for(int c=0;c<count;++c){
        const auto& e=edits[c];bool relevant=false;
        for(int i=0;i<e.count;++i)relevant|=std::string(e.redirects[i].hook).starts_with("freeCam");
        for(int i=0;i<e.appendCount;++i)relevant|=std::string(e.appends[i].hook)=="postGameTick";
        if(!relevant)continue;
        std::ifstream in(std::filesystem::path(argv[2])/(std::string(e.internalName)+".class"),std::ios::binary);
        std::vector<u1> bytes{std::istreambuf_iterator<char>(in),{}};
        assert(!bytes.empty());ClassFileEditor editor(bytes.data(),bytes.size());
        for(int i=0;i<e.count;++i){
            const auto& r=e.redirects[i];if(!std::string(r.hook).starts_with("freeCam"))continue;
            if(!editor.redirectCalls(r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc,
                "nova/test/Bridge",r.hook,r.hookDesc,r.opcode,r.limit,r.ordinal)){
                std::cerr<<"FAIL "<<e.internalName<<" "<<r.method<<" "<<r.hook<<'\n';return 1;
            }
            ++checked;
        }
        for(int i=0;i<e.appendCount;++i){
            const auto& a=e.appends[i];if(std::string(a.hook)!="postGameTick")continue;
            assert(!a.atHead&&a.allVoidReturns);
            auto ref=editor.addMethodRef("nova/test/Bridge",a.hook,a.hookDesc);
            auto code=nova::hook::AppendCode(ref,a.loads,a.loadCount);
            assert(editor.appendBeforeVoidReturns(a.method,a.methodDesc,code,a.loadCount));
            ++tails;
        }
        std::string error;if(!audit::AuditClass(editor.serialize(),error)){std::cerr<<error;return 2;}
    }
    assert(checked==(profile=="lunar"?3:2));
    assert(tails==1);
    std::cout<<"PASS "<<profile<<": Raven tick speed, additive diagonals, vertical factor, body isolation; "<<checked<<" attack/view hooks and stack maps\n";
}
