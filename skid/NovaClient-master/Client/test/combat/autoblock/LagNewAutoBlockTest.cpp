#include "minecraft/modules/combat/autoblock/LagNewAutoBlock.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

struct IO {
    bool eligible = true, blocking = false, beginAllowed = true, manual = false, click = false;
    bool target = true, lagging = false, blockAllowed = true;
    unsigned attacks = 0;
    std::uint64_t now = 1000;
    std::uint64_t movement = 0;
    std::string wire, pending;
    bool physicsBlocking = false;
    std::vector<bool> movementPhysics;
    bool Eligible() const { return eligible; }
    bool Blocking() const { return blocking; }
    std::uint64_t Now() const { return now; }
    std::uint64_t Movement() const { return movement; }
    void Packet(char value) {
        if (value == 'M') { ++movement; movementPhysics.push_back(physicsBlocking); }
        (lagging ? pending : wire) += value;
    }
    bool Begin() { if (!beginAllowed) return false; lagging = true; return true; }
    void Flush() { wire += pending; pending.clear(); lagging = false; }
    bool Release() { Packet('R'); blocking = false; return true; }
    bool Attack(bool& hit) {
        if (manual && !click) return false;
        assert(!blocking); Packet('S'); hit = target;
        if (hit) { Packet('A'); ++attacks; }
        click = false; return true;
    }
    bool Block(bool hit) {
        if (!blockAllowed) return false;
        if (hit) { Packet('T'); Packet('I'); }
        Packet('U'); blocking = true; return true;
    }
};
// Relevant 1.8 branches of Grim Post + PacketOrderI/J. Transactions end Post's
// observation window; only movement clears input-order flags.
struct OrderChecks {
    bool moved = false, postAction = false, right = false, release = false, attack = false;
    bool interacted = false;
    int postFlags = 0, orderFlags = 0, interactionFlags = 0;
    void Feed(char p) {
        if (p == 'M') { moved = true; postAction = right = release = attack = interacted = false; return; }
        if (p == 'Q') {
            if (moved && postAction) ++postFlags;
            moved = postAction = false; return;
        }
        if (moved) postAction = true;
        if (p == 'R') { if (right || attack) ++orderFlags; release = true; }
        if (p == 'A') { if (right || release) ++orderFlags; attack = true; }
        if (p == 'U' && attack && !interacted) ++interactionFlags;
        if (p == 'T' || p == 'I') interacted = true;
        if (p == 'T' || p == 'I' || p == 'U') { if (release) ++orderFlags; right = true; }
    }
};
int main() {
    nova::module::LagNewAutoBlock state;
    IO io;
    int slowTicks = 0;
    auto tick = [&] {
        state.Pre(io);
        io.physicsBlocking = io.blocking;
        if (io.physicsBlocking) ++slowTicks;
        io.Packet('M'); state.Post(io); io.now += 50;
    };
    tick(); assert(state.stage == 7 && io.wire == "SAM" && !io.blocking);
    tick(); assert(state.stage == 0 && io.lagging && io.wire == "SAMUM");
    tick(); assert(state.stage == 3 && io.pending == "RM" && !io.blocking);
    tick(); assert(state.stage == 7 && io.pending == "RMSAM");
    tick(); assert(state.stage == 0 && io.pending.empty() && io.wire == "SAMUMRMSAMUM");
    slowTicks = 0;
    for (int i = 0; i < 60; ++i) tick();
    assert(io.attacks == 22); // 20 more attacks across 60 game ticks; the label is 7APS.
    assert(slowTicks == 20); // Expo without NoSlow: one blocked physics tick per cycle, not two.
    io.eligible = false; tick();
    assert(!state.active && !io.lagging && !io.blocking && io.pending.empty());
    const auto stopped = io.wire; state.Reset(io); assert(io.wire == stopped);

    state = {}; io = {}; io.manual = true;
    tick(); assert(state.stage == 2 && io.attacks == 0);
    io.now = 1328; state.Pre(io); state.Post(io); assert(state.stage == 2);
    io.now = 1329; tick(); assert(state.stage == 7 && !io.blocking);
    tick(); assert(state.stage == 0 && io.attacks == 0 && io.blocking); // 329ms fallback, no unsolicited attack.
    tick(); io.click = true; tick(); assert(io.attacks == 1);
    io.eligible = false; tick(); assert(!io.lagging && !io.blocking);

    state = {}; io = {}; io.blocking = true;
    tick(); assert(state.stage == 3 && io.pending == "RM");
    io.eligible = false; tick(); assert(!io.lagging && io.pending.empty());
    state = {}; io = {}; io.blocking = true; io.beginAllowed = false;
    tick(); assert(!state.active && !io.blocking && !io.lagging && io.attacks == 0);
    state = {}; io = {}; io.blockAllowed = false;
    tick(); assert(state.active && state.stage == 7 && !io.blocking && io.attacks == 1);
    tick(); assert(state.stage == 7 && io.attacks == 1);
    io.blockAllowed = true; tick(); assert(state.stage == 0 && io.attacks == 1 && io.wire == "SAMMUM");
    state = {}; io = {}; io.target = false;
    tick(); tick(); assert(io.wire == "SMUM" && io.attacks == 0); // Miss swings don't fabricate INTERACT.

    state = {}; io = {};
    state.Pre(io); assert(io.wire == "SA" && !io.blocking && state.stage == 7);
    state.Pre(io); assert(io.wire == "SA"); // repeated input cannot spend pending attack twice
    io.now += 400; state.Pre(io); state.Post(io);
    assert(io.wire == "SA" && state.stage == 7); // skipped movement cannot reblock after attack
    io.Packet('M'); state.Post(io);
    state.Pre(io); assert(io.wire == "SAMU" && state.stage == 11);
    io.now += 400; state.Pre(io); state.Post(io);
    assert(io.wire == "SAMU" && state.stage == 11); // skipped movement cannot flush then release use
    io.Packet('M'); state.Post(io);
    state.Pre(io); assert(state.stage == 3 && io.pending == "R");
    io.now += 400; state.Pre(io); state.Post(io);
    assert(io.pending == "R" && state.stage == 3); // expired timeout cannot attack after RELEASE
    io.Packet('M'); tick(); assert(state.stage == 7 && io.attacks == 2);
    OrderChecks old;
    for (char p : std::string("SAMTIUQR")) old.Feed(p);
    assert(old.postFlags == 1 && old.orderFlags == 1); // Detect both reported failures.
    OrderChecks missingInteraction;
    for (char p : std::string("SAUM")) missingInteraction.Feed(p);
    assert(missingInteraction.interactionFlags == 1);
    state = {}; io = {};
    for (int i = 0; i < 300; ++i) {
        state.Pre(io);
        io.physicsBlocking = io.blocking;
        // Some callbacks complete without movement. The next input phase must wait.
        if (i % 7 != 0) io.Packet('M');
        state.Post(io); io.Packet('Q'); io.now += 50;
    }
    io.Flush();
    OrderChecks checks;
    for (char p : io.wire) checks.Feed(p);
    assert(checks.postFlags == 0 && checks.orderFlags == 0 && checks.interactionFlags == 0 && io.attacks > 50);
    auto mismatch = [](const std::string& wire, const std::vector<bool>& physics) {
        bool serverBlocking = false;
        std::size_t movement = 0;
        int count = 0;
        for (char p : wire) {
            if (p == 'U') serverBlocking = true;
            if (p == 'R') serverBlocking = false;
            if (p == 'M') { assert(movement < physics.size()); if (physics[movement++] != serverBlocking) ++count; }
        }
        assert(movement == physics.size());
        return count;
    };
    // Previous late-use implementation passes packet-order checks, but sends
    // a full-speed physics step under server-side use state. Detect it explicitly.
    assert(mismatch("SATIUM", {false}) == 1);
    assert(mismatch(io.wire, io.movementPhysics) == 0);
    // 420 ticks is a whole number of cycles for all five cadences.
    const unsigned expectedAttacks[] = {140, 70, 105, 210, 300};
    const int expectedSlow[] = {140, 280, 210, 105, 60};
    for (int mode = 0; mode < 5; ++mode) {
        state = {}; io = {}; int slowed = 0;
        for (int i = 0; i < 420; ++i) {
            const auto before = io.attacks;
            state.Pre(io, mode);
            state.Pre(io, mode); // duplicate input must not issue another attack
            assert(io.attacks - before <= 1);
            io.physicsBlocking = io.blocking;
            if (io.blocking) ++slowed;
            io.Packet('M'); state.Post(io); state.Post(io); io.Packet('Q'); io.now += 50;
        }
        io.Flush();
        assert(io.attacks == expectedAttacks[mode] && slowed == expectedSlow[mode]);
        OrderChecks order;
        for (char p : io.wire) order.Feed(p);
        assert(order.postFlags == 0 && order.orderFlags == 0 && order.interactionFlags == 0);
        assert(mismatch(io.wire, io.movementPhysics) == 0);
        std::cout << "APS mode " << mode << ": attacks=" << io.attacks << "/21s, slowed=" << slowed << "/420 ticks\n";
    }
    state = {}; io = {}; io.manual = true;
    for (int mode = 0; mode < 5; ++mode) {
        for (int i = 0; i < 80; ++i) {
            state.Pre(io, mode); io.physicsBlocking = io.blocking;
            io.Packet('M'); state.Post(io); io.now += 50;
        }
    }
    assert(io.attacks == 0); // APS changes and 329ms fallback never fabricate clicks.
    std::cout << "PASS: Lag New 7APS, 1/3 slowed physics ticks, server/client use-state agreement, Post/I/J and skipped updates\n";
}
