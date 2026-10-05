#pragma once

namespace nova::module::refill {
struct Move {
    int source = -1; // player inventory 9..35, same indices in the player menu
    int target = -1; // hotbar 0..8, the Swap button (not menu slot 36..44)
    bool Valid() const { return source >= 9 && source < 36 && target >= 0 && target < 9; }
};

struct Session {
    enum class Phase { Ready, Moving, Closing, Done } phase = Phase::Ready;

    template<class IO> void Finish(IO& io) {
        if (phase == Phase::Done) return;
        io.End();
        phase = Phase::Done;
    }
    template<class IO> void Tick(IO& io) {
        if (phase == Phase::Done) return;
        if (phase == Phase::Closing) {
            if (io.CloseDue()) Finish(io);
            return;
        }
        if (phase == Phase::Moving && !io.MoveDue()) return;
        Move move;
        if (!io.Plan(move)) { Finish(io); return; }
        if (!move.Valid()) {
            if (phase == Phase::Ready) Finish(io);
            else { phase = Phase::Closing; io.ResetCloseDelay(); }
            return;
        }
        if (phase == Phase::Ready) {
            if (!io.Begin()) { Finish(io); return; }
            phase = Phase::Moving;
            io.ResetMoveDelay(); // Opening and first click are separate game ticks.
            return;
        }
        // One click per tick, including zero delay. Never loop over a stale snapshot.
        if (!io.MoveItem(move)) { Finish(io); return; }
        io.ResetMoveDelay();
    }
};
}
