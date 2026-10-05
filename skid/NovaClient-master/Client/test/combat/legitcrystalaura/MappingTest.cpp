#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include "minecraft/compat/lunar12111/Mapping.h"
#include "minecraft/java/api/class_edit.h"
#include "test/bytecode/audit.h"
using nova::compat::lunar12111::Member;
constexpr Member members[]{
#include "members.inc"
};
bool HasMember(const char* root,Member m,int depth=0) {
    if(depth>12)return false;
    std::ifstream file(std::string(root)+"/"+m.owner+".class",std::ios::binary);
    std::vector<u1> bytes{std::istreambuf_iterator<char>(file),{}};
    if(bytes.size()<10)return false;
    const bool method=m.descriptor[0]=='(';
    if(method&&ClassFileEditor(bytes.data(),bytes.size()).hasMethod(m.name,m.descriptor))return true;
    const u1* p=bytes.data()+8;const auto end=bytes.data()+bytes.size();audit::Pool cp;
    assert(audit::ParsePool(p,end,cp));
    auto word=[&](){assert(end-p>=2);auto n=audit::Be16(p);p+=2;return n;};
    auto utf=[&](u2 i){return std::string(cp.bytes[i].begin(),cp.bytes[i].end());};
    word();word();auto super=word();std::vector<u2> parents;if(super)parents.push_back(super);
    auto count=word();for(int i=0;i<count;++i)parents.push_back(word());
    if(!method) {
        auto fields=word();
        for(int i=0;i<fields;++i) {
            word();auto name=word(),desc=word();if(utf(name)==m.name&&utf(desc)==m.descriptor)return true;
            auto attrs=word();for(int j=0;j<attrs;++j){word();auto size=audit::Be32(p);p+=4;assert(size<=end-p);p+=size;}
        }
    }
    for(auto parent:parents) {
        auto owner=utf(cp.a[parent]);std::replace(owner.begin(),owner.end(),'/','.');
        if(HasMember(root,{owner.c_str(),m.name,m.descriptor},depth+1))return true;
    }
    return false;
}
int main(int argc,char** argv) {
    assert(argc==3);int failed=0;
    for(int lunar=0;lunar<2;++lunar) {
        for(auto m:members) {
            if(lunar)m=nova::compat::lunar12111::Translate(m);
            if(!HasMember(argv[lunar+1],m)){std::printf("MISSING %s.%s%s\n",m.owner,m.name,m.descriptor);++failed;}
        }
        std::printf("Checked %zu JNI bindings against %s jar\n",std::size(members),lunar?"Lunar 1.21.11":"Minecraft 26.3");
    }
    return failed?1:0;
}
