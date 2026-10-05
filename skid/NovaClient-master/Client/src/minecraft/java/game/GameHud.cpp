#include "minecraft/java/api/game_hud.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar1710/Access.h"
#include <mutex>
#include <string_view>
#include <unordered_map>

namespace nova::java {
namespace {
struct Binding { const char* key; const char* owner; const char* name; const char* desc; };
#define NOVA_HUD_MAPPING
#define NOVA_HUD(key, owner, name, desc) {key, owner, name, desc},
const Binding lunar[]{
#include "minecraft/compat/mappings/Lunar189.inc"
};
const Binding forge[]{
#include "minecraft/compat/mappings/forge1201.inc"
};
const Binding modern[]{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_HUD
#undef NOVA_HUD_MAPPING
struct Member { jclass owner = nullptr; jmethodID method = nullptr; jfieldID field = nullptr; };
std::mutex mutex;
std::unordered_map<std::string, Member> members;
bool Good(JNIEnv* env) {
    if (!env->ExceptionCheck()) return true;
    env->ExceptionClear(); return false;
}
Member Resolve(JNIEnv* env, const char* key) {
    auto found = members.find(key);
    if (found != members.end()) return found->second;
    const auto& table = compat::IsLegacyLunar() ? lunar : compat::UsesModernRuntime() ? modern : forge;
    const Binding* binding = nullptr;
    for (const auto& entry : table) if (std::string_view(entry.key) == key) binding = &entry;
    if (!binding) return {};
    Binding translated=*binding;
    auto symbol=compat::lunar1710::Translate(compat::mc263::names::Member{binding->owner,binding->name,binding->desc});
    translated.owner=symbol.owner; translated.name=symbol.name; translated.desc=symbol.descriptor;
    binding=&translated;
    jobject loader = compat::ClassLoader(env);
    jclass cls = LoadClassInLoader(env, loader, binding->owner);
    if (loader) env->DeleteGlobalRef(loader);
    if (!Good(env) || !cls) return {};
    Member result;
    if (binding->desc[0] == '(') result.method = env->GetMethodID(cls, binding->name, binding->desc);
    else result.field = env->GetFieldID(cls, binding->name, binding->desc);
    if (Good(env) && (result.method || result.field)) {
        result.owner = static_cast<jclass>(env->NewGlobalRef(cls));
        if (Good(env) && result.owner) members.emplace(key, result);
    }
    env->DeleteLocalRef(cls);
    return result;
}
jobject Call(JNIEnv* env, jobject object, const char* key) {
    if (!object) return nullptr;
    const auto member = Resolve(env, key);
    if (!member.method) return nullptr;
    jobject value = env->CallObjectMethod(object, member.method);
    return Good(env) ? value : nullptr;
}
struct Frame {
    JNIEnv* env;
    bool valid;
    explicit Frame(JNIEnv* env) : env(env), valid(env->PushLocalFrame(32) == JNI_OK) { Good(env); }
    ~Frame() { if (valid) env->PopLocalFrame(nullptr); }
};
}

HudInfo ReadHudInfo() {
    HudInfo result;
    JniAttach attach;
    if (!attach) return result;
    JNIEnv* env = attach.env;
    Frame frame(env);
    if (!frame.valid) return result;
    std::lock_guard lock(mutex);
    // Resolve the packet type on the game thread, never once per incoming packet.
    Resolve(env, "time");
    jobject mc = compat::Minecraft(env), player = compat::LocalPlayer(env);
    if (!mc || !player) return result;
    const auto local = Resolve(env, "local");
    const bool single = local.method && env->CallBooleanMethod(mc, local.method);
    if (!Good(env)) return result;
    if (single) result.server = "Singleplayer";
    else if (jobject server = Call(env, mc, "server")) {
        const auto address = Resolve(env, "address");
        auto value = address.field ? static_cast<jstring>(env->GetObjectField(server, address.field)) : nullptr;
        if (Good(env) && value) {
            const char* chars = env->GetStringUTFChars(value, nullptr);
            if (chars) { result.server = chars; env->ReleaseStringUTFChars(value, chars); }
            Good(env);
        }
    }
    if (compat::IsLunar1710()) {
        compat::lunar1710::Frame f(env);
        auto self=PlayerEntity(); bool available=false;
        auto info=compat::lunar1710::TabInfo(f,self,available);
        if (available && info) {
            const int ping=f.get<int>(info,compat::lunar1710::names::Ping);
            if (f && ping>=0) result.ping=ping;
        }
        return result;
    }
    jobject uuid = Call(env, player, "uuid"), connection = Call(env, mc, "connection");
    const auto lookup = Resolve(env, "info");
    jobject info = uuid && connection && lookup.method ? env->CallObjectMethod(connection, lookup.method, uuid) : nullptr;
    if (Good(env) && info) {
        const auto latency = Resolve(env, "latency");
        if (latency.method) {
            const int ping = env->CallIntMethod(info, latency.method);
            if (Good(env) && ping >= 0) result.ping = ping;
        }
    }
    return result;
}

bool ReadHudServerTime(jobject packet, long long& ticks) {
    JniAttach attach;
    if (!attach || !packet) return false;
    std::lock_guard lock(mutex);
    const auto it = members.find("time");
    if (it == members.end() || !it->second.owner || !it->second.method) return false;
    JNIEnv* env = attach.env;
    if (!env->IsInstanceOf(packet, it->second.owner)) return false;
    ticks = env->CallLongMethod(packet, it->second.method);
    return Good(env);
}

void ReleaseHudAccess(JNIEnv* env) {
    std::lock_guard lock(mutex);
    for (const auto& [key, member] : members) if (member.owner) env->DeleteGlobalRef(member.owner);
    members.clear();
}
}
