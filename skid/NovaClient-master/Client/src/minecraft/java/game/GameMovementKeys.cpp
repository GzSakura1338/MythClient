#include "minecraft/java/api/game_options.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"

namespace nova::java {
namespace {
using compat::mc263::access::Frame;
using compat::mc263::access::Member;
struct Mapping {
    Member sprint{},drop{},swap{},sneak{},attack{},use{},clicks{};
    Member options{}, jump{}, forward{}, back{}, left{}, right{}, key{}, keyCode{}, keyType{}, pressed{};
    Member window{}, windowHandle{}, physicalKey{};
};
#define NOVA_MOVEMENT_KEYS_MAPPING
#define NOVA_MOVEMENT_KEY(id, owner, name, desc) .id = {owner, name, desc},
constexpr Mapping lunar{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr Mapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr Mapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_MOVEMENT_KEY
#undef NOVA_MOVEMENT_KEYS_MAPPING

constexpr bool Complete(const Mapping& m) {
    for (Member member : {m.options,m.jump,m.forward,m.back,m.left,m.right,m.sneak,
             m.sprint,m.drop,m.attack,m.use,m.clicks,m.keyCode,m.pressed})
        if (!compat::mc263::access::ValidMember(member)) return false;
    return true;
}
static_assert(Complete(lunar) && Complete(forge) && Complete(modern),
    "Movement/FreeCam input mapping is incomplete");

template<class T, class... Args>
T Static(Frame& f, Member member, Args... args) {
    const auto method = compat::mc263::access::Method(f.env, member, true);
    if (!method || !f.check()) { f.ok = false; return {}; }
    const auto type = compat::mc263::access::Class(f.env, member.owner);
    T value{};
    if constexpr (std::is_same_v<T, bool>) value = f.env->CallStaticBooleanMethod(type, method, args...) != JNI_FALSE;
    else value = f.env->CallStaticIntMethod(type, method, args...);
    f.check();
    return value;
}

bool PhysicalDown(Frame& f, const Mapping& m, jobject client, jobject binding) {
    if (!binding || !f.check()) { f.ok=false; return false; }
    if (compat::IsLegacyLunar()) {
        const int code = f.call<int>(binding, m.keyCode);
        if (code == 0 || code < -100) return false;
        return code < 0 ? Static<bool>(f, {"org.lwjgl.input.Mouse", "isButtonDown", "(I)Z"}, code + 100)
            : Static<bool>(f, {"org.lwjgl.input.Keyboard", "isKeyDown", "(I)Z"}, code);
    }
    auto key = f.get(binding, m.key);
    const int code = f.call<int>(key, m.keyCode);
    if (!key || code < 0) return false;
    auto type = f.call(key, m.keyType);
    const auto name = f.text(f.call(type, {"java.lang.Enum", "name", "()Ljava/lang/String;"}));
    if (compat::IsMinecraft263()) {
        if (name != "MOUSE") return Static<bool>(f, m.physicalKey, code);
        constexpr int buttons[]{VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2};
        return code < 5 && (GetAsyncKeyState(buttons[code]) & 0x8000) != 0;
    }
    const jlong window = compat::UsesGlfwModernRuntime() ? f.call<jlong>(f.call(client,compat::mc263::names::Minecraft_getWindow),compat::mc263::names::Window_handle) : f.call<jlong>(f.call(client, m.window), m.windowHandle);
    if (name == "MOUSE")
        return Static<int>(f, {"org.lwjgl.glfw.GLFW", "glfwGetMouseButton", "(JI)I"}, window, code) == 1;
    if (name == "SCANCODE") {
        for (int symbol = 32; symbol <= 348 && f; ++symbol)
            if (Static<int>(f, {"org.lwjgl.glfw.GLFW", "glfwGetKeyScancode", "(I)I"}, symbol) == code)
                return Static<int>(f, {"org.lwjgl.glfw.GLFW", "glfwGetKey", "(JI)I"}, window, symbol) == 1;
        return false;
    }
    return Static<int>(f, {"org.lwjgl.glfw.GLFW", "glfwGetKey", "(JI)I"}, window, code) == 1;
}
}

bool SampleKeys(InventoryMoveKeys* out) {
    if(FreeCamActive())return false;
    Frame f;
    if (!f || (!out&&ScreenOpen()) || !f.player()) return false;
    const auto& m = compat::IsLegacyLunar() ? lunar : compat::UsesModernRuntime() ? modern : forge;
    auto client = f.client();
    auto options = f.get(client, m.options);
    if (!options || !f.check()) return false;
    const Member members[]{m.forward, m.back, m.left, m.right,m.jump};
    jobject bindings[5]{};
    bool down[5]{};
    const int count=out?5:4;
    for (int i = 0; i < count; ++i) {
        bindings[i] = f.get(options, members[i]);
        if (!bindings[i] || !f.check()) return false;
        down[i] = PhysicalDown(f, m, client, bindings[i]);
        if (!f.check()) return false;
    }
    // Set held state only: no click counters and no synthetic input events.
    // Read the live binding, so rebinding movement away from WASD still works.
    for (int i = 0; i < count; ++i)
        if (!f.set(bindings[i], m.pressed, down[i])) return false;
    if(out){out->forward=float(down[0])-float(down[1]);out->strafe=float(down[2])-float(down[3]);out->jump=down[4];out->directions=int(down[0])+int(down[1])+int(down[2])+int(down[3]);}
    return true;
}
bool RestoreMovementKeys(){return SampleKeys(nullptr);}
bool RestoreSprintKey(bool resume){
    Frame f;const auto& m=compat::IsLegacyLunar()?lunar:compat::UsesModernRuntime()?modern:forge;
    auto client=f.client(),key=f.get(f.get(client,m.options),m.sprint);
    if(!key)return false;
    const bool down=!ScreenOpen()&&(resume||PhysicalDown(f,m,client,key));
    return f.check()&&f.set(key,m.pressed,down);
}
void SuppressUseKey(){
    Frame f;const auto& m=compat::IsLegacyLunar()?lunar:compat::UsesModernRuntime()?modern:forge;
    auto key=f.get(f.get(f.client(),m.options),m.use);
    if(key){f.set(key,m.pressed,false);f.set(key,m.clicks,0);}
}
void RestoreUseKey(){
    Frame f;const auto& m=compat::IsLegacyLunar()?lunar:compat::UsesModernRuntime()?modern:forge;
    auto client=f.client(),key=f.get(f.get(client,m.options),m.use);
    if(!key)return;
    const bool down=!ScreenOpen()&&PhysicalDown(f,m,client,key);
    if(f.check()){f.set(key,m.pressed,down);f.set(key,m.clicks,0);}
}
bool ReadFreeCamKeys(FreeCamKeys& out){
    out={};
    Frame f;const auto& m=compat::IsLegacyLunar()?lunar:compat::UsesModernRuntime()?modern:forge;
    auto client=f.client(),options=f.get(client,m.options);if(!options)return false;
    auto down=[&](Member member){return PhysicalDown(f,m,client,f.get(options,member));};
    FreeCamKeys sampled;
    sampled.forward=float(down(m.forward))-float(down(m.back));sampled.strafe=float(down(m.left))-float(down(m.right));
    sampled.up=down(m.jump);sampled.down=down(m.sneak);
    if(!f.check())return false;
    out=sampled;return true;
}
void SuppressFreeCamKeys(){
    if(!FreeCamActive())return;
    Frame f;const auto& m=compat::IsLegacyLunar()?lunar:compat::UsesModernRuntime()?modern:forge;
    auto options=f.get(f.client(),m.options);if(!options)return;
    for(auto member:{m.forward,m.back,m.left,m.right,m.jump,m.sneak,m.sprint}){
        if(!member.owner)continue;
        auto key=f.get(options,member);if(key){f.set(key,m.pressed,false);f.set(key,m.clicks,0);}
    }
    for(bool right:{false,true})if(FreeCamBlocksMouse(right)){
        auto key=f.get(options,right?m.use:m.attack);
        if(key){f.set(key,m.pressed,false);f.set(key,m.clicks,0);}
    }
}
bool ReadInventoryMoveKeys(InventoryMoveKeys& out){return compat::IsLunar189()&&SampleKeys(&out);}
}
