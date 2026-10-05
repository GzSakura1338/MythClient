#include "minecraft/java/api/game_bridgeassist.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_player.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/compat/mappings/BridgeAssistMapping.h"
#include "minecraft/compat/lunar1710/Access.h"
#include "minecraft/modules/player/bridgeassist/BridgeSimulation.h"

namespace nova::java {
using rotation::Vec3;
namespace {
using compat::BridgeAssistMapping;
namespace access = compat::mc263::access;
using access::Frame;
using access::Member;
namespace simulation = module::bridgeassist;

#define NOVA_BRIDGEASSIST_MAPPING
#define NOVA_BRIDGE(id, owner, name, desc) .id = {owner, name, desc},
constexpr BridgeAssistMapping legacy{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr BridgeAssistMapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr BridgeAssistMapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_BRIDGE
#undef NOVA_BRIDGEASSIST_MAPPING
const BridgeAssistMapping& Mapping() {
    return compat::IsLegacyLunar() ? legacy : compat::UsesModernRuntime() ? modern : forge;
}

struct Locals {
    Frame& f;
    bool pushed;
    explicit Locals(Frame& frame) : f(frame), pushed(f && f.env->PushLocalFrame(64) == JNI_OK) {
        if (!pushed) { f.check(); f.ok = false; }
    }
    ~Locals() { if (pushed) f.env->PopLocalFrame(nullptr); }
};
template<class T, class... Args> T Static(Frame& f, Member member, Args... args) {
    if (!f) return {};
    const auto method = access::Method(f.env, member, true);
    if (!method) { f.ok = false; return {}; }
    const auto cls = access::Class(f.env, member.owner);
    T result{};
    if constexpr (std::is_same_v<T, bool>) result = f.env->CallStaticBooleanMethod(cls, method, args...) != JNI_FALSE;
    if constexpr (std::is_same_v<T, int>) result = f.env->CallStaticIntMethod(cls, method, args...);
    if constexpr (std::is_same_v<T, float>) result = f.env->CallStaticFloatMethod(cls, method, args...);
    f.check();
    return result;
}
bool PhysicalSneak(Frame& f, const BridgeAssistMapping& m, jobject mc) {
    auto binding = f.get(f.get(mc,m.options),m.sneakKey);
    if (!binding) { f.ok = false; return false; }
    if (compat::IsLegacyLunar()) {
        const int code = f.call<int>(binding,m.keyCode);
        if (code == 0) return false;
        return code < 0 ? Static<bool>(f,{"org.lwjgl.input.Mouse","isButtonDown","(I)Z"},code+100) :
            Static<bool>(f,{"org.lwjgl.input.Keyboard","isKeyDown","(I)Z"},code);
    }
    auto key = f.get(binding,m.key);
    if (!key) { f.ok = false; return false; }
    const int code=f.call<int>(key,m.keyCode);
    if (code < 0) return false;
    auto type=f.call(key,m.keyType);
    const auto name=f.text(f.call(type,{"java.lang.Enum","name","()Ljava/lang/String;"}));
    if (compat::IsMinecraft263()) {
        if (name != "MOUSE") return Static<bool>(f,m.physicalKey,code);
        constexpr int buttons[]{VK_LBUTTON,VK_RBUTTON,VK_MBUTTON,VK_XBUTTON1,VK_XBUTTON2};
        return code < 5 && (GetAsyncKeyState(buttons[code]) & 0x8000) != 0;
    }
    const jlong window=compat::UsesGlfwModernRuntime()?f.call<jlong>(f.call(mc,compat::mc263::names::Minecraft_getWindow),compat::mc263::names::Window_handle):f.call<jlong>(f.call(mc,m.window),m.windowHandle);
    if (name == "MOUSE") return Static<int>(f,{"org.lwjgl.glfw.GLFW","glfwGetMouseButton","(JI)I"},window,code) == 1;
    if (name == "SCANCODE") {
        // GLFW polls keys by key symbol; translate an explicitly rebound scan code.
        for (int symbol=32; symbol<=348 && f; ++symbol)
            if (Static<int>(f,{"org.lwjgl.glfw.GLFW","glfwGetKeyScancode","(I)I"},symbol)==code)
                return Static<int>(f,{"org.lwjgl.glfw.GLFW","glfwGetKey","(JI)I"},window,symbol)==1;
        return false;
    }
    return Static<int>(f,{"org.lwjgl.glfw.GLFW","glfwGetKey","(JI)I"},window,code)==1;
}

struct World {
    Frame& f;
    const BridgeAssistMapping& m;
    jobject player, level;
    bool old = compat::IsLegacyLunar();
    bool Good() { return f && level && player; }
    jobject BoxObject(const Box& b) { return f.create(m.boxCtor,b.minX,b.minY,b.minZ,b.maxX,b.maxY,b.maxZ); }
    Box ReadBox(jobject b) {
        return {f.get<double>(b,m.minX),f.get<double>(b,m.minY),f.get<double>(b,m.minZ),
            f.get<double>(b,m.maxX),f.get<double>(b,m.maxY),f.get<double>(b,m.maxZ)};
    }
    template<class Fn> bool Each(jobject iterable, Fn&& fn) {
        if (!iterable) { f.ok=false; return false; }
        auto iterator=f.call(iterable,{"java.lang.Iterable","iterator","()Ljava/util/Iterator;"});
        while (f && f.call<bool>(iterator,{"java.util.Iterator","hasNext","()Z"})) {
            auto value=f.call(iterator,{"java.util.Iterator","next","()Ljava/lang/Object;"});
            if (!value || !fn(value)) { f.ok=false; break; }
            f.env->DeleteLocalRef(value);
        }
        if (iterator) f.env->DeleteLocalRef(iterator);
        return Good();
    }
    bool Collisions(const Box& area, std::vector<Box>& out) {
        out.clear();
        if (!Good() || !simulation::Finite(area)) return false;
        Locals scope(f);
        auto shapes=f.call(level,m.collisions,player,BoxObject(area));
        const auto append=[&](jobject value) {
            auto b=ReadBox(value);
            if (!f || !simulation::Finite(b)) return false;
            out.push_back(b); return true;
        };
        return Each(shapes,[&](jobject value) {
            if (old) return append(value);
            auto boxes=f.call(value,m.shapeBoxes);
            bool ok=Each(boxes,append);
            if (boxes) f.env->DeleteLocalRef(boxes);
            return ok;
        });
    }
    jobject Block(int x,int y,int z) {
        if (compat::IsLunar1710()) return compat::lunar1710::Block(f,level,x,y,z);
        return f.call(f.call(level,m.state,f.create(m.posCtor,x,y,z)),m.block);
    }
    bool NormalCube(int x,int y,int z) {
        Locals scope(f);
        return f.call<bool>(Block(x,y,z),m.normal);
    }
    float Slipperiness(int x,int y,int z) {
        Locals scope(f);
        auto block=Block(x,y,z);
        return old ? f.get<float>(block,m.friction) : f.call<float>(block,m.friction);
    }
    bool Water(simulation::Simulation& s) {
        if (!old) {
            s.water=f.call<bool>(player,m.inWater); s.lava=f.call<bool>(player,m.inLava);
            return Good();
        }
        Locals scope(f);
        const Box waterBox{s.box.minX+.001,s.box.minY+.4010000059604645,s.box.minZ+.001,
            s.box.maxX-.001,s.box.maxY-.4010000059604645,s.box.maxZ-.001};
        const Box lavaBox{s.box.minX+.10000000149011612,s.box.minY+.4000000059604645,s.box.minZ+.10000000149011612,
            s.box.maxX-.10000000149011612,s.box.maxY-.4000000059604645,s.box.maxZ-.10000000149011612};
        s.lava=f.call<bool>(level,m.materialInBox,BoxObject(lavaBox),f.stat(m.lava));
        const int x0=int(std::floor(waterBox.minX)), y0=int(std::floor(waterBox.minY)), z0=int(std::floor(waterBox.minZ));
        const int x1=int(std::floor(waterBox.maxX+1)), y1=int(std::floor(waterBox.maxY+1)), z1=int(std::floor(waterBox.maxZ+1));
        s.water=false;
        const bool v17=compat::IsLunar1710();
        if (v17 ? !f.call<bool>(level,compat::lunar1710::names::AreaLoaded,x0,y0,z0,x1,y1,z1) :
            !f.call<bool>(level,m.areaLoaded,f.create(m.posCtor,x0,y0,z0),f.create(m.posCtor,x1,y1,z1),JNI_TRUE)) return Good();
        auto water=f.stat(m.water), property=v17 ? nullptr : f.stat(m.liquidLevel);
        Vec3 flow;
        for (int x=x0;x<x1 && f;++x) for (int y=y0;y<y1 && f;++y) for (int z=z0;z<z1 && f;++z) {
            Locals cell(f);
            auto pos=v17 ? nullptr : f.create(m.posCtor,x,y,z);
            auto state=v17 ? nullptr : f.call(level,m.state,pos);
            auto block=v17 ? Block(x,y,z) : f.call(state,m.block);
            auto material=f.call(block,m.material);
            if (!f || !material || !f.env->IsSameObject(material,water)) continue;
            const int value=v17 ? f.call<int>(level,compat::lunar1710::names::BlockMetadata,x,y,z) :
                f.call<int>(f.call(state,m.stateValue,property),{"java.lang.Integer","intValue","()I"});
            const float height=Static<float>(f,m.liquidHeight,value);
            if (y1 < (y+1)-height) continue;
            s.water=true;
            auto vec=f.create(m.vecCtor,flow.x,flow.y,flow.z);
            if (v17) f.invoke(block,compat::lunar1710::names::Flow,level,x,y,z,player,vec);
            else vec=f.call(block,m.flow,level,pos,player,vec);
            flow={f.get<double>(vec,m.vecX),f.get<double>(vec,m.vecY),f.get<double>(vec,m.vecZ)};
        }
        const double length=std::sqrt(flow.x*flow.x+flow.y*flow.y+flow.z*flow.z);
        if (length>0) {
            // Vec3.normalize returns zero below 1e-4 in Minecraft 1.8.
            const double scale=length<1e-4 ? 0 : .014/length;
            s.motion=s.motion.Add(flow.x*scale,flow.y*scale,flow.z*scale);
        }
        return Good();
    }
};
}

bool ReadBridgePlayer(BridgePlayer& out) {
    out={};
    Frame f;
    if (!f) return false;
    const auto& m=Mapping();
    auto mc=f.client(), player=f.player();
    if (!mc || !player) return false;
    auto abilities=compat::IsLegacyLunar() ? f.get(player,m.abilities) : f.call(player,m.abilities);
    if (!abilities) return false;
    out.flying=f.get<bool>(abilities,m.flying);
    out.manualSneak=PhysicalSneak(f,m,mc);
    auto input=f.get(player,m.input);
    if (!input) return false;
    out.forward=compat::UsesModernRuntime() ? f.get<float>(f.call(input,m.moveVector),m.vectorY) : f.get<float>(input,m.forward);
    out.reach=compat::UsesModernRuntime() ? f.call<double>(player,m.reach) : f.call<float>(f.get(mc,m.controller),m.reach);
    return f && std::isfinite(out.reach) && out.reach>0;
}

bool BridgeEdgeOffset(float forward, float strafe, bool jump, double& out) {
    Frame f;
    if (!f) return false;
    const auto& m=Mapping();
    auto player=PlayerEntity();
    if (!player) return false;
    auto object=static_cast<jobject>(player.raw());
    World world{f,m,object,f.get(f.client(),m.world)};
    simulation::Simulation s;
    s.legacy=compat::IsLegacyLunar();
    Vec3 pos;
    float pitch=0,health=0,absorption=0;
    if (!world.Good() || !player.BoundingBox(s.box) || !player.Position(pos) || !PlayerDeltaMovement(s.motion) ||
        !player.OnGround(s.ground) || !PlayerRotation(s.yaw,pitch) || !PlayerSprinting(s.sprint) ||
        !player.Health(health,absorption)) return false;
    s.x=pos.x; s.y=pos.y; s.z=pos.z;
    s.forward=forward; s.strafe=strafe; s.jump=jump;
    s.spectator=player.IsSpectator();
    s.riding=f.call<bool>(object,m.riding);
    s.blocked=health<=0 || f.call<bool>(object,m.sleeping);
    s.usingItem=PlayerUsingItem();
    s.horizontalCollision=f.get<bool>(object,m.horizontal);
    s.jumpCooldown=f.get<int>(object,m.jumpCooldown);
    s.food=f.call<int>(f.call(object,m.food),m.foodLevel);
    s.ladder=f.call<bool>(object,m.ladder);
    if (s.legacy) {
        s.stepHeight=f.get<float>(object,m.step); s.airSpeed=f.get<float>(object,m.airSpeed);
        s.web=f.get<bool>(object,m.web); s.width=f.get<float>(object,m.width); s.height=f.get<float>(object,m.height);
        s.speed=static_cast<float>(f.call<double>(f.call(object,m.attribute,f.stat(m.speedAttribute)),m.attributeValue));
        auto effect=f.call(object,m.getEffect,f.stat(m.jumpEffect));
        if (effect) s.jumpPower += (f.call<int>(effect,m.amplifier)+1)*.1f;
        s.depthStrider=compat::IsLunar1710() ? 0.f : static_cast<float>(Static<int>(f,m.depth,object));
    } else {
        s.stepHeight=f.call<float>(object,m.step);
        s.jumpPower=f.call<float>(object,m.jumpPower);
        // 1.20 adds the effect after getJumpPower; 26.3 includes it in that method.
        if (!compat::UsesModernRuntime()) s.jumpPower += f.call<float>(object,m.jumpBoost);
        if (!PlayerSpeed(s.speed)) return false;
        s.airSpeed=s.sprint?.026f:.02f;
    }
    Box predicted;
    return f && simulation::Predict(s,world,predicted) && simulation::EdgeOffset(predicted,world,out);
}
void ReleaseBridgeAssistAccess(JNIEnv* env) { access::Release(env); }
}
