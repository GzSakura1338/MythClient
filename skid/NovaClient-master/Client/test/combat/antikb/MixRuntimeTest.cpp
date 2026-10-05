#include "minecraft/modules/combat/antikb/mix/MixVelocity.h"
#include "minecraft/events/EventBus.h"
#include <atomic>
#include <cassert>
#include <cmath>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct Packet { int serial = 0, kind = 0; bool motion = false; int id = 7, y = 100; int action = 0; };
using jobject = Packet*;
namespace mock {
bool ground = false, liquid = false, dead = false, failRef = false, forward = true, boost = false;
bool sprint = true, blocking = false, aura = false, ab = false, blockTarget = false;
bool cycle = false, hypixel = false, canReduce = true, busy = false, slotReady = true, attacked = false;
bool legacy = true, swingSuccess = true, attackSuccess = true;
int refs = 0, world = 1, player = 7, hover = 8, jumps = 0, attackedId = 0;
std::string actions;
std::vector<int> handled;
std::function<void(jobject)> replay;
std::vector<std::pair<int, double>> entities;
}
struct Env {
    jobject NewGlobalRef(jobject p) { if (mock::failRef) return nullptr; ++mock::refs; return p; }
    void DeleteGlobalRef(jobject) { --mock::refs; }
    bool ExceptionCheck() { return false; }
    void ExceptionClear() {}
};
struct JniAttach { Env storage; Env* env = &storage; explicit operator bool() const { return true; } };
// @EVENTS@
namespace nova::rotation { struct Vec3 { double x = 1, y = .42, z = 2; }; }
namespace mock { nova::rotation::Vec3 motion; }
namespace nova::java {
enum { kTypeLiving = 1, kTypePlayer = 2, kTypeMonster = 4, kTypeAnimal = 8, kTypeVillager = 16 };
struct EntityHandle {
    int id = 0; double distance = 0;
    explicit operator bool() const { return id != 0; }
    bool IsSame(const EntityHandle& other) const { return id == other.id; }
    void* raw() const { return nullptr; }
    void Reset() { id = 0; }
    bool EyePosition(float, rotation::Vec3&) const { return bool(*this); }
};
bool EntityId(const EntityHandle& e, int& out) { out = e.id; return bool(e); }
bool EntityTypes(const EntityHandle& e, unsigned& out) { out = kTypeLiving | kTypePlayer; return bool(e); }
bool PacketIsMovePlayer(jobject p) { return p->action == 1; }
bool PacketIsInteract(jobject p) { return p->action == 2; }
bool PacketIsUseItem(jobject p) { return p->action == 3; }
bool PacketIsUseItemOn(jobject p) { return p->action == 4; }
bool PacketIsPlayerAction(jobject p) { return p->action == 5; }
bool PacketIsContainerClose(jobject p) { return p->action == 6; }
}
namespace nova::rotation { double EntityDistance(const Vec3&, const java::EntityHandle& e) { return e.distance; } }
namespace nova::compat {
bool IsLegacyLunar() { return mock::legacy; }
namespace lunar {
java::EntityHandle LevelEntity() { return {mock::world}; }
java::EntityHandle PlayerEntity() { return {mock::player}; }
bool EntityDead(void*) { return mock::dead; }
bool PlayerOnGround() { return mock::ground; }
bool PredictionLiquidWeb() { return mock::liquid; }
bool PredictionForwardPressed() { return mock::forward; }
bool PredictionJumpBoost() { return mock::boost; }
void PredictionJump() { ++mock::jumps; }
bool PlayerSprinting(bool& out) { out = mock::sprint; return true; }
void SetPlayerSprinting(bool value) { mock::sprint = value; }
bool HypixelPlayerBlocking() { return mock::blocking; }
bool HypixelPlayerBusy() { return mock::busy; }
bool HeldSlotChangeReady() { return mock::slotReady; }
bool BlockInputAttacked() { return mock::attacked; }
bool AttackTargetEntity(java::EntityHandle& e) { e = {mock::hover}; return bool(e); }
bool WorldLoadedEntities(std::vector<java::EntityHandle>& out) {
    for (auto [id, distance] : mock::entities) out.push_back({id, distance}); return true;
}
bool SendSwingPacket() { mock::actions += 'S'; return mock::swingSuccess; }
bool SendAttackPacket(const java::EntityHandle& e) { mock::actions += 'A'; mock::attackedId = e.id; return mock::attackSuccess; }
bool PlayerDeltaMovement(rotation::Vec3& out) { out = mock::motion; return true; }
void SetPlayerDeltaMovement(double x, double y, double z) { mock::motion = {x,y,z}; }
int PredictionIncomingKind(jobject p) { return p->kind; }
bool ReadMotionPacket(jobject p, int& id, int&, int& y, int&) { id = p->id; y = p->y; return p->motion; }
void HandleIncomingPacket(jobject p) { mock::handled.push_back(p->serial); if (mock::replay) mock::replay(p); }
}
}
namespace nova::module::killaura {
bool IsEnabled() { return mock::aura; }
bool HasBlockTarget() { return mock::blockTarget; }
}
namespace nova::module::autoblock {
bool Active() { return mock::ab; }
bool AttackCycleActive() { return mock::cycle; }
bool HypixelActive() { return mock::hypixel; }
bool VelocityCanReduce(int) { return mock::canReduce; }
}
// @RUNTIME@

int main() {
    using namespace nova;
    using namespace nova::module::mix;
    Initialize(); Configure(true, {});
    auto& r = *runtime;
    auto& bus = event::EventBus::Get();
    bus.Seal();
    auto receive = [&](Packet& p) { event::PacketEvent e(event::EventType::Incoming, &p); (void)bus.Post(e); return e.IsCancelled(); };
    auto pre = [&] { event::PrePlayerInteractEvent e; (void)bus.Post(e); };
    auto post = [&] { event::PostPlayerTickEvent e; (void)bus.Post(e); };
    auto living = [&] { event::LivingUpdateEvent e; (void)bus.Post(e); };
    auto outgoing = [&](int action) { Packet p; p.action = action; event::PacketEvent e(event::EventType::Outgoing, &p); (void)bus.Post(e); };
    mock::replay = [&](jobject p) {
        assert(OwnsIncoming()); // Ownership survives the last packet's callback.
        if (p->motion) { event::AppliedVelocityEvent e(0, .4, 0); (void)bus.Post(e); }
    };
    Packet hurt{1,3}, kb{2,0,true}, ordinary{3}, keep{4,1}, correction{5,6}, transaction{6,7};
    assert(!receive(kb) && !r.state.received); // Fake check requires S19 opcode 2.
    assert(!receive(hurt) && receive(kb) && OwnsIncoming() && mock::refs == 1);
    assert(receive(ordinary) && !receive(keep));
    for (int i=0; i<8; ++i) post();
    assert(OwnsIncoming() && mock::handled.empty()); // Air buffer does not expire at Delay Ticks.
    mock::ground = true; post();
    assert(!OwnsIncoming() && mock::refs == 0 && (mock::handled == std::vector<int>{2,3}));
    assert(r.state.released && r.state.reduceCounts == 3 && r.state.allowNext);
    living(); living(); assert(mock::jumps == 1 && !r.state.received);
    pre(); assert(mock::actions == "SA" && !mock::sprint && mock::motion.x == .6 && mock::motion.y == .42 && mock::motion.z == 1.2);
    pre(); pre(); assert(mock::actions == "SA" && r.state.reduceCounts == 0); // No automatic sprint restoration.
    mock::actions.clear(); mock::sprint = true;

    r.Reset(false); mock::ground = false; r.settings.airBuffer = false;
    receive(hurt); assert(receive(kb)); post(); assert(OwnsIncoming()); post(); assert(!OwnsIncoming());
    assert(r.state.reduceCounts == 3); mock::blocking = true; pre(); assert(r.state.reduceCounts == 2 && mock::actions.empty());
    mock::blocking = false; mock::hover = 0; pre(); assert(r.state.reduceCounts == 1);
    mock::hover = 8; mock::aura = true; mock::canReduce = false; pre(); assert(mock::actions.empty() && r.state.reduceCounts == 0);
    mock::aura = false; mock::canReduce = true;

    r.state.reduceCounts = 1; mock::cycle = true; pre(); assert(mock::actions.empty()); // Lag New owns its cycle.
    mock::cycle = false; r.settings.noBadPackets = true;
    outgoing(3); r.state.reduceCounts = 1; pre(); assert(mock::actions.empty());
    outgoing(1); r.state.reduceCounts = 1; pre(); assert(mock::actions == "SA");
    mock::actions.clear(); mock::sprint = true;
    r.settings.airPush = true; mock::hover = 0; mock::entities = {{7,100}, {8,10.9}, {9,11}, {10,50}, {11,20}};
    r.state.reduceCounts = 1; pre(); assert(mock::attackedId == 10 && mock::actions == "SA");
    mock::actions.clear(); mock::sprint = true; mock::hover = 8;
    mock::attackSuccess = false; const auto prior = mock::motion;
    r.state.reduceCounts = 1; pre(); assert(mock::motion.x == prior.x && mock::sprint); mock::attackSuccess = true;

    r.Reset(false); r.settings = {}; mock::ground = true;
    receive(hurt); assert(!receive(kb) && r.state.released && r.state.reduceCounts == 3); // Immediate on ground.
    r.Reset(false); r.settings.groundDelay = true;
    receive(hurt); assert(receive(kb)); post(); assert(!OwnsIncoming()); // airBuffer ground release overrides groundDelay.
    r.Reset(false); r.settings = {}; mock::ground = false; mock::liquid = true;
    receive(hurt); assert(!receive(kb)); mock::liquid = false;
    r.Reset(false); receive(hurt); assert(receive(kb)); mock::liquid = true; post(); assert(!OwnsIncoming()); mock::liquid = false;
    for (int y : {0,-1}) { r.Reset(false); receive(hurt); kb.y = y; assert(!receive(kb) && !r.state.received); }
    kb.y = 100; r.Reset(false); kb.id = 20; receive(hurt); assert(!receive(kb)); kb.id = 7;

    r.Reset(false); r.settings.fakeCheck = false; assert(receive(kb));
    receive(ordinary); auto size = mock::handled.size();
    assert(!receive(correction) && !OwnsIncoming() && mock::handled.size() == size+2 && r.state.reduceCounts == 0);
    assert(receive(kb)); assert(!receive(transaction) && !OwnsIncoming() && r.state.reduceCounts == 3);
    for (int kind : {2,5}) {
        assert(receive(kb)); size = mock::handled.size(); Packet transition{90,kind};
        assert(!receive(transition) && mock::refs == 0 && mock::handled.size() == size && !OwnsIncoming());
    }
    assert(receive(kb)); ++mock::world; receive(ordinary); assert(!OwnsIncoming() && mock::refs == 0);
    assert(receive(kb)); ++mock::player; receive(ordinary); assert(!OwnsIncoming() && mock::refs == 0); mock::player = 7;
    r.Ready(); assert(receive(kb)); mock::dead = true; pre(); assert(!OwnsIncoming()); mock::dead = false;
    mock::failRef = true; assert(!receive(kb) && !OwnsIncoming()); mock::failRef = false;
    assert(receive(kb)); mock::failRef = true; assert(!receive(ordinary) && !OwnsIncoming()); mock::failRef = false;
    assert(receive(kb)); size = mock::handled.size(); Configure(false, {});
    assert(!OwnsIncoming() && mock::refs == 0 && mock::handled.size() == size+1 && !r.active);
    Configure(true, {}); assert(r.state.reduceCounts == 0 && r.state.allowNext);
    receive(hurt); assert(receive(kb)); ++mock::world; size = mock::handled.size(); Configure(false, {});
    assert(mock::handled.size() == size && mock::refs == 0); // Disable in a replaced world discards.
    mock::legacy = false; Configure(true, {}); assert(!r.active); mock::legacy = true;
    Configure(true, {}); receive(hurt); assert(receive(kb)); Shutdown(); assert(mock::refs == 0 && !OwnsIncoming());
    std::cout << "PASS: Mix production runtime/events, hurt gate, delay/release, raw attacks/friction, jump, AB gate, air push, FIFO and lifecycle\n";
}
