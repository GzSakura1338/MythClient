#include "minecraft/java/api/class_edit.h"
#include "minecraft/hooks/Edits.h"
#include "../../bytecode/audit.h"
#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>
int main(int argc,char** argv){
    if(argc!=3)return 2;
    nova::hook::SelectLunarEdits(std::string(argv[1])=="lunar");
    nova::hook::SelectLunar1710Edits(std::string(argv[1])=="1710");
    nova::hook::SelectMinecraft263Edits(std::string(argv[1])=="263");
    int count=0,checked=0;auto edits=nova::hook::AllEdits(count);
    for(int c=0;c<count;++c){
        const auto& e=edits[c]; bool relevant=false;
        for(int i=0;i<e.count;++i){const std::string name=e.redirects[i].hook;
            relevant|=name=="spoofSlot"||name=="visualPlayerHand"||name=="visualInventoryHand";}
        if(!relevant)continue;
        const auto path=std::filesystem::path(argv[2])/(std::string(e.internalName)+".class");
        std::ifstream in(path,std::ios::binary);
        std::vector<u1> bytes{std::istreambuf_iterator<char>(in),{}};
        if(bytes.empty()){std::cerr<<"Missing "<<path<<"\n";return 3;}
        ClassFileEditor editor(bytes.data(),bytes.size());
        for(int i=0;i<e.count;++i){const auto& r=e.redirects[i];
            const std::string name=r.hook;
            if(name!="spoofSlot"&&name!="visualPlayerHand"&&name!="visualInventoryHand")continue;
            if(!editor.redirectCalls(r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc,
                "nova/test/SlotBridge",r.hook,r.hookDesc,r.opcode,r.limit,r.ordinal)){
                std::cerr<<"FAIL "<<e.internalName<<" "<<r.method<<" "<<r.hook<<"\n";return 4;}
            ++checked;
        }
        std::string error;if(!audit::AuditClass(editor.serialize(),error)){std::cerr<<error;return 5;}
    }
    std::cout<<"PASS: "<<argv[1]<<" "<<checked<<" slot/render redirects and bytecode structure\n";
    return checked?0:6;
}
