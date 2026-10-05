#include <cassert>
#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <atomic>
#include <chrono>
#include <cmath>
#include <format>
#include "minecraft/modules/player/hypixelautoplay/AutoPlayLogic.h"
namespace mock { long long now = 0; int interest = 2, notifications = 0; bool player = true, sendOk = true; std::vector<std::string> sent; }
namespace Properties { struct NumberProp { double value = 5; double get() const { return value; } }; }
namespace nova::event {
enum class EventType { Incoming, Outgoing, Pre, Post };
struct PacketEvent { EventType type = EventType::Incoming; int packet = 1; bool cancelled = false; bool IsCancelled() const { return cancelled; } };
struct GameTickEvent { EventType type = EventType::Pre; };
struct RespawnEvent {};
namespace Priority { constexpr int Lowest = 20; }
template<class E> std::function<void(E&)> callback;
struct Subscription { std::function<void()> reset; void Reset() { if (reset) { reset(); reset = {}; } } };
struct EventBus {
    static EventBus& Get() { static EventBus bus; return bus; }
    template<class E, class F> Subscription Subscribe(F f, int = 10) { callback<E> = f; return {[] { callback<E> = {}; }}; }
};
template<class E> void Post(E e = {}) { if (callback<E>) callback<E>(e); }
}
namespace nova::java {
void SetIncomingPacketsWanted(bool on) { mock::interest += on ? 1 : -1; }
bool IsDisconnectPacket(int p) { return p == 2; }
bool IsRespawnPacket(int p) { return p == 3; }
bool IsChatSessionStart(int p) { return p == 4; }
bool PlayerEntity() { return mock::player; }
bool ReadAutoPlayCommands(int p, std::vector<std::string>& commands) { if (p != 1) return false; commands = {"/play bedwars_eight_one"}; return true; }
bool SendPlayCommand(const std::string& c) { if (!mock::sendOk) return false; mock::sent.push_back(c); return true; }
}
namespace nova::notifications { enum class Type { Success }; void Post(std::string, Type) { ++mock::notifications; } }
namespace nova::module {
enum class Category { Player };
class Module {
public:
    Module(const char*, Category, const char*, const char*) {}
    virtual ~Module() = default;
    virtual std::string suffix() const { return {}; }
    void Enable() { onEnable(); }
    void Disable() { onDisable(); }
    void Delay(double v) { number.value = v; onUpdate(); }
    Properties::NumberProp number;
protected:
    virtual void onEnable() {} virtual void onDisable() {} virtual void onUpdate() {}
    void addMode(const char*, int, std::vector<std::string>) {}
    Properties::NumberProp& addNumber(const char*, double min, double max, double step, double value,
            std::vector<int>, std::function<std::string(double)> format) {
        assert(min == 0 && max == 30 && step == .1 && value == 5);
        assert(format(5) == "5.0 s"); number.value = value; return number;
    }
};
}
// @PRODUCTION_MODULE@
int main() {
    using namespace nova;
    using event::Post;
    auto m = module::MakeHypixelAutoPlayModule();
    Post<event::PacketEvent>(); mock::now = 6000; Post<event::GameTickEvent>();
    assert(mock::interest == 2 && mock::sent.empty());
    m->Enable(); assert(mock::interest == 3);
    mock::now = 10000; Post<event::PacketEvent>();
    mock::now = 12000; Post<event::PacketEvent>(); // Duplicate must not postpone 15s deadline.
    mock::now = 14999; Post<event::GameTickEvent>(); assert(mock::sent.empty());
    mock::now = 15000; Post<event::GameTickEvent>(); assert(mock::sent.size() == 1 && mock::notifications == 1);
    Post<event::GameTickEvent>(); assert(mock::sent.size() == 1);
    for (int boundary : {2, 3, 4}) {
        Post<event::PacketEvent>(); Post<event::PacketEvent>({event::EventType::Incoming, boundary});
        mock::now += 6000; Post<event::GameTickEvent>(); assert(mock::sent.size() == 1);
    }
    Post<event::PacketEvent>(); Post<event::RespawnEvent>(); mock::now += 6000; Post<event::GameTickEvent>(); assert(mock::sent.size() == 1);
    Post<event::PacketEvent>(); m->Disable(); m->Enable(); mock::now += 6000; Post<event::GameTickEvent>(); assert(mock::sent.size() == 1);
    Post<event::PacketEvent>(); mock::player = false; Post<event::GameTickEvent>(); mock::player = true;
    mock::now += 6000; Post<event::GameTickEvent>(); assert(mock::sent.size() == 1);
    m->Delay(0);
    Post<event::PacketEvent>({event::EventType::Outgoing, 1}); Post<event::PacketEvent>({event::EventType::Incoming, 1, true});
    Post<event::GameTickEvent>(); assert(mock::sent.size() == 1);
    Post<event::PacketEvent>(); assert(mock::sent.size() == 1); // Never send on packet callback.
    Post<event::GameTickEvent>(); assert(mock::sent.size() == 2);
    mock::sendOk = false; Post<event::PacketEvent>(); Post<event::GameTickEvent>();
    assert(mock::notifications == 2);
    m->Disable(); m->Disable(); assert(mock::interest == 2);
    m->Enable(); m.reset(); assert(mock::interest == 2 && !event::callback<event::PacketEvent>);
    module::autoplay::ReplayQueue queue;
    queue.Add({"/play a"}, 0, 5000); queue.Add({"/play b"}, 100, 0);
    assert(queue.TakeReady(100) == std::vector<std::string>{"/play b"});
    assert(queue.TakeReady(5000) == std::vector<std::string>{"/play a"});
    std::puts("PASS: production AutoPlay lifecycle, 5s delay, zero delay, deduplication, cancellation and incoming ownership");
}
