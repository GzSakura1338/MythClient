#include "minecraft/java/api/game_chat.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/modules/player/hypixelautoplay/AutoPlayLogic.h"

namespace nova::java {
namespace {
using compat::mc263::access::Frame;
using compat::mc263::access::Member;
struct Mapping {
    const char* packet = nullptr;
    const char* runCommand = nullptr;
    const char* join = nullptr;
    Member overlay{}, content{}, text{}, siblings{}, style{}, click{}, action{}, value{};
    Member connection{}, send{}, chatPacket{};
};
#define NOVA_AUTOPLAY_MAPPING
#define NOVA_AUTOPLAY_CLASS(id, name) .id = name,
#define NOVA_AUTOPLAY(id, owner, name, desc) .id = {owner, name, desc},
constexpr Mapping lunar{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr Mapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr Mapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_AUTOPLAY
#undef NOVA_AUTOPLAY_CLASS
#undef NOVA_AUTOPLAY_MAPPING
const Mapping& Names() { return compat::IsLegacyLunar() ? lunar : compat::UsesModernRuntime() ? modern : forge; }
constexpr Member listSize{"java.util.List", "size", "()I"};
constexpr Member listGet{"java.util.List", "get", "(I)Ljava/lang/Object;"};

// Read click events directly; modern record toString() no longer quotes values.
// Find the first replay command per sibling, matching Rise's inner-loop break.
bool FindCommand(Frame& parent, const Mapping& m, jobject component, std::string& out, int depth, int& budget) {
    if (!component || depth > 32 || --budget < 0) return false;
    Frame f(parent.env);
    if (!f) return false;
    auto style = f.call(component, m.style);
    auto click = f.call(style, m.click);
    if (!f.check()) return false;
    if (click) {
        bool runs = false;
        if (compat::UsesModernRuntime()) runs = f.instance(click, m.runCommand);
        else {
            auto action = f.call(click, m.action);
            runs = f.text(f.call(action, {"java.lang.Enum", "name", "()Ljava/lang/String;"})) == "RUN_COMMAND";
        }
        if (runs) {
            auto command = f.text(f.call(click, m.value));
            if (f.check() && module::autoplay::IsPlayCommand(command)) { out = std::move(command); return true; }
        }
    }
    auto siblings = f.call(component, m.siblings);
    const int count = f.call<int>(siblings, listSize);
    for (int i = 0; i < count && budget > 0 && f; ++i) {
        auto child = f.call(siblings, listGet, i);
        const bool found = FindCommand(f, m, child, out, depth + 1, budget);
        if (child) f.env->DeleteLocalRef(child);
        if (found) return true;
    }
    return false;
}
}

bool IsChatSessionStart(jobject packet) {
    Frame f;
    return f && f.instance(packet, Names().join);
}

bool ReadAutoPlayCommands(jobject packet, std::vector<std::string>& commands) {
    commands.clear();
    Frame f;
    if (!f || !packet) return false;
    const auto& m = Names();
    if (!f.instance(packet, m.packet) || (!compat::IsLunar1710() && f.call<bool>(packet, m.overlay)) || !f.check()) return false;
    auto component = f.call(packet, m.content);
    auto text = f.text(f.call(component, m.text));
    if (!f.check() || !module::autoplay::IsReplayPrompt(text)) return false;
    auto siblings = f.call(component, m.siblings);
    const int count = f.call<int>(siblings, listSize);
    int budget = 1024;
    for (int i = 0; i < count && budget > 0 && commands.size() < 64 && f; ++i) {
        auto child = f.call(siblings, listGet, i);
        std::string command;
        if (FindCommand(f, m, child, command, 0, budget)) commands.push_back(std::move(command));
        if (child) f.env->DeleteLocalRef(child);
    }
    if (!f.check()) commands.clear();
    return !commands.empty();
}

bool SendPlayCommand(const std::string& command) {
    if (!module::autoplay::IsPlayCommand(command)) return false;
    Frame f;
    if (!f || !f.player()) return false;
    const auto& m = Names();
    // 1.8 uses C01; modern command APIs take the command without the slash.
    auto text = f.env->NewStringUTF(command.c_str() + (compat::IsLegacyLunar() ? 0 : 1));
    if (!text || !f.check()) return false;
    if (compat::IsLegacyLunar()) {
        auto packet = f.create(m.chatPacket, text);
        return packet && f.check() && SendPacket(packet);
    }
    auto connection = f.call(f.client(), m.connection);
    return connection && f.invoke(connection, m.send, text);
}
}
