#include "minecraft/compat/neoforge12110/NeoForge12110.h"
#include "minecraft/compat/easecation/TimerRate.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include <mutex>
#include <unordered_set>
#include <string_view>

namespace nova::compat::neoforge12110 {
namespace {
struct TypeAlias {const char* from;const char* to;};
constexpr TypeAlias types[]{
#define NOVA_NEOFORGE12110_TYPE(from,to) {from,to},
#include "minecraft/compat/mappings/neoforge12110.inc"
#undef NOVA_NEOFORGE12110_TYPE
};
std::mutex mutex;
std::unordered_set<std::string> descriptors;
jfieldID screen=nullptr;
easecation::TimerRate timerRate; // Shared arithmetic; this profile owns its state.
}
const char* ClassName(const char* name) {
    if(!IsNeoForge12110() || !name)return name;
    for(auto& type:types)if(std::string_view(name)==type.from)return type.to;
    return name;
}
mc263::names::Member Translate(mc263::names::Member member) {
    if(!IsNeoForge12110() || !member.owner || !member.name || !member.descriptor)return member;
    struct Alias {mc263::names::Member from,to;};
    static constexpr Alias aliases[]{
#define NOVA_NEOFORGE12110_ALIAS(oc,on,od,nc,nn,nd) {{oc,on,od},{nc,nn,nd}},
#include "minecraft/compat/mappings/neoforge12110.inc"
#undef NOVA_NEOFORGE12110_ALIAS
    };
    for(auto& alias:aliases)
        if(std::string_view(member.owner)==alias.from.owner && std::string_view(member.name)==alias.from.name &&
           std::string_view(member.descriptor)==alias.from.descriptor)return alias.to;
    member.owner=ClassName(member.owner);
    std::string descriptor(member.descriptor);
    bool changed=false;
    for(auto& type:types) {
        std::string from(type.from),to(type.to);
        for(char& c:from)if(c=='.')c='/';
        for(char& c:to)if(c=='.')c='/';
        from="L"+from+";";to="L"+to+";";
        for(size_t pos=0;(pos=descriptor.find(from,pos))!=std::string::npos;pos+=to.size()) {
            descriptor.replace(pos,from.size(),to);changed=true;
        }
    }
    if(changed){std::lock_guard lock(mutex);member.descriptor=descriptors.insert(std::move(descriptor)).first->c_str();}
    return member;
}
bool Initialize(JNIEnv* env) {
    mc263::access::Frame frame(env);
    const auto version=frame.staticCall(mc263::names::SharedConstants_getCurrentVersion);
    const auto id=frame.text(frame.call(version,mc263::names::WorldVersion_id));
    if(!frame.check() || id!="1.21.10") {
        LogTo("NeoForge 1.21.10: expected Minecraft 1.21.10, found %s",id.c_str());return false;
    }
    // Require the loader too: a Mojang-named vanilla JVM is a different target.
    if(!mc263::access::Class(env,"net.neoforged.neoforge.common.NeoForge"))return false;
    screen=mc263::access::Field(env,names::Screen);
    const auto window=frame.call(frame.client(),mc263::names::Minecraft_getWindow);
    const auto handle=frame.call<jlong>(window,mc263::names::Window_handle);
    LogTo("NeoForge 1.21.10: Mojang namespace, Java 21, GLFW window=%p",reinterpret_cast<void*>(handle));
    return screen && handle && frame.check();
}
jobject Screen(JNIEnv* env,jobject minecraft){return minecraft&&screen?env->GetObjectField(minecraft,screen):nullptr;}
bool SetTimerSpeed(float speed){return timerRate.Set(speed);}
float TimerMillis(float vanilla){return timerRate.Millis(vanilla);}
void Shutdown(JNIEnv*){timerRate.Reset();screen=nullptr;std::lock_guard lock(mutex);descriptors.clear();}
}
