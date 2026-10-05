#include "minecraft/compat/mappings/LunarNick.h"
#include "minecraft/java/api/class_edit.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cassert>
#include <algorithm>
int main(int argc,char** argv){
    assert(argc==2||argc==3);using namespace nova::compat::lunarnick;
    if(argc==3){
        std::ifstream file(std::filesystem::path(argv[1])/"net/minecraft/client/gui/FontRenderer.class",std::ios::binary);
        std::vector<u1> bytes{std::istreambuf_iterator<char>(file),{}};assert(!bytes.empty());ClassFileEditor editor(bytes.data(),bytes.size());
        assert(editor.hasMethod("localvar$zcb000$lunar$renderString","(Ljava/lang/String;)Ljava/lang/String;"));
        std::cout<<"PASS: legacy font Nick Hider entry in "<<argv[1]<<'\n';return 0;
    }
    const auto& build=std::filesystem::exists(std::filesystem::path(argv[1])/
        "com/moonsworth/lunar/client/HRCIHRROICRHRHOHIRORRCCHOICRRI/CIIHHRHHHIOIIHHRROOHRHICIHOOOC/RIRROIIHHOCRIOHHIIOCCIOIHOORRI/ROOCCIHRCCHORHCCOIIRIHHHIIHHHO.class")?ModernBuilds[0]:ModernBuilds[1];
    for(const auto m:{build.instance,build.modules,build.nickHider,build.format}){
        std::string name=m.owner;std::replace(name.begin(),name.end(),'.','/');
        std::ifstream file(std::filesystem::path(argv[1])/(name+".class"),std::ios::binary);
        std::vector<u1> bytes{std::istreambuf_iterator<char>(file),{}};assert(!bytes.empty());
        ClassFileEditor editor(bytes.data(),bytes.size());assert(editor.hasMethod(m.name,m.descriptor));
    }std::cout<<"PASS: four Lunar Nick Hider JNI bindings in "<<argv[1]<<'\n';
}
