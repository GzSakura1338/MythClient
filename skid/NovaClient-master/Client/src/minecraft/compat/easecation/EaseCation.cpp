#include "minecraft/compat/easecation/EaseCation.h"
#include "minecraft/compat/easecation/TimerRate.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include <mutex>
#include <unordered_set>
#include <string_view>

namespace nova::compat::easecation {
namespace {
struct TypeAlias { const char* from; const char* to; };
constexpr TypeAlias types[]{
#define NOVA_EASECATION_TYPE(from, to) {from, to},
#include "minecraft/compat/mappings/easecation.inc"
#undef NOVA_EASECATION_TYPE
};
std::mutex mutex;
std::unordered_set<std::string> descriptors;
jfieldID screen = nullptr;
TimerRate timerRate;
}
const char* ClassName(const char* name) {
    if (!IsEaseCation() || !name) return name;
    for (auto& t : types) if (std::string_view(name) == t.from) return t.to;
    return name;
}
mc263::names::Member Translate(mc263::names::Member m) {
    if (!IsEaseCation() || !m.owner || !m.name || !m.descriptor) return m;
    struct Alias { mc263::names::Member from, to; };
    static constexpr Alias aliases[]{
#define NOVA_EASECATION_ALIAS(oc,on,od,nc,nn,nd) {{oc,on,od},{nc,nn,nd}},
#include "minecraft/compat/mappings/easecation.inc"
#undef NOVA_EASECATION_ALIAS
    };
    for (auto& a : aliases)
        if (std::string_view(m.owner)==a.from.owner && std::string_view(m.name)==a.from.name &&
            std::string_view(m.descriptor)==a.from.descriptor) return a.to;
    m.owner = ClassName(m.owner);
    std::string desc(m.descriptor);
    bool changed = false;
    for (auto& t : types) {
        std::string from(t.from), to(t.to);
        for (char& c : from) if (c=='.') c='/';
        for (char& c : to) if (c=='.') c='/';
        from = "L"+from+";"; to = "L"+to+";";
        for (size_t pos=0;(pos=desc.find(from,pos))!=std::string::npos;pos+=to.size()) {
            desc.replace(pos,from.size(),to); changed=true;
        }
    }
    if (changed) { std::lock_guard lock(mutex); m.descriptor=descriptors.insert(std::move(desc)).first->c_str(); }
    return m;
}
bool Initialize(JNIEnv* env) {
    using namespace mc263;
    access::Frame f(env);
    auto version=f.staticCall(mc263::names::SharedConstants_getCurrentVersion);
    if (f.text(f.call(version,mc263::names::WorldVersion_id))!="1.21.8") {
        LogTo("EaseCation: expected the 1.21.8 NeoForge runtime"); return false;
    }
    screen=access::Field(env,{"net.minecraft.client.Minecraft","screen","Lnet/minecraft/client/gui/screens/Screen;"});
    auto window=f.call(f.client(),mc263::names::Minecraft_getWindow);
    const auto handle=f.call<jlong>(window,mc263::names::Window_handle);
    LogTo("EaseCation: Mojang namespace, Java 21, GLFW window=%p",reinterpret_cast<void*>(handle));
    return screen && handle && f.check();
}
jobject Screen(JNIEnv* env,jobject mc) { return mc && screen ? env->GetObjectField(mc,screen) : nullptr; }
bool SetTimerSpeed(float speed) { return timerRate.Set(speed); }
float TimerMillis(float vanilla) { return timerRate.Millis(vanilla); }
void Shutdown(JNIEnv*) { timerRate.Reset(); screen=nullptr; std::lock_guard lock(mutex); descriptors.clear(); }
}
