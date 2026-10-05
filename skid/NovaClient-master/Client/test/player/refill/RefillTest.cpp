#include "minecraft/modules/player/refill/RefillLogic.h"
#include <array>
#include <cassert>
#include <cstdio>

using nova::module::refill::Move;
using nova::module::refill::Session;
struct IO {
    // 0 empty, 1 healing splash potion, 2 other item.
    std::array<int, 36> slots{};
    int moves = 0, opens = 0, closes = 0, ends = 0, plans = 0;
    int now = 0, last = 0, moveDelay = 150, closeDelay = 100;
    bool scanOk = true, openOk = true, moveOk = true;
    bool inventoryOpen = false, owned = false, working = true;
    bool Plan(Move& move) {
        ++plans;
        if (!scanOk) return false;
        for (int i = 0; i < 9; ++i) if (slots[i] == 0) { move.target = i; break; }
        for (int i = 9; i < 36; ++i) if (slots[i] == 1) { move.source = i; break; }
        return true;
    }
    bool Begin() {
        if (!openOk) return false;
        if (!inventoryOpen) { ++opens; owned = true; inventoryOpen = true; }
        return true;
    }
    bool MoveItem(const Move& move) {
        assert(move.Valid() && inventoryOpen && working);
        assert(slots[move.target] == 0 && slots[move.source] == 1);
        if (!moveOk) return false;
        slots[move.target] = slots[move.source]; slots[move.source] = 0; ++moves;
        return true;
    }
    bool MoveDue() const { return now - last >= moveDelay; }
    bool CloseDue() const { return now - last >= closeDelay; }
    void ResetMoveDelay() { last = now; }
    void ResetCloseDelay() { last = now; }
    void End() { ++ends; if (owned && inventoryOpen) { ++closes; inventoryOpen = false; } owned = false; working = false; }
};

int main() {
    {
        Session session; IO io; io.slots[0] = 2; io.slots[9] = io.slots[35] = 1;
        session.Tick(io); assert(io.opens == 1 && io.moves == 0 && io.working);
        io.now = 149; session.Tick(io); assert(io.moves == 0 && io.plans == 1);
        io.now = 150; session.Tick(io); assert(io.moves == 1 && io.slots[1] == 1 && io.slots[9] == 0);
        io.now = 299; session.Tick(io); assert(io.moves == 1);
        io.now = 300; session.Tick(io); assert(io.moves == 2 && io.slots[2] == 1 && io.slots[35] == 0);
        io.now = 450; session.Tick(io); assert(session.phase == Session::Phase::Closing && io.working);
        io.now = 549; session.Tick(io); assert(io.closes == 0);
        io.now = 550; session.Tick(io); assert(io.ends == 1 && io.closes == 1 && !io.working);
        assert(io.slots[0] == 2);
        session.Tick(io); assert(io.ends == 1 && io.opens == 1);
    }
    for (bool full : {false, true}) {
        Session session; IO io;
        if (full) { for (int i = 0; i < 9; ++i) io.slots[i] = 2; io.slots[9] = 1; }
        session.Tick(io);
        assert(io.opens == 0 && io.moves == 0 && io.ends == 1 && !io.working);
    }
    {
        Session session; IO io; io.moveDelay = io.closeDelay = 0;
        for (int i = 9; i < 36; ++i) io.slots[i] = 1;
        session.Tick(io);
        for (int i = 1; i <= 9; ++i) { session.Tick(io); assert(io.moves == i); }
        session.Tick(io); session.Tick(io);
        assert(io.moves == 9 && io.ends == 1 && io.slots[18] == 1); // only free hotbar slots
    }
    {
        Session session; IO io; io.inventoryOpen = true; io.slots[9] = 1; io.moveDelay = 0;
        session.Tick(io); session.Tick(io); session.Tick(io); io.now = 100; session.Tick(io);
        assert(io.moves == 1 && io.opens == 0 && io.closes == 0 && io.inventoryOpen);
    }
    {
        Session session; IO io; io.slots[9] = 1; io.moveDelay = 0;
        session.Tick(io);
        io.slots[0] = 2; // User changes a slot while waiting: recompute destination.
        session.Tick(io); assert(io.slots[0] == 2 && io.slots[1] == 1);
    }
    for (int failure = 0; failure < 3; ++failure) {
        Session session; IO io; io.slots[9] = 1; io.moveDelay = 0;
        if (failure == 0) io.openOk = false;
        session.Tick(io);
        if (failure == 1) io.scanOk = false;
        if (failure == 2) io.moveOk = false;
        session.Tick(io); session.Tick(io);
        assert(io.moves == 0 && io.ends == 1 && !io.working);
    }
    {
        Session session; IO io; io.slots[9] = 1;
        session.Tick(io); io.inventoryOpen = false; // ESC / server screen replacement.
        session.Finish(io); session.Tick(io);
        assert(io.ends == 1 && io.closes == 0 && io.moves == 0 && !io.working);
    }
    assert(!(Move{8, 0}.Valid()) && !(Move{36, 0}.Valid()) && !(Move{9, 9}.Valid()));
    std::puts("PASS: one-shot refill, delays, fresh slot selection, full/empty hotbar, zero-delay pacing, cancellation and failed actions");
}
