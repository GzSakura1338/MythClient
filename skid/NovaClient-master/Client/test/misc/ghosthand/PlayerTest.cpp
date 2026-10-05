#include "minecraft/modules/misc/ghosthand/RayTraversal.h"
#include "minecraft/java/api/class_edit.h"
#include "minecraft/hooks/Edits.h"
#include "../../bytecode/audit.h"
#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>

using nova::module::ghosthand::Traverse;
using nova::rotation::Vec3;
int main(int argc, char** argv) {
    using Cell = std::array<int,3>;
    auto cells = [](Vec3 from, Vec3 to) {
        std::vector<Cell> result;
        Traverse(from,to,[&](int x,int y,int z){ result.push_back({x,y,z}); return false; });
        return result;
    };
    assert((cells({.5,.5,.5},{3.5,.5,.5}) == std::vector<Cell>{{0,0,0},{1,0,0},{2,0,0},{3,0,0}}));
    assert((cells({0,.5,.5},{-2.5,.5,.5}) == std::vector<Cell>{{0,0,0},{-1,0,0},{-2,0,0},{-3,0,0}}));
    assert((cells({.5,2,.5},{.5,-.5,.5}) == std::vector<Cell>{{0,2,0},{0,1,0},{0,0,0},{0,-1,0}}));
    assert((cells({.5,.5,.5},{.5,.5,.5}) == std::vector<Cell>{{0,0,0}}));
    assert(cells({NAN,0,0},{1,1,1}).empty());
    assert(cells({0,0,0},{INFINITY,1,1}).empty());
    const auto diagonal = cells({.5,.5,.5},{3.5,3.5,3.5});
    assert((diagonal.front()==Cell{0,0,0} && diagonal.back()==Cell{3,3,3}));
    int visits=0;
    Traverse({.5,.5,.5},{5.5,.5,.5},[&](int x,int,int){ ++visits; return x==2; });
    assert(visits==3); // A preferred block terminates scanning before further blocks.

    assert(argc==3);
    const std::string profile=argv[1];
    nova::hook::SelectLunarEdits(profile=="lunar");
    nova::hook::SelectLunar1710Edits(profile=="1710");
    nova::hook::SelectMinecraft263Edits(profile=="263");
    nova::hook::SelectEaseCationEdits(profile=="easecation");
    int count=0,checked=0; auto edits=nova::hook::AllEdits(count);
    for(int c=0;c<count;++c) {
        const auto& e=edits[c];
        for(int i=0;i<e.appendCount;++i) {
            const auto& a=e.appends[i];
            if(std::string(a.hook)!="cursorPickEnd")continue;
            std::ifstream in(std::filesystem::path(argv[2])/(std::string(e.internalName)+".class"),std::ios::binary);
            std::vector<u1> bytes{std::istreambuf_iterator<char>(in),{}};
            assert(!bytes.empty());
            assert(!a.atHead && a.allVoidReturns && a.loadCount==1 && std::string(a.hookDesc)=="(F)V");
            ClassFileEditor editor(bytes.data(),bytes.size());
            auto ref=editor.addMethodRef("nova/test/GhostHand",a.hook,a.hookDesc);
            auto code=nova::hook::AppendCode(ref,a.loads,a.loadCount);
            assert(editor.appendBeforeVoidReturns(a.method,a.methodDesc,code,a.loadCount));
            std::string error;
            if(!audit::AuditClass(editor.serialize(),error)){std::cerr<<error;return 1;}
            ++checked;
        }
    }
    assert(checked==1);
    std::cout<<"PASS "<<profile<<": ray traversal, exact-boundary/negative/diagonal rays, cursor tail rewrite and stack maps\n";
}
