#include "minecraft/java/api/game_backtrack.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/jni_lookup.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar1710/Mapping.h"
#include "minecraft/events/GameEvents.h"
#include <cmath>
#include <mutex>
#include <string>
#include <unordered_map>

namespace nova::java {
namespace {
struct Binding { const char* key; const char* owner; const char* name; const char* desc; };
#define NOVA_BACKTRACK_MAPPING
#define NOVA_BT(key, owner, name, desc) {key, owner, name, desc},
const Binding lunar[]{
#include "minecraft/compat/mappings/Lunar189.inc"
};
const Binding forge[]{
#include "minecraft/compat/mappings/forge1201.inc"
};
const Binding modern[]{
#include "minecraft/compat/mappings/minecraft263.inc"
};
const Binding easecation[]{
#include "minecraft/compat/mappings/easecation.inc"
};
const Binding neoforge12110[]{
#include "minecraft/compat/mappings/neoforge12110.inc"
};
#undef NOVA_BT
#undef NOVA_BACKTRACK_MAPPING
std::mutex cacheLock;
std::unordered_map<std::string, jclass> classes;
const Binding* Find(const char* key) {
    auto find = [&](const auto& table) -> const Binding* {
        for (const auto& b : table) if (std::string_view(b.key) == key) return &b;
        return nullptr;
    };
    if(compat::IsNeoForge12110()){if(auto entry=find(neoforge12110))return entry;}
    else if(compat::UsesGlfwModernRuntime())if(auto entry=find(easecation))return entry;
    return compat::IsLegacyLunar() ? find(lunar) : compat::UsesModernRuntime() ? find(modern) : find(forge);
}
struct Frame {
    JniAttach attach;
    JNIEnv* e = attach.env;
    bool ok = false;
    Frame() { pushed = ok = e && e->PushLocalFrame(128) == JNI_OK; }
    ~Frame() { if (e && pushed) e->PopLocalFrame(nullptr); }
    bool pushed = false;
    bool good() { if (e && e->ExceptionCheck()) { e->ExceptionClear(); ok = false; } return ok; }
    jclass type(const char* name) {
        if (!good()) return nullptr;
        std::lock_guard lock(cacheLock);
        auto it = classes.find(name);
        if (it != classes.end()) return it->second;
        jobject loader = compat::ClassLoader(e);
        jclass local = LoadClassInLoader(e, loader, name);
        if (loader) e->DeleteGlobalRef(loader);
        if (!good() || !local) { ok = false; return nullptr; }
        auto global = static_cast<jclass>(e->NewGlobalRef(local));
        e->DeleteLocalRef(local);
        if (!global) { good(); ok = false; return nullptr; }
        classes.emplace(name, global);
        return global;
    }
    bool is(jobject o, const char* key) {
        const auto* b = Find(key);
        if (!b || !o) return false; // Optional family absent from this protocol.
        auto c = type(b->owner);
        return c && e->IsInstanceOf(o, c) && good();
    }
    jfieldID field(const char* key) {
        const auto* b = Find(key);
        if (!b) { ok = false; return nullptr; }
        auto c = type(b->owner);
        const auto symbol=compat::lunar1710::Translate(compat::mc263::names::Member{b->owner,b->name,b->desc});
        auto id = c ? e->GetFieldID(c,symbol.name,symbol.descriptor) : nullptr;
        if (!good() || !id) { ok = false; return nullptr; }
        return id;
    }
    jmethodID method(const char* key) {
        const auto* b = Find(key);
        if (!b) { ok = false; return nullptr; }
        auto c = type(b->owner);
        const auto symbol=compat::lunar1710::Translate(compat::mc263::names::Member{b->owner,b->name,b->desc});
        auto id = c ? e->GetMethodID(c,symbol.name,symbol.descriptor) : nullptr;
        if (!good() || !id) { ok = false; return nullptr; }
        return id;
    }
    jobject object(jobject o,const char* key) {
        auto id = field(key); return o && id ? e->GetObjectField(o,id) : nullptr;
    }
    jobject call(jobject o,const char* key) {
        auto id = method(key); return o && id ? e->CallObjectMethod(o,id) : nullptr;
    }
    double number(jobject o,const char* key) {
        auto id = field(key); if (!o || !id) return 0;
        switch (Find(key)->desc[0]) {
        case 'B': return e->GetByteField(o,id);
        case 'S': return e->GetShortField(o,id);
        case 'I': return e->GetIntField(o,id);
        case 'F': return e->GetFloatField(o,id);
        default: return e->GetDoubleField(o,id);
        }
    }
    rotation::Vec3 vec(jobject o) {
        if (!o) { ok = false; return {}; }
        return {number(o,"x"),number(o,"y"),number(o,"z")};
    }

};

bool Track(Frame& f,jobject packet,int id,int selfId,rotation::Vec3& pos,BacktrackPacket& out) {
    if(!f.good()||f.e->PushLocalFrame(32)!=JNI_OK)return false;
    struct LocalFrame {JNIEnv* e;~LocalFrame(){e->PopLocalFrame(nullptr);}} scope{f.e};
    bool changed=false;
    out.resetWorld |= IsDisconnectPacket(packet)||IsRespawnPacket(packet);
    out.flush |= IsPlayerPositionPacket(packet)||IsExplodePacket(packet);
    if(IsMotionPacket(packet)) {
        int entity=0,x=0,y=0,z=0;
        if(!ReadMotionPacket(packet,entity,x,y,z)) {f.ok=false;return false;}
        out.flush |= entity==selfId;
    }
    if(f.is(packet,"Health"))out.deadHealth |= f.number(packet,"health")<=0;
    if (f.is(packet,"Move") && f.number(packet,"moveId")==id) {
        if (compat::IsMinecraft263()) {
            jobject delta=f.object(packet,"delta");
            auto cc=f.type(Find("Codec")->owner), vc=f.type(Find("Vec")->owner);
            auto ctor=f.method("codecCtor"), vecCtor=f.method("vecCtor");
            auto setBase=f.method("setBase"),decode=f.method("decode");
            if (!f.good() || !ctor || !vecCtor || !setBase || !decode) return false;
            jobject codec=f.e->NewObject(cc,ctor);
            jobject base=f.e->NewObject(vc,vecCtor,pos.x,pos.y,pos.z);
            if (!f.good() || !codec || !base || !delta) return false;
            f.e->CallVoidMethod(codec,setBase,base);
            jobject path=f.e->CallObjectMethod(delta,decode,codec);
            pos=f.vec(f.call(path,"end"));
        } else {
            const double scale=compat::IsLegacyLunar()?32.0:4096.0;
            auto decode=[&](double base,double delta) {
                // 1.8 stores integral server positions; modern VecDeltaCodec uses
                // Java Math.round (floor(x + .5), including negative ties).
                return delta==0?base:(std::floor(base*scale+0.5)+delta)/scale;
            };
            pos={decode(pos.x,f.number(packet,"dx")),decode(pos.y,f.number(packet,"dy")),decode(pos.z,f.number(packet,"dz"))};
        }
        changed=true;
    } else if (f.is(packet,"Teleport") && f.number(packet,"teleId")==id) {
        if (compat::UsesModernRuntime()) pos=f.vec(f.object(f.object(packet,"change"),"position"));
        else {
            double scale=compat::IsLegacyLunar()?32.0:1.0;
            pos={f.number(packet,"tx")/scale,f.number(packet,"ty")/scale,f.number(packet,"tz")/scale};
        }
        changed=true;
    } else if (f.is(packet,"Sync") && f.number(packet,"syncId")==id) {
        // 26.3 also sends rotation-only syncs with no position path.
        jobject path=f.object(packet,"syncPosition");
        if(path) {pos=f.vec(compat::UsesGlfwModernRuntime()?f.object(path,"position"):f.call(path,"end"));changed=true;}
    }
    if (f.is(packet,"Bundle")) {
        jobject packets=f.call(packet,"bundlePackets");
        auto iterable=f.type("java.lang.Iterable"),iterator=f.type("java.util.Iterator");
        auto iterMethod=f.e->GetMethodID(iterable,"iterator","()Ljava/util/Iterator;");
        auto has=f.e->GetMethodID(iterator,"hasNext","()Z"),next=f.e->GetMethodID(iterator,"next","()Ljava/lang/Object;");
        if (!f.good() || !packets) return changed;
        jobject it=f.e->CallObjectMethod(packets,iterMethod);
        while (it && f.good() && f.e->CallBooleanMethod(it,has)) {
            jobject child=f.e->CallObjectMethod(it,next);
            changed=Track(f,child,id,selfId,pos,out)||changed;
            if(child) f.e->DeleteLocalRef(child);
        }
    }
    return changed && f.good();
}
}

bool ReadBacktrackPacket(jobject packet,int targetId,int selfId,const rotation::Vec3& base,BacktrackPacket& out) {
    Frame f;if(!f.good()||!packet)return false;
    out={};out.position=base;
    out.movedTarget=Track(f,packet,targetId,selfId,out.position,out);
    return f.good();
}
bool BacktrackBase(const EntityHandle& target,rotation::Vec3& out) {
    Frame f;if(!f.good()||!target)return false;
    auto o=static_cast<jobject>(target.raw());
    if(compat::IsLegacyLunar())out={f.number(o,"baseX")/32,f.number(o,"baseY")/32,f.number(o,"baseZ")/32};
    else out=f.vec(f.object(f.object(o,"codec"),"base"));
    return f.good();
}
void ReleaseBacktrackAccess(JNIEnv* env) {
    std::lock_guard lock(cacheLock);
    for(auto& [_,c]:classes)env->DeleteGlobalRef(c);
    classes.clear();
}
void JNICALL BacktrackAttackHook(JNIEnv*,jclass,jobject target) {
    if(IsCircuitOpen()||!target)return;
    event::AttackEntityEvent e(target);
    (void)event::EventBus::Get().Post(e);
}
}
