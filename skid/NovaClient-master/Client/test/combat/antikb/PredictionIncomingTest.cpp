#include "minecraft/modules/combat/antikb/prediction/PredictionVelocity.h"
#include <atomic>
#include <cassert>
#include <cstdarg>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <vector>
struct Packet { int serial = 0, kind = 0; bool motion = false; int id = 7; };
using jobject = Packet*;
bool failRef = false, ground = false, liquid = false, dead = false, stuck = false, blink = false;
unsigned long long nowMillis = 0;
unsigned long long GetTickCount64() { return nowMillis; }
int refs = 0, worldId = 1;
std::vector<int> handled;
std::function<void(jobject)> replay;
struct Env {
    jobject NewGlobalRef(jobject p) { if (failRef) return nullptr; ++refs; return p; }
    void DeleteGlobalRef(jobject) { --refs; }
    bool ExceptionCheck() { return false; }
    void ExceptionClear() {}
};
struct JniAttach { Env storage; Env* env = &storage; explicit operator bool() const { return true; } };
void LogTo(const char*, ...) {}
namespace nova::rotation { struct Rotation { float yaw = 0, pitch = 0; }; }
namespace nova::java {
struct EntityHandle {
    int id = 0;
    explicit operator bool() const { return id != 0; }
    bool IsSame(const EntityHandle& other) const { return id == other.id; }
    void* raw() const { return nullptr; }
    void Reset() { id = 0; }
};
bool EntityId(const EntityHandle& p, int& out) { out = p.id; return bool(p); }
}
namespace nova::event {
enum class EventType { Incoming, Outgoing };
struct Subscription {};
struct PacketEvent {
    EventType type = EventType::Incoming;
    jobject packet;
    bool cancelled = false;
    bool IsCancelled() const { return cancelled; }
    void Cancel() { cancelled = true; }
};
}
namespace lunar {
nova::java::EntityHandle LevelEntity() { return {worldId}; }
nova::java::EntityHandle PlayerEntity() { return {worldId ? 7 : 0}; }
bool EntityDead(void*) { return dead; }
void EndPredictionBlink(bool) { blink = false; }
bool PredictionBlinking() { return blink; }
bool PlayerOnGround() { return ground; }
bool PredictionLiquidWeb() { return liquid; }
int PredictionIncomingKind(jobject p) { return p->kind; }
bool ReadMotionPacket(jobject p, int& id, int&, int&, int&) { id = p->id; return p->motion; }
bool HandleIncomingPacket(jobject p) { handled.push_back(p->serial); if (replay) replay(p); return true; }
}
struct Module { bool isEnabled() const { return stuck; } };
struct ModuleManager {
    static ModuleManager& Get() { static ModuleManager m; return m; }
    Module* Find(const char*) { static Module m; return &m; }
};
using namespace nova;
using namespace nova::module;
std::atomic<bool> ownsIncoming{false}, stopped{false}, cancel{false};
// @INCOMING_PRODUCTION@
int main() {
    Runtime r;
    r.active = true; r.settings.delay = true;
    auto receive = [&](Packet& packet) {
        event::PacketEvent e{event::EventType::Incoming, &packet};
        r.Packet(e); return e.cancelled;
    };
    Packet hurt{1,3}, velocity{2,0,true}, ordinary{3}, keepalive{4,1}, otherStatus{5,4};
    assert(!receive(hurt));
    assert(receive(velocity) && ownsIncoming && refs == 1);
    assert(receive(ordinary) && !receive(keepalive) && !receive(otherStatus));
    assert(receive(hurt)); // own status is held while the delay owns the stream
    replay = [&](jobject p) { if (p->motion) r.state.Applied(r.settings,.4,.42,0,false); };
    { std::lock_guard lock(r.mutex); r.Drain(true); r.state.delayFlag = false; }
    assert((handled == std::vector<int>{2,3,1}) && refs == 0 && !ownsIncoming);
    assert(r.state.hitCount == 3 && !r.state.knockback); // replay bypasses PacketEvent
    receive(hurt); assert(receive(velocity));
    Packet respawn{6,2}; assert(!receive(respawn));
    assert(handled.back() == 2 && refs == 0 && !ownsIncoming && !r.state.delayFlag);
    receive(hurt); assert(receive(velocity));
    Packet disconnect{7,5}; assert(!receive(disconnect));
    assert(refs == 0 && !ownsIncoming && handled.size() == 4); // disconnected queue is discarded
    receive(hurt); stuck = true;
    assert(!receive(velocity) && !ownsIncoming); stuck = false;
    receive(hurt); failRef = true;
    assert(!receive(velocity) && refs == 0 && !ownsIncoming); failRef = false;
    receive(hurt); assert(receive(velocity));
    ++worldId; receive(ordinary);
    assert(refs == 0 && !ownsIncoming && handled.size() == 4); // old-world queue cannot leak
    receive(hurt); assert(receive(velocity)); dead = true; receive(ordinary); dead = false;
    assert(refs == 0 && !ownsIncoming);
    r.Reset(false);
    receive(hurt); assert(receive(velocity));
    nowMillis += 499; assert(receive(ordinary));
    nowMillis += 1; assert(!receive(ordinary));
    assert(refs == 0 && !ownsIncoming && !r.state.delayFlag);
    for (int kind : {6, 7}) {
        receive(hurt); assert(receive(velocity)); assert(receive(ordinary));
        Packet critical{90, kind}; assert(!receive(critical));
        assert(refs == 0 && !ownsIncoming && handled.back() == 3);
    }
    receive(hurt); assert(receive(velocity)); nowMillis += 500;
    assert(r.ReleaseExpiredDelay() && refs == 0 && !ownsIncoming);
    receive(hurt); assert(receive(velocity)); r.settings.delay = false;
    assert(r.ReleaseExpiredDelay() && !receive(velocity) && refs == 0);
    std::cout << "PASS: production incoming FIFO, keepalive/status exceptions, hurt replay, allocation failure, Stuck and world/death/disconnect cleanup\n";
}
