#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include "minecraft/compat/lunar12111/Mapping.h"
#include "minecraft/hooks/Edits.h"
#include "minecraft/java/api/class_edit.h"
#include "test/bytecode/audit.h"
using nova::hook::Guard;
#include "modern-guard.inc"
using nova::compat::lunar12111::Member;
constexpr Member members[]{
#define NOVA_MACE_MAPPING
#define NOVA_MACE(key,owner,name,desc) {owner,name,desc},
#include "minecraft/compat/mappings/minecraft263.inc"
#undef NOVA_MACE
#undef NOVA_MACE_MAPPING
};
ClassFileEditor Read(const char* root,const char* owner){
    std::ifstream file(std::string(root)+"/"+owner+".class",std::ios::binary);
    std::vector<u1> bytes{std::istreambuf_iterator<char>(file),{}};
    if(bytes.empty()){std::printf("missing class: %s\n",owner);std::abort();}
    return ClassFileEditor(bytes.data(),bytes.size());
}
// JNI GetMethodID also resolves inherited default interface methods. In 26.3,
// ItemStack.is(TagKey) moved to TypedInstance through ItemInstance.
bool HasMember(const char* root,Member member,int depth=0){
    if(depth>8)return false;
    std::ifstream file(std::string(root)+"/"+member.owner+".class",std::ios::binary);
    std::vector<u1> bytes{std::istreambuf_iterator<char>(file),{}};
    if(bytes.size()<10)return false;
    const bool method=member.descriptor[0]=='(';
    if(method&&ClassFileEditor(bytes.data(),bytes.size()).hasMethod(member.name,member.descriptor))return true;
    const u1* p=bytes.data()+8;const auto end=bytes.data()+bytes.size();audit::Pool cp;
    assert(audit::ParsePool(p,end,cp));
    auto word=[&](){assert(end-p>=2);const auto n=audit::Be16(p);p+=2;return n;};
    auto utf=[&](u2 index){return std::string(cp.bytes[index].begin(),cp.bytes[index].end());};
    word();word();const auto super=word();std::vector<u2> parents;
    if(super)parents.push_back(super);
    const auto count=word();for(int i=0;i<count;++i)parents.push_back(word());
    if(!method){
        const auto fields=word();
        for(int i=0;i<fields;++i){
            word();const auto name=word(),desc=word();
            if(utf(name)==member.name&&utf(desc)==member.descriptor)return true;
            const auto attrs=word();
            for(int j=0;j<attrs;++j){word();assert(end-p>=4);const auto size=audit::Be32(p);p+=4;assert(size<=end-p);p+=size;}
        }
    }
    for(auto parent:parents){
        auto owner=utf(cp.a[parent]);std::replace(owner.begin(),owner.end(),'/','.');
        if(HasMember(root,{owner.c_str(),member.name,member.descriptor},depth+1))return true;
    }
    return false;
}
int main(int argc,char** argv){
    assert(argc==3);
    for(int lunar=0;lunar<2;++lunar){
        const auto root=argv[lunar+1];
        for(auto member:members){
            if(lunar)member=nova::compat::lunar12111::Translate(member);
            if(!HasMember(root,member)){
                std::printf("missing member: %s.%s%s\n",member.owner,member.name,member.descriptor);return 1;
            }
        }
        const auto& guard=mc263GameModeGuards[0];
        Member attack{"net.minecraft.client.multiplayer.MultiPlayerGameMode",guard.method,guard.methodDesc};
        if(lunar)attack=nova::compat::lunar12111::Translate(attack);
        auto file=Read(root,attack.owner);
        assert(file.insertCancelGuard(attack.name,attack.descriptor,"Bridge",guard.hook,guard.hookDesc,guard.argCount,guard.entryReturn));
        std::string error;assert(audit::AuditClass(file.serialize(),error));
        std::printf("PASS: %s mace access methods and native attack guard bytecode audit\n",lunar?"Lunar 1.21.11":"Minecraft 26.3");
    }
}
