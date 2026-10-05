#include "minecraft/java/api/class_edit.h"
#include "minecraft/hooks/Edits.h"
#include "../../bytecode/audit.h"
#include <cassert>
#include <fstream>
#include <iostream>
int main(){
    nova::hook::SelectLunarEdits(true);
    auto edits=nova::hook::EditsFor("net/minecraft/client/Minecraft");assert(edits);
    std::ifstream in("Minecraft.class",std::ios::binary);
    std::vector<u1> bytes{std::istreambuf_iterator<char>(in),{}};assert(!bytes.empty());
    ClassFileEditor editor(bytes.data(),bytes.size());int checked=0;
    for(int i=0;i<edits->count;++i){
        const auto& r=edits->redirects[i];
        if(std::string(r.method)!="runTick")continue;
        assert(editor.redirectCalls(r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc,
            "nova/test/DisplaceBridge",r.hook,r.hookDesc,r.opcode,r.limit,r.ordinal));
        if(std::string(r.hook)=="clientRotation")++checked;
    }
    for(int i=0;i<edits->appendCount;++i){
        const auto& a=edits->appends[i];if(std::string(a.method)!="runTick")continue;
        const auto ref=editor.addMethodRef("nova/test/DisplaceBridge",a.hook,a.hookDesc);
        auto code=nova::hook::AppendCode(ref,a.loads,a.loadCount);
        assert(a.atHead?editor.prependToMethodCode(a.method,a.methodDesc,code,a.loadCount):
            editor.appendBeforeVoidReturns(a.method,a.methodDesc,code,a.loadCount));
    }
    std::string error;
    if(checked!=1||!audit::AuditClass(editor.serialize(),error)){std::cerr<<error;return 1;}
    std::puts("PASS: Displace pre-mouse-over hook and all existing runTick edits rewrite the original Lunar Minecraft class; branch/stack-map audit");
}
