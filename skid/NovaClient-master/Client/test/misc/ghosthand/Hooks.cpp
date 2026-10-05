#include "minecraft/java/api/class_edit.h"
#include "minecraft/hooks/Edits.h"
#include "../../bytecode/audit.h"
#include <fstream>
#include <iostream>
#include <string>

int main(int argc,char** argv) {
    if(argc!=3)return 2;
    const std::string profile=argv[1];
    nova::hook::SelectLunarEdits(profile=="lunar");
    if(profile=="263")nova::hook::SelectMinecraft263Edits(true);
    const auto* e=nova::hook::EditsFor("net/minecraft/client/Minecraft");
    std::ifstream in(argv[2],std::ios::binary);
    std::vector<u1> bytes{std::istreambuf_iterator<char>(in),{}};
    if(!e||bytes.empty())return 3;
    ClassFileEditor editor(bytes.data(),bytes.size());
    const char* bridge="nova/test/GhostHandBridge";
    bool click=false;
    for(int i=0;i<e->count;++i) {
        const auto& r=e->redirects[i];
        if(!editor.redirectCalls(r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc,
            bridge,r.hook,r.hookDesc,r.opcode,r.limit,r.ordinal))return 4;
        if(profile=="lunar" && std::string(r.hook)=="blockVanillaUse")click=true;
    }
    for(int i=0;i<e->guardCount;++i) {
        const auto& g=e->guards[i];
        if(!editor.insertCancelGuard(g.method,g.methodDesc,bridge,g.hook,g.hookDesc,g.argCount)) {
            std::cerr<<"FAIL guard "<<profile<<" "<<g.method<<"\n";return 5;
        }
        if(std::string(g.hook)=="click")click=true;
    }
    for(int i=0;i<e->appendCount;++i) {
        const auto& a=e->appends[i];
        const auto ref=editor.addMethodRef(bridge,a.hook,a.hookDesc);
        const auto code=nova::hook::AppendCode(ref,a.loads,a.loadCount);
        if(!(a.atHead?editor.prependToMethodCode(a.method,a.methodDesc,code,a.loadCount):
             a.allVoidReturns?editor.appendBeforeVoidReturns(a.method,a.methodDesc,code,a.loadCount):
             editor.appendBeforeLastReturn(a.method,a.methodDesc,code,a.loadCount)))return 6;
    }
    std::string error;
    if(!click||!audit::AuditClass(editor.serialize(),error)){std::cerr<<error;return 7;}
    std::cout<<"PASS: "<<profile<<" Minecraft click guard and existing hooks, branch/stack-map structure\n";
}
