#pragma once
#include <cstdint>

namespace nova::module {
// AutoBlock.java: LAG_NEW, C()'s 7APS branch, without Expo's NoSlow module.
// Separate attack, block and release intervals; 7APS remains the default.
// Item use must be established before physics for the corresponding C03.
struct LagNewAutoBlock {
    int stage = 0;
    bool active = false, blocking = false, attacked = false;
    std::uint64_t lastFlush = 0;
    std::uint64_t actionMovement = 0;
    bool waitingMovement = false;
    int attacksRemaining = 1, holdRemaining = 1;
    std::uint64_t holdMovement = ~std::uint64_t{0};
    // Select only at a new attack phase; changing settings never restarts a
    // partially sent action group or creates multiple attacks in one tick.
    void SelectCadence(int mode) {
        attacksRemaining = mode == 3 ? 2 : mode == 4 ? 5 : 1;
        holdRemaining = mode == 1 ? 4 : mode == 2 ? 2 : 1;
        holdMovement = ~std::uint64_t{0};
    }
    template<class IO> void Reset(IO& io) {
        if (active) {
            io.Flush();
            if (blocking) io.Release();
        }
        *this = {};
    }
    template<class IO> void Pre(IO& io, int apsMode = 0) {
        // A skipped player update isn't a protocol tick. Do not release after
        // use or attack after release until vanilla generated a movement packet.
        if (waitingMovement && actionMovement == io.Movement()) return;
        waitingMovement = false;
        if (!io.Eligible()) { Reset(io); return; }
        if (!active) {
            active = true;
            blocking = io.Blocking();
            stage = blocking ? 0 : 1;
            lastFlush = io.Now();
        }
        if (stage == 7) {
            // The attack's movement already went out. Reblock in this input
            // phase so vanilla slowdown and the server's use state agree.
            if (!io.Block(false)) {
                actionMovement = io.Movement(); waitingMovement = true;
                return;
            }
            blocking = true; stage = 11;
            actionMovement = io.Movement(); waitingMovement = true;
            return;
        }
        if (stage == 11) {
            actionMovement = io.Movement(); waitingMovement = true;
            return;
        }
        if (stage == 0 || stage == 3) {
            if (!io.Begin()) { Reset(io); return; }
            if (stage == 0) {
                if (!blocking || io.Release()) {
                    blocking = false; stage = 3;
                    actionMovement = io.Movement(); waitingMovement = true;
                }
                return; // The 329ms fallback must never attack in a release interval.
            } else { stage = 2; SelectCadence(apsMode); }
        } else if (stage == 1) { stage = 2; SelectCadence(apsMode); }
        // mb decrypts to 329 ms. Preserve its fallback for missing manual clicks.
        const bool expired = io.Now() - lastFlush >= 329;
        if (stage == 2) {
            attacked = false;
            const bool consumed = io.Attack(attacked);
            if (consumed || expired) {
                if (!consumed || --attacksRemaining <= 0) stage = 7;
                actionMovement = io.Movement(); waitingMovement = true;
            }
        }
    }
    template<class IO> void Post(IO& io) {
        if (!active) return;
        // Cleanup may send RELEASE: leave it to the next input phase too.
        if (!io.Eligible()) return;
        if (waitingMovement && actionMovement == io.Movement()) return;
        if (stage == 11) {
            // Spend hold budget once per generated movement, even if Post is
            // dispatched repeatedly or the player update was skipped.
            if (holdMovement == io.Movement()) return;
            holdMovement = io.Movement();
            if (--holdRemaining > 0) return;
            io.Flush();
            lastFlush = io.Now();
            stage = 0;
            // Reacquisition is optional; a denied queue must not send RELEASE
            // from Post. Pre retries with the normal ownership checks.
            io.Begin();
        }
    }
};
}
