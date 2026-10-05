#include "minecraft/java/api/game_teams.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar1710/Mapping.h"

#include <array>
#include <mutex>

namespace nova::java {
namespace {
enum Key {
    Uuid, Connection, Info, Team, Name, Format, Color, Value, EntityTeam,
    Equipped, StackItem, HasCustomColor, ArmorColor, HeadSlot, ChestSlot,
    LeatherHead, LeatherChest, DyedComponent, ComponentGet, Count
};
struct Binding { const char* owner; const char* name; const char* descriptor; };
#define NOVA_TEAMS_MAPPING
#define NOVA_TEAM(key, owner, name, desc) bindings[key] = Binding{owner, name, desc};
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
#undef NOVA_TEAM
constexpr auto easecation = [] {
    auto bindings=modern;
#define NOVA_TEAM(key,owner,name,desc) bindings[key]=Binding{owner,name,desc};
#include "minecraft/compat/mappings/easecation.inc"
#undef NOVA_TEAM
    return bindings;
}();
constexpr auto neoforge12110 = [] {
    auto bindings=modern;
#define NOVA_TEAM(key,owner,name,desc) bindings[key]=Binding{owner,name,desc};
#include "minecraft/compat/mappings/neoforge12110.inc"
#undef NOVA_TEAM
    return bindings;
}();
#undef NOVA_TEAMS_MAPPING
struct Member {
    jclass owner = nullptr;
    jmethodID method = nullptr;
    jfieldID field = nullptr;
    bool resolved = false;
};
std::array<Member, Count> members;
std::mutex mutex;

struct Frame {
    JniAttach attach;
    JNIEnv* env = attach.env;
    bool valid = env && env->PushLocalFrame(32) == JNI_OK;
    bool ok = true;
    ~Frame() { if (env) check(); if (valid) env->PopLocalFrame(nullptr); }
    bool check() {
        if (!env) return false;
        if (env->ExceptionCheck()) { env->ExceptionClear(); ok = false; }
        return valid && ok;
    }
    bool resolveMember(Key key) {
        if (!check()) return false;
        auto& member = members[key];
        if (!member.resolved) {
            member.resolved = true;
            const auto binding = compat::lunar1710::Translate((compat::IsNeoForge12110()?neoforge12110:compat::UsesGlfwModernRuntime() ? easecation : compat::IsLegacyLunar() ? lunar : compat::UsesModernRuntime() ? modern : forge)[key]);
            if (!binding.owner) { ok = false; return false; }
            jobject loader = compat::ClassLoader(env);
            jclass cls = LoadClassInLoader(env, loader, binding.owner);
            if (loader) env->DeleteGlobalRef(loader);
            if (check() && cls) {
                if (binding.descriptor[0] == '(')
                    member.method = env->GetMethodID(cls, binding.name, binding.descriptor);
                else
                    member.field = env->GetStaticFieldID(cls, binding.name, binding.descriptor);
                if (check() && (member.method || member.field))
                    member.owner = static_cast<jclass>(env->NewGlobalRef(cls));
            }
            if (!check() || !member.owner) { member.method = nullptr; member.field = nullptr; }
            if (!member.owner) LogTo("Teams: unavailable mapping %s.%s%s", binding.owner, binding.name, binding.descriptor);
        }
        if (!member.owner) ok = false;
        return check();
    }
    jmethodID resolve(Key key) {
        return resolveMember(key) ? members[key].method : nullptr;
    }
    jobject staticObject(Key key) {
        if (!resolveMember(key) || !members[key].field) return nullptr;
        auto result = env->GetStaticObjectField(members[key].owner, members[key].field);
        return check() ? result : nullptr;
    }
    template<typename... A> jobject call(jobject object, Key key, A... args) {
        if (!object || !check()) return nullptr;
        const auto method = resolve(key);
        if (!method) return nullptr;
        auto result = env->CallObjectMethod(object, method, args...);
        return check() ? result : nullptr;
    }
    template<typename... A> std::optional<int> integer(jobject object, Key key, A... args) {
        if (!object || !check()) return {};
        const auto method = resolve(key);
        if (!method) return {};
        const int value = env->CallIntMethod(object, method, args...);
        return check() ? std::optional<int>(value) : std::nullopt;
    }
    std::optional<int> armorColor(jobject player, bool head) {
        jobject stack = nullptr;
        if (compat::IsLegacyLunar()) {
            // getCurrentArmor indexes inventory armor from feet (0) to head (3).
            stack = call(player, Equipped, static_cast<jint>(head ? 3 : 2));
        } else {
            auto slot = staticObject(head ? HeadSlot : ChestSlot);
            if (slot) stack = call(player, Equipped, slot);
        }
        auto item = call(stack, StackItem);
        if (!item || !check()) return {};
        if (compat::UsesModernRuntime()) {
            // Modern versions store explicit dye as a component. Default leather
            // color is not evidence of a team; restrict this to leather armor.
            auto leather = staticObject(head ? LeatherHead : LeatherChest);
            if (!leather || !check() || !env->IsSameObject(item, leather)) return {};
            auto type = staticObject(DyedComponent);
            if (!type) return {};
            auto color = call(stack, ComponentGet, type);
            return integer(color, ArmorColor);
        }
        const auto hasColor = resolve(HasCustomColor);
        if (!hasColor || !env->IsInstanceOf(item, members[HasCustomColor].owner) || !check()) return {};
        // ItemArmor.hasColor (1.8.9) also rejects non-leather materials.
        const bool dyed = env->CallBooleanMethod(item, hasColor, stack) == JNI_TRUE;
        if (!check() || !dyed) return {};
        return integer(item, ArmorColor, stack);
    }
};
}

TeamInfo ReadTeamInfo(const EntityHandle& player, bool withColor) {
    if (!player) return {};
    std::lock_guard lock(mutex);
    Frame f;
    if (!f.check()) return {};
    auto team = f.call(static_cast<jobject>(player.raw()), EntityTeam);
    if (!team) return {};
    TeamInfo result;
    auto name = static_cast<jstring>(f.call(team, Name));
    if (name) {
        const char* chars = f.env->GetStringUTFChars(name, nullptr);
        if (chars) { result.name = chars; f.env->ReleaseStringUTFChars(name, chars); }
    }
    if (!f.check()) return {};
    if (!withColor) return result;
    if (compat::IsLegacyLunar()) {
        std::string display;
        if(player.DisplayName(display))result.color=LeadingTeamColor(display);
    } else {
        auto format = f.call(team, Format);
        // 26.3 uses Optional<TeamColor>; 1.20.1 uses ChatFormatting and boxed RGB.
        auto color = compat::IsMinecraft263() ? f.call(format, Color, static_cast<jobject>(nullptr)) : f.call(format, Color);
        result.color = f.integer(color, Value);
    }
    return f.check() ? result : TeamInfo{};
}

TeamInfo ReadArmorTeamInfo(const EntityHandle& player) {
    if (!player) return {};
    std::lock_guard lock(mutex);
    Frame f;
    if (!f.check()) return {};
    TeamInfo result;
    auto entity = static_cast<jobject>(player.raw());
    result.headColor = f.armorColor(entity, true);
    result.chestColor = f.armorColor(entity, false);
    return f.check() ? result : TeamInfo{};
}

void ReleaseTeamsAccess(JNIEnv* env) {
    std::lock_guard lock(mutex);
    for (auto& member : members) {
        if (member.owner) env->DeleteGlobalRef(member.owner);
        member = {};
    }
}
}
