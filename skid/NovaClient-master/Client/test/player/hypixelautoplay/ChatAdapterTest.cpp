#include <cassert>
#include <cstdio>
#include <deque>
#include <string>
#include <type_traits>
#include <vector>
#include "minecraft/modules/player/hypixelautoplay/AutoPlayLogic.h"
struct Node {
    std::string type, text;
    bool overlay = false;
    Node *component = nullptr, *click = nullptr, *action = nullptr;
    std::vector<Node*> children;
};
using jobject = Node*;
namespace mock {
int profile = 0;
bool playerPresent = true, sendOk = true;
std::deque<Node> objects;
Node* Object(std::string type = {}, std::string text = {}) { objects.push_back({type, text}); return &objects.back(); }
Node *client = Object(), *connection = Object(), *player = Object();
std::vector<std::string> sent;
struct Env {
    jobject NewStringUTF(const char* text) { return Object("string", text); }
    void DeleteLocalRef(jobject) {}
} env;
}
namespace nova::compat {
bool IsLegacyLunar() { return mock::profile == 0; }
bool IsLunar1710() { return false; }
bool IsMinecraft263() { return mock::profile == 2; }
bool UsesModernRuntime() { return IsMinecraft263(); }
namespace mc263::access {
struct Member { const char *owner = "", *name = "", *descriptor = ""; };
struct Frame {
    mock::Env* env;
    Frame(mock::Env* supplied = nullptr) : env(supplied ? supplied : &mock::env) {}
    explicit operator bool() const { return true; }
    bool check() const { return true; }
    jobject client() { return mock::client; }
    jobject player() { return mock::playerPresent ? mock::player : nullptr; }
    bool instance(jobject o, const char* type) { return o && type && o->type == type; }
    std::string text(jobject o) { return o ? o->text : ""; }
    template<class T = jobject, class... A> T call(jobject o, Member m, A... args) {
        if (!o) return {};
        if constexpr (std::is_same_v<T, bool>) return o->overlay;
        else if constexpr (std::is_same_v<T, int>) return static_cast<int>(o->children.size());
        else {
            const std::string name = m.name;
            if constexpr (sizeof...(args) == 1) {
                int index = (args, ...);
                return o->children.at(index);
            }
            if (name == "getChatComponent" || name == "f_237849_" || name == "content") return o->component;
            if (name == "getSiblings" || name == "m_7360_" || name == "getChatStyle" || name == "m_7383_" || name == "getStyle") return o;
            if (name == "getChatClickEvent" || name == "m_131182_" || name == "getClickEvent") return o->click;
            if (name == "getAction" || name == "m_130622_") return o->action;
            if (name == "getConnection" || name == "m_91403_") return mock::connection;
            if (name == "getFormattedText" || name == "getString" || name == "name" || name == "getValue" || name == "m_130623_" || name == "command") return o;
            assert(false); return {};
        }
    }
    jobject create(Member m, jobject value) {
        assert(std::string(m.owner) == "net.minecraft.network.play.client.C01PacketChatMessage");
        return mock::Object(m.owner, value->text);
    }
    bool invoke(jobject object, Member m, jobject value) {
        assert(object == mock::connection);
        assert(std::string(m.name) == (mock::profile == 1 ? "m_246623_" : "sendCommand"));
        if (!mock::sendOk) return false;
        mock::sent.push_back(value->text); return true;
    }
};
}
}
namespace nova::java {
bool SendPacket(jobject p) {
    assert(p->type == "net.minecraft.network.play.client.C01PacketChatMessage");
    if (!mock::sendOk) return false;
    mock::sent.push_back(p->text); return true;
}
}
// @PRODUCTION_CHAT@
int main() {
    using namespace nova;
    const std::string command = "/play bedwars_eight_one";
    for (mock::profile = 0; mock::profile < 3; ++mock::profile) {
        auto packet = mock::Object(mock::profile == 0 ? "net.minecraft.network.play.server.S02PacketChat" : "net.minecraft.network.protocol.game.ClientboundSystemChatPacket");
        auto root = mock::Object("component", "Want to play again? Click here!");
        auto wrapper = mock::Object("component");
        auto child = mock::Object("component");
        auto click = mock::Object(mock::profile == 2 ? "net.minecraft.network.chat.ClickEvent$RunCommand" : "click", command);
        click->action = mock::Object("enum", "RUN_COMMAND");
        child->click = click; wrapper->children = {child}; root->children = {wrapper}; packet->component = root;
        std::vector<std::string> commands;
        assert(java::ReadAutoPlayCommands(packet, commands) && commands == std::vector<std::string>{command});
        packet->overlay = true; assert(!java::ReadAutoPlayCommands(packet, commands) && commands.empty()); packet->overlay = false;
        root->text = "normal chat"; assert(!java::ReadAutoPlayCommands(packet, commands)); root->text = "play again?";
        for (auto invalid : {"/party invite user", "/player test", "/play", "/play bad.mode", "/play a\n/help"}) {
            click->text = invalid; assert(!java::ReadAutoPlayCommands(packet, commands)); assert(!java::SendPlayCommand(invalid));
        }
        click->text = command;
        if (mock::profile == 2) click->type = "net.minecraft.network.chat.ClickEvent$SuggestCommand";
        else click->action->text = "SUGGEST_COMMAND";
        assert(!java::ReadAutoPlayCommands(packet, commands));
        if (mock::profile == 2) click->type = "net.minecraft.network.chat.ClickEvent$RunCommand";
        else click->action->text = "RUN_COMMAND";
        root->children.clear(); root->click = click; // Rise only scans siblings.
        assert(!java::ReadAutoPlayCommands(packet, commands));
        assert(java::SendPlayCommand(command));
        assert(mock::sent.back() == (mock::profile == 0 ? command : command.substr(1)));
        mock::playerPresent = false; assert(!java::SendPlayCommand(command)); mock::playerPresent = true;
        mock::sendOk = false; assert(!java::SendPlayCommand(command)); mock::sendOk = true;
        auto join = mock::Object(mock::profile == 0 ? "net.minecraft.network.play.server.S01PacketJoinGame" : "net.minecraft.network.protocol.game.ClientboundLoginPacket");
        assert(java::IsChatSessionStart(join) && !java::IsChatSessionStart(packet));
    }
    assert(mock::sent.size() == 3);
    std::puts("PASS: production chat adapter: nested replay links, overlays, non-command links, command validation, C01 and modern send paths");
}
