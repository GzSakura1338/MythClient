#include "minecraft/java/api/class_edit.h"
#include "minecraft/hooks/Edits.h"
#include "../../bytecode/audit.h"
#include <fstream>
#include <iostream>
#include <string>
#include <set>
int main(int argc,char** argv) {
    if(argc!=4)return 2;
    const std::string profile=argv[1],owner=argv[2];
    nova::hook::SelectLunarEdits(profile=="lunar");
    if(profile=="263")nova::hook::SelectMinecraft263Edits(true);
    const auto* edits=nova::hook::EditsFor(owner.c_str());
    std::ifstream file(argv[3],std::ios::binary);
    std::vector<u1> bytes{std::istreambuf_iterator<char>(file),{}};
    if(!edits||bytes.empty())return 3;
    ClassFileEditor editor(bytes.data(),bytes.size());
    int count=0;
    const std::set<std::string> hooks{"attackMotionX","attackMotionZ","attackSprinting","attackVelocity","attackSprint"};
    for(int i=0;i<edits->count;++i){
        const auto& r=edits->redirects[i];
        if(std::string(r.hook)=="localAttack"){std::cerr<<"Removed Aura hook remains";return 6;}
        if(!hooks.contains(r.hook))continue;
        if(!editor.redirectCalls(r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc,
            "nova/test/KeepSprintBridge",r.hook,r.hookDesc,r.opcode,r.limit,r.ordinal)) {
            std::cerr<<"FAIL "<<profile<<" "<<r.method<<" "<<r.hook<<"\n";return 4;
        }
        ++count;
    }
    const int expected=owner.find("multiplayer")!=std::string::npos?0:profile=="lunar"?3:2;
    std::string error;
    if(count!=expected||!audit::AuditClass(editor.serialize(),error)){std::cerr<<count<<" "<<error;return 5;}
    std::cout<<"PASS: "<<profile<<" "<<owner<<" "<<count<<" real bytecode hooks / stack-map audit\n";
}
