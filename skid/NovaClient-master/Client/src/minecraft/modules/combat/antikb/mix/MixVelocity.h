#pragma once

namespace nova::module::mix {
struct Settings {
    int delayTicks = 2, reduceTicks = 3;
    bool noBadPackets = false, jump = true, airPush = false;
    bool airBuffer = true, groundDelay = false, fakeCheck = true;
};

// AntiKnockBack.java's Mix state, independent of JNI and packet ownership.
struct Velocity {
    bool allowNext = true, received = false, positive = false;
    bool delaying = false, pending = false, released = false;
    int reduceCounts = 0, delayedTicks = 0;

    void Hurt() { allowNext = false; }
    template<class IO> bool VelocityPacket(const Settings& s, int y, IO& io) {
        const bool real = !s.fakeCheck || !allowNext;
        received = positive = real && y > 0;
        return !delaying && received &&
            ((!io.CanDelay() || s.groundDelay) || (s.airBuffer && !io.OnGround())) && !io.LiquidWeb();
    }
    void Begin() { delaying = pending = true; delayedTicks = 0; }
    void Passed(const Settings& s) {
        if (received) { reduceCounts = s.reduceTicks; released = true; }
    }
    void Applied(const Settings& s) {
        if (!allowNext || !s.fakeCheck) allowNext = true;
        positive = false;
    }
    void Finish(const Settings& s) {
        delaying = false; delayedTicks = 0;
        if (pending) { pending = false; released = true; reduceCounts = s.reduceTicks; }
    }
    template<class IO> void Pre(const Settings& s, IO& io) {
        if (reduceCounts <= 0) return;
        --reduceCounts; // A missed/gated tick still consumes one attempt in the reference.
        if (!io.Target(s.airPush) || !io.Sprinting() ||
            (s.noBadPackets && io.BadPackets()) || io.Blocking() ||
            !io.CanReduce()) return;
        io.Attack();
    }
    template<class IO> void Post(const Settings& s, IO& io) {
        if (!delaying) return;
        ++delayedTicks;
        if ((!s.groundDelay && io.CanDelay()) || (s.airBuffer && io.OnGround()) ||
            io.LiquidWeb() || (!s.airBuffer && delayedTicks >= s.delayTicks)) {
            io.Release();
            Finish(s);
        }
    }
    template<class IO> void Living(const Settings& s, IO& io) {
        if (!released || !s.jump || !received) return;
        if (io.OnGround() && io.ForwardPressed() && !io.JumpBoost() && !io.LiquidWeb()) io.Jump();
        received = released = false;
    }
};
}
