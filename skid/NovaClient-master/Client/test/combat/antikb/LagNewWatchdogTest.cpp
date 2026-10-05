// Real LagNewIO + cycle, AntiKB, EventBus and native output queues; only the
// game/JNI boundary is mocked. This catches mutual waiting across modules.
#include "minecraft/modules/combat/autoblock/LagNewAutoBlock.h"

namespace lagGame {
bool blocking = false, busy = false, sword = true;
bool rayHits = true, attackedInInterval = false;
int attacks = 0;
std::uint64_t movements = 0;
blink::Object manager, move{blink::Move, 10}, swing{blink::Ordinary, 20};
blink::Object hit{blink::Interact, 3}, release{blink::Ordinary, 21}, use{blink::Ordinary, 30};
blink::Object interactAt{blink::Interact, 4}, interact{blink::Interact, 5};
blink::Object ack{blink::Transaction, 31};
mock::Packet attackEvent{5, mock::Attack};
void Send(blink::Object& packet) {
    if (packet.kind == blink::Move) { ++movements; attackedInInterval = false; }
    if (&packet == &hit) attackedInInterval = true;
    if (blink::StageWatchdogPacket(&blink::environment, &manager, &packet, nullptr, false)) return;
    if (blink::HoldBlockPacket(&blink::environment, &manager, &packet, nullptr, false)) return;
    blink::environment.CallNonvirtualVoidMethod(&manager, 0, 0, &packet, nullptr);
}
}
std::uint64_t GetTickCount64() { return blink::now; }
namespace nova::rotation {
enum class HitResultType { Entity, Miss };
struct LagRay {
    HitResultType type;
    java::EntityHandle entity;
    int location = 0;
};
LagRay LeaderRayCast(Rotation, double) { return {lagGame::rayHits ? HitResultType::Entity : HitResultType::Miss, {mock::target, 42}}; }
}
namespace nova::compat::lunar {
bool EntityDead(const void*) { return !mock::alive; }
bool HoldingSword() { return lagGame::sword; }
bool BlockLagging() { return blink::blockLagActive.load(); }
bool HypixelPlayerBlocking() { return lagGame::blocking; }
bool HypixelPlayerBusy() { return lagGame::busy; }
std::uint64_t BlockInputMovementSequence() { return lagGame::movements; }
bool BlockInputAttacked() { return lagGame::attackedInInterval; }
void BeginHypixelBlockLag() { blink::BeginHypixelBlockLag(); }
bool StopSwordBlockDirect() { lagGame::Send(lagGame::release); lagGame::blocking = false; return true; }
bool SwingHand() { lagGame::Send(lagGame::swing); return true; }
bool AttackEntity(const java::EntityHandle&) {
    Check(!lagGame::blocking, "Lag New attacked while still using sword");
    if (Receive(lagGame::attackEvent, false, event::EventType::Outgoing)) return false;
    lagGame::Send(lagGame::hit); ++lagGame::attacks; return true;
}
bool StartSwordBlockDirect() { lagGame::Send(lagGame::use); lagGame::blocking = true; return true; }
bool InteractSwordBlock(const java::EntityHandle&, int) {
    lagGame::Send(lagGame::interactAt); lagGame::Send(lagGame::interact);
    return StartSwordBlockDirect();
}
}
namespace nova::module::velocity {
int attackQueue = 0;
int AttackQueue() { return attackQueue; }
}
namespace nova::module {
namespace autoblock { void SynchronizePacketDelays(bool discard = false); }
namespace lunar = compat::lunar;
// @LAG_NEW_IO@
}

int main() {
    auto module = nova::module::MakeAntiKbModule();
    nova::event::EventBus::Get().Seal();
    module->modes.at("Mode").value = 3;
    module->bools.at("Cancel On Ground").value = false;
    module->bools.at("Cancel On Attack").value = true;
    mock::legacy = mock::target = true;
    mock::ground = false;
    blink::watchdogBlocksBlink = &nova::module::antikb::BlocksAutoBlockBlink;
    blink::watchdogHoldingPackets = &nova::module::antikb::WatchdogHoldingPackets;
    blink::watchdogReplayingPackets = &nova::module::antikb::WatchdogReplayingPackets;
    mock::endSlinky = [](bool send) { blink::EndSlinkyBlockLag(send); };
    mock::endHypixel = [](bool send) { blink::EndHypixelBlockLag(send); };
    mock::slinkyLagging = [] { return blink::SlinkyBlockLagging(); };
    mock::hypixelLagging = [] { return blink::HypixelBlockLagging(); };
    mock::syncSlinky = [](bool discard) { nova::module::autoblock::SynchronizePacketDelays(discard); };
    mock::replayObserver = [](auto* packet) { if (packet->kind == mock::Ping) lagGame::Send(lagGame::ack); };
    mock::Packet kb{1, mock::Motion}, tx{2, mock::Ping}, correction{6, mock::Position};
    nova::module::LagNewAutoBlock state;
    nova::java::EntityHandle target{true, 42};
    bool queue = false, clicked = false;
    nova::module::LagNewIO io{target, queue, clicked, true, false};
    auto tick = [&] {
        blink::watchdogBeforeMovement = false;
        blink::MaintainWatchdogPhase(&blink::environment, false);
        StartTick();
        state.Pre(io);
        lagGame::Send(lagGame::move);
        state.Post(io);
        blink::now += 50;
    };
    module->setEnabled(true);
    SettleWatchdog();
    tick(); tick();
    Check(queue && state.stage == 0, "baseline Lag New never started blink");

    // Incoming S12 forcibly drains the existing output queue. Ground release
    // and optional Auto Attack are OFF, so Lag New itself must break the wait.
    Check(Receive(kb) && Receive(tx), "Watchdog did not hold the input batch");
    Check(!blink::HypixelBlockLagging(), "Lag New failed to yield output ownership");
    blink::sent.clear();
    const auto before = lagGame::attacks;
    tick();
    Check(state.active && !queue && !lagGame::blocking, "blink denial stopped release stage");
    tick();
    Check(lagGame::attacks == before + 1 && !nova::module::antikb::WatchdogHoldingPackets(),
          "Lag New and Watchdog deadlocked without optional Auto Attack");
    Check(state.stage == 7 && !lagGame::blocking, "attack frame reblocked before its physics completed");
    Check(blink::sent == std::vector<int>({21, 10, 31, 20, 3, 10}),
          "Watchdog acknowledgement split actions or attack frame included use");
    tick(); Check(queue && state.stage == 0, "blink never recovered after input release");
    Check(lagGame::blocking && blink::sent == std::vector<int>({21, 10, 31, 20, 3, 10, 30, 10}),
          "reblock failed to use its own physics/movement interval");

    // A fresh batch every cycle cannot permanently suppress attacks or leak JNI refs.
    for (int i = 0; i < 20; ++i) {
        Receive(kb); Receive(tx);
        const auto count = lagGame::attacks;
        tick(); tick(); tick();
        Check(lagGame::attacks == count + 1 && queue && mock::refs == 0,
              "repeated knockback stalled or duplicated the 7APS cycle");
    }

    // During spawn/S08 recovery, blink stays denied but combat need not freeze.
    Receive(correction);
    const auto count = lagGame::attacks;
    for (int i = 0; i < 9; ++i) tick();
    Check(lagGame::attacks == count + 3 && !queue && nova::module::antikb::BlocksAutoBlockBlink(),
          "recovery disabled combat or incorrectly reacquired blink");
    SettleWatchdog();
    for (int i = 0; i < 3; ++i) tick();
    Check(queue, "stable recovery failed to restore blink");

    // Target/input restrictions and another blink owner's FIFO remain authoritative.
    state.Reset(io);
    Check(blink::BeginPredictionBlink(), "foreign owner setup failed");
    const auto foreignCount = lagGame::attacks;
    tick();
    Check(!state.active && lagGame::attacks == foreignCount && blink::predictionBlockLag,
          "fallback stole another mode's queue");
    blink::EndPredictionBlink(false);
    nova::module::velocity::attackQueue = 1; tick();
    Check(!state.active, "fallback bypassed Velocity attack priority");
    nova::module::velocity::attackQueue = 0;
    mock::screen = true; tick(); Check(!state.active, "GUI failed to stop cycle"); mock::screen = false;
    io.manual = true; clicked = false; tick();
    Check(lagGame::attacks == foreignCount, "fallback fabricated a manual click");
    clicked = true; tick(); Check(lagGame::attacks == foreignCount + 1, "manual click stopped working");
    io.eligible = false; tick();
    Check(!state.active && !queue && !lagGame::blocking, "target loss did not clean up");

    // The ray was valid at attack but misses after physics. Do not turn that
    // into ATTACK -> USE; resume in the next movement interval without attacks.
    io.eligible = true; io.manual = false;
    blink::sent.clear();
    state.Pre(io);
    const auto attackCount = lagGame::attacks;
    lagGame::rayHits = false;
    state.Pre(io); state.Pre(io);
    Check(state.stage == 7 && !lagGame::blocking && blink::sent == std::vector<int>({20, 3}),
          "late ray miss sent a bare use after attack (PacketOrderJ)");
    lagGame::Send(lagGame::move); state.Post(io);
    tick();
    Check(lagGame::attacks == attackCount && state.stage == 0 && lagGame::blocking &&
          blink::sent == std::vector<int>({20, 3, 10, 30, 10}),
          "deferred reblock repeated the attack or failed to wait for movement");
    state.Reset(io); lagGame::Send(lagGame::move);

    // Auto Attack may emit C02 between Lag New's own miss and its reblock.
    blink::sent.clear();
    state.Pre(io); lagGame::Send(lagGame::move); lagGame::Send(lagGame::hit);
    state.Pre(io);
    Check(!lagGame::blocking && blink::sent == std::vector<int>({20, 10, 3}),
          "external attack bypassed the interaction requirement");
    lagGame::Send(lagGame::move); tick();
    Check(lagGame::blocking && state.stage == 0, "external attack prevented recovery after movement");
    state.Reset(io); lagGame::rayHits = true;
    module->setEnabled(false);
    blink::MaintainWatchdogPhase(&blink::environment, true);
    module.reset();
    Check(mock::refs == 0 && blink::refs == 0 && mock::wanted == 0, "combined cycle leaked ownership");
    std::puts("PASS: real Lag New/Watchdog cooperation, attack release, exact packet order, repeated KB, recovery and owner isolation");
}
