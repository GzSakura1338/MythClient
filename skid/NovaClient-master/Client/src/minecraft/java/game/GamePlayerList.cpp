#include "minecraft/java/api/game_player_list.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar1710/Access.h"

#include <array>
#include <mutex>

namespace nova::java {
namespace {
enum Key { Uuid, Connection, OnlineIds, Contains, PlayerInfo, Count };
struct Binding { const char* owner; const char* name; const char* descriptor; };
#define NOVA_PLAYER_LIST_MAPPING
#define NOVA_PLAYER_LIST(key, owner, name, descriptor) bindings[key] = {owner, name, descriptor};
constexpr auto lunar = [] {
    std::array<Binding, Count> bindings{};
#include "minecraft/compat/mappings/Lunar189.inc"
    return bindings;
}();
constexpr auto forge = [] {
    std::array<Binding, Count> bindings{};
#include "minecraft/compat/mappings/forge1201.inc"
    return bindings;
}();
constexpr auto modern = [] {
    std::array<Binding, Count> bindings{};
#include "minecraft/compat/mappings/minecraft263.inc"
    return bindings;
}();
#undef NOVA_PLAYER_LIST
#undef NOVA_PLAYER_LIST_MAPPING

struct Member { jclass owner = nullptr; jmethodID method = nullptr; bool resolved = false; };
std::array<Member, Count> members;
std::mutex mutex;

struct Frame {
    JniAttach attach;
    JNIEnv* env = attach.env;
    bool valid = env && env->PushLocalFrame(16) == JNI_OK;
    bool ok = true;
    ~Frame() { if (env) check(); if (valid) env->PopLocalFrame(nullptr); }
    bool check() {
        if (!env) return false;
        if (env->ExceptionCheck()) { env->ExceptionClear(); ok = false; }
        return valid && ok;
    }
    jmethodID resolve(Key key) {
        if (!check()) return nullptr;
        auto& member = members[key];
        if (!member.resolved) {
            member.resolved = true;
            const auto binding = compat::lunar1710::Translate((compat::IsLegacyLunar() ? lunar : compat::UsesModernRuntime() ? modern : forge)[key]);
            if (!binding.owner) { ok = false; return nullptr; }
            jobject loader = compat::ClassLoader(env);
            jclass cls = LoadClassInLoader(env, loader, binding.owner);
            if (loader) env->DeleteGlobalRef(loader);
            if (check() && cls) {
                member.method = env->GetMethodID(cls, binding.name, binding.descriptor);
                if (check() && member.method) member.owner = static_cast<jclass>(env->NewGlobalRef(cls));
            }
            if (!check() || !member.owner) member.method = nullptr;
            if (!member.method) LogTo("PlayerList: unavailable mapping %s.%s%s", binding.owner, binding.name, binding.descriptor);
        }
        if (!member.method) ok = false;
        return member.method;
    }
    template<typename... Args> jobject call(jobject object, Key key, Args... args) {
        if (!object || !check()) return nullptr;
        auto method = resolve(key);
        if (!method) return nullptr;
        auto result = env->CallObjectMethod(object, method, args...);
        return check() ? result : nullptr;
    }
};
}

std::optional<bool> OnlinePlayerListContains(const EntityHandle& entity) {
    if (!entity) return {};
    if (compat::IsLunar1710()) {
        compat::lunar1710::Frame f;
        bool available = false;
        auto info = compat::lunar1710::TabInfo(f, entity, available);
        return available ? std::optional<bool>(info != nullptr) : std::nullopt;
    }
    std::lock_guard lock(mutex);
    Frame f;
    if (!f.check()) return {};
    auto uuid = f.call(static_cast<jobject>(entity.raw()), Uuid);
    auto minecraft = compat::Minecraft(f.env);
    auto connection = f.call(minecraft, Connection);
    if (!uuid || !connection || !f.check()) return {};
    if (compat::IsLegacyLunar()) {
        // 1.8.9 exposes getPlayerInfo(UUID), backed by the same UUID map.
        // A missing entry is exactly equivalent to keySet().contains(uuid) == false.
        auto info = f.call(connection, PlayerInfo, uuid);
        return f.check() ? std::optional<bool>(info != nullptr) : std::nullopt;
    }
    auto ids = f.call(connection, OnlineIds);
    if (!ids) return {};
    auto contains = f.resolve(Contains);
    if (!contains) return {};
    const bool found = f.env->CallBooleanMethod(ids, contains, uuid) == JNI_TRUE;
    return f.check() ? std::optional<bool>(found) : std::nullopt;
}

void ReleasePlayerListAccess(JNIEnv* env) {
    if (!env) return;
    std::lock_guard lock(mutex);
    for (auto& member : members) {
        if (member.owner) env->DeleteGlobalRef(member.owner);
        member = {};
    }
}
}
