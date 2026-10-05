#include "minecraft/hooks/Edits.h"
#include "minecraft/java/api/base.h"
#include <cassert>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
namespace mock {
bool loader=true,exception=false,failLazy=false;
int localDeletes=0,globalDeletes=0;
std::vector<std::string> loads,loaded;
template<class T>T Id(int n){return reinterpret_cast<T>(static_cast<uintptr_t>(n));}
jboolean Exception(JNIEnv*){return exception?JNI_TRUE:JNI_FALSE;}
void Clear(JNIEnv*){exception=false;}
void DeleteLocal(JNIEnv*,jobject){++localDeletes;}
void DeleteGlobal(JNIEnv*,jobject){++globalDeletes;}
}
namespace nova::compat {
jobject ClassLoader(JNIEnv*){return mock::loader?mock::Id<jobject>(1):nullptr;}
}
namespace nova::hook {
const ClassEdits* AllEdits(int& count){
    static const ClassEdits targets[]{
        {"net/minecraft/client/Minecraft",nullptr,0},
        {"net/minecraft/network/NetworkManager",nullptr,0},
        {"net/minecraft/client/multiplayer/GuiConnecting$1",nullptr,0}
    };
    count=3;return targets;
}
}
void LogTo(const char*,...){}
jclass LoadClassInLoader(JNIEnv*,jobject loader,const char* name){
    assert(loader==mock::Id<jobject>(1));assert(std::strchr(name,'/')==nullptr);
    mock::loads.emplace_back(name);
    if(mock::failLazy&&std::strstr(name,"GuiConnecting$1")){mock::exception=true;return nullptr;}
    if(std::find(mock::loaded.begin(),mock::loaded.end(),name)==mock::loaded.end())mock::loaded.emplace_back(name);
    return mock::Id<jclass>(2);
}
namespace nova::hook {
// @PRODUCTION_LOAD_TARGETS@
}
int main(){
    JNINativeInterface_ functions{};functions.ExceptionCheck=mock::Exception;
    functions.ExceptionClear=mock::Clear;functions.DeleteLocalRef=mock::DeleteLocal;functions.DeleteGlobalRef=mock::DeleteGlobal;
    JNIEnv env{&functions};
    mock::loaded={"net.minecraft.client.Minecraft","net.minecraft.network.NetworkManager"};
    assert(nova::hook::LoadRequiredTargets(&env));assert(mock::loaded.size()==3&&mock::loads.size()==3);
    assert(mock::loads.back()=="net.minecraft.client.multiplayer.GuiConnecting$1");
    assert(mock::localDeletes==3&&mock::globalDeletes==1);
    mock::failLazy=true;assert(!nova::hook::LoadRequiredTargets(&env));
    assert(!mock::exception&&mock::localDeletes==5&&mock::globalDeletes==2);
    mock::loader=false;const auto before=mock::loads.size();assert(!nova::hook::LoadRequiredTargets(&env));assert(mock::loads.size()==before);
    std::cout<<"PASS: main-menu lazy join class is loaded before snapshot; missing class stays fatal, exceptions and JNI references are cleaned\n";
}
