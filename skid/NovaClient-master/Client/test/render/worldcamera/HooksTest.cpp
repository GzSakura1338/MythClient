#include "minecraft/java/api/class_edit.h"
#include "minecraft/hooks/Edits.h"
#include "minecraft/compat/lunar12111/Edits.h"
#include "../../bytecode/audit.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

bool Relevant(const char* name) {
    const std::string s=name;
    return s=="worldCamera"||s=="resetWorldCamera"||s=="freeLookParticles"||s=="renderFrame"||s=="renderFrame17";
}
int main(int argc,char** argv) {
    if(argc!=3)return 2;
    const std::string profile=argv[1];
    if(profile=="lunar12111")nova::compat::lunar12111::SelectEdits();
    else if(profile=="easecation")nova::hook::SelectEaseCationEdits(true);
    else if(profile=="lunar189")nova::hook::SelectLunarEdits(true);
    else if(profile=="lunar1710")nova::hook::SelectLunar1710Edits(true);
    else if(profile!="forge")return 2;
    int count=0,rules=0,failed=0;
    const auto* edits=nova::hook::AllEdits(count);
    for(int c=0;c<count;++c) {
        const auto& e=edits[c];bool needed=false;
        for(int i=0;i<e.count;++i)needed|=Relevant(e.redirects[i].hook);
        for(int i=0;i<e.appendCount;++i)needed|=Relevant(e.appends[i].hook);
        if(!needed)continue;
        std::ifstream input(std::filesystem::path(argv[2])/(std::string(e.internalName)+".class"),std::ios::binary);
        std::vector<u1> bytes{std::istreambuf_iterator<char>(input),{}};
        if(bytes.empty()){std::cerr<<"missing "<<e.internalName<<'\n';++failed;continue;}
        ClassFileEditor editor(bytes.data(),bytes.size());
        for(int i=0;i<e.count;++i){const auto& r=e.redirects[i];if(!Relevant(r.hook))continue;++rules;
            if(!editor.redirectCalls(r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc,"nova/test/Camera",r.hook,r.hookDesc,r.opcode,r.limit,r.ordinal)){
                std::cerr<<e.internalName<<" "<<r.hook<<'\n';++failed;
            }
        }
        for(int i=0;i<e.appendCount;++i){const auto& a=e.appends[i];if(!Relevant(a.hook))continue;++rules;
            const auto ref=editor.addMethodRef("nova/test/Camera",a.hook,a.hookDesc);
            const auto code=nova::hook::AppendCode(ref,a.loads,a.loadCount);
            const bool ok=a.atHead?editor.prependToMethodCode(a.method,a.methodDesc,code,a.loadCount):
                a.allVoidReturns?editor.appendBeforeVoidReturns(a.method,a.methodDesc,code,a.loadCount):
                editor.appendBeforeLastReturn(a.method,a.methodDesc,code,a.loadCount);
            if(!ok){std::cerr<<e.internalName<<" "<<a.hook<<'\n';++failed;}
        }
        std::string error;if(!audit::AuditClass(editor.serialize(),error)){std::cerr<<error<<'\n';++failed;}
    }
    const int expected=(profile=="easecation"||profile=="lunar12111")?3:2;
    if(rules!=expected){std::cerr<<"missing camera rule: "<<rules<<'\n';++failed;}
    std::cout<<profile<<": "<<rules<<" camera hook rewrites, "<<failed<<" failures\n";
    return failed?1:0;
}
