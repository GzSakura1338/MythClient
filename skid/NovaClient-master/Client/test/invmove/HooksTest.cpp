#include "minecraft/java/api/class_edit.h"
#include "minecraft/hooks/Edits.h"
#include "../bytecode/audit.h"
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cstring>
int main(int argc,char** argv){
    if(argc!=2)return 2;
    nova::hook::SelectLunarEdits(true);
    int checks=0;
    for(auto name:{"net/minecraft/entity/Entity","net/minecraft/entity/EntityLivingBase"}){
        std::ifstream in(std::filesystem::path(argv[1])/(std::string(name)+".class"),std::ios::binary);
        std::vector<u1> bytes{std::istreambuf_iterator<char>(in),{}};
        if(bytes.empty())return 3;
        ClassFileEditor editor(bytes.data(),bytes.size());
        const auto* e=nova::hook::EditsFor(name);
        for(int i=0;i<e->count;++i){const auto& r=e->redirects[i];
            if(std::strncmp(r.hook,"inventory",9)!=0)continue;
            if(!editor.redirectCalls(r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc,"nova/NativeBridge",r.hook,r.hookDesc,r.opcode,r.limit,r.ordinal))return 4;
            ++checks;
        }
        for(int i=0;i<e->guardCount;++i){const auto& g=e->guards[i];
            if(std::strncmp(g.hook,"inventory",9)!=0)continue;
            if(!editor.insertCancelGuard(g.method,g.methodDesc,"nova/NativeBridge",g.hook,g.hookDesc,g.argCount,g.entryReturn))return 5;
            ++checks;
        }
        std::string error;
        if(!audit::AuditClass(editor.serialize(),error)){std::printf("FAIL %s: %s\n",name,error.c_str());return 6;}
    }
    std::printf("PASS: %d InventoryMove hook rules, instruction/branch/stack-map audit\n",checks);
    return checks==3?0:7;
}
