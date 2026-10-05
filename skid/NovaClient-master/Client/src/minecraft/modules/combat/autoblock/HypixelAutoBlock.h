#pragma once

#include <algorithm>

namespace nova::module {

// Leader Lite B0.4 KillAura.Hypixel. Indices and even the Custom mode's
// one-based comparisons / maxTick - 1 wrap deliberately follow the source.
struct HypixelBlockSettings {
    int mode = 0; // OldHypixel, Without NoSlow, Custom, Lag
    int lagMode = 1; // 2Tick, 3Tick, 4Tick, 3Tick + 2Tick, 5Tick
    bool noSwap = true;
    bool moreAttack = false;
    int moreAttackDelay = 1;
    int maxTick = 3;
    int startBlinkTick = 0, stopBlinkTick = 2;
    int swapTick = 2, switchBackTick = 2, stopBlockTick = 2;
    int attackTick = 0, startBlockTick = 0;
    bool postBlock = false;
    bool alwaysRenderBlocking = true;
    bool c09Instead = true;
    bool fullC09 = false;
    int aps = 10;
    double range = 6.0;
};

// IO performs actions immediately, including PlayerStateManager's digging /
// placing observations. Deferring any of these actions to key processing would
// change both the attack gates and the packet order.
class HypixelAutoBlock {
public:
    int blockTick = 0;
    int attackDelayMS = 0;
    int testAttackTick = 0;
    bool isBlocking = false, fakeBlockState = false;
    bool swapped = false, postBlock = false;

    template<class IO>
    void Pre(const HypixelBlockSettings& s, IO& io) {
        if (attackDelayMS > 0) attackDelayMS -= 50;
        bool attack = io.CanAttack();
        const bool block = attack && io.CanBlock();
        if (!block) {
            io.Blink(false);
            if (s.mode == 0 && isBlocking && io.NoSlowEnabled()) {
                isBlocking = false;
                io.Stop();
            }
            if (swapped) { io.Slot(io.CurrentSlot()); swapped = false; }
            else isBlocking = false;
            fakeBlockState = false;
            blockTick = 0;
        }
        if (!attack) return;
        bool swap = false, blocked = false;
        if (block) {
            if (io.HasBlockTarget()) {
                if (!io.Busy()) {
                    switch (s.mode) {
                    case 0:
                    case 1:
                        switch (blockTick) {
                        case 0:
                            if (s.mode == 1) io.Blink(false);
                            if (!io.IsPlayerBlocking()) swap = true;
                            if (s.mode == 0) blocked = true;
                            blockTick = 1;
                            break;
                        case 1: attack = false; blockTick = 2; break;
                        case 2:
                            if (s.mode == 1) io.Blink(true);
                            if (io.IsPlayerBlocking()) {
                                if (s.mode == 0 && !s.noSwap) TripleSwap(io);
                                io.Stop();
                            }
                            if (s.moreAttack) {
                                if (testAttackTick >= s.moreAttackDelay) testAttackTick = 0;
                                else { ++testAttackTick; attack = false; }
                            } else if (s.mode == 0) attack = false;
                            blockTick = 0;
                            break;
                        default: blockTick = 0; break;
                        }
                        break;
                    case 2:
                        if (blockTick + 1 == s.startBlinkTick) blocked = true;
                        if (blockTick + 1 != s.attackTick) attack = false;
                        if (blockTick + 1 == s.startBlockTick && !io.IsPlayerBlocking()) {
                            swap = true;
                            if (s.postBlock) postBlock = true;
                        }
                        if (blockTick + 1 == s.stopBlinkTick) io.Blink(false);
                        if (blockTick + 1 == s.swapTick) { io.Slot(io.SwapSlot()); swapped = true; }
                        if (blockTick + 1 == s.switchBackTick && swapped) {
                            io.Slot(io.CurrentSlot()); swapped = false;
                        }
                        if (blockTick + 1 == s.stopBlockTick && io.IsPlayerBlocking()) io.Stop();
                        if (++blockTick >= s.maxTick - 1) blockTick = 0;
                        break;
                    case 3:
                        Lag(s, io, attack, swap, blocked);
                        break;
                    }
                }
                isBlocking = true;
                fakeBlockState = s.mode != 3 || s.alwaysRenderBlocking;
            } else {
                io.ClearExtraAttack();
                if (s.mode == 2 && swapped) { io.Slot(io.CurrentSlot()); swapped = false; }
                io.Blink(false);
                isBlocking = fakeBlockState = false;
                if (s.mode == 0) { io.Slot(io.SwapSlot()); io.Slot(io.CurrentSlot()); }
            }
        }
        bool attacked = false;
        // performAttack in Leader rechecks digging / placing after the state
        // machine. A release packet sent above suppresses this tick's attack.
        if (attack && io.InSwingRange() && !io.Busy() && !io.IsPlayerBlocking() &&
            attackDelayMS <= 0 && io.AttackReady() && io.PredictionAttack(s, blockTick)) {
            attackDelayMS += isBlocking ? static_cast<int>(1000.0f / std::max(1, s.aps)) : io.NormalAttackDelay();
            io.Swing();
            attacked = io.Attack(); // ray check happens after swing and timer consumption
        }
        if (swap) {
            if (attacked) io.InteractBlock();
            else if (!postBlock) io.Start();
        }
        if (blocked) { io.Blink(false); io.Blink(true); }
    }

    template<class IO>
    void Post(IO& io) {
        if (postBlock) { io.Start(); postBlock = false; }
    }

    template<class IO>
    void Disable(const HypixelBlockSettings& s, IO& io) {
        io.Blink(false);
        fakeBlockState = false;
        if (swapped) { io.Slot(io.CurrentSlot()); swapped = false; }
        if (s.mode == 0 && isBlocking && io.NoSlowEnabled()) io.Stop();
        isBlocking = false;
        postBlock = false;
    }

    bool VelocityCanReduce(const HypixelBlockSettings& s, int phase, int tick) const {
        if (s.mode == 0 || s.mode == 1) return phase == 2 ? tick == 2 : tick == 0;
        if (s.mode == 2) {
            const int maxT = std::max(1, s.maxTick - 1);
            if (phase == 0) return tick == s.attackTick;
            if (phase == 1) return tick == s.attackTick % maxT;
            return tick == (s.attackTick - 2 + maxT) % maxT;
        }
        if (s.lagMode == 0) return phase == 2 ? tick == 1 : tick == 0;
        const int end = s.lagMode == 1 ? 2 : s.lagMode == 2 ? 3 : 4;
        return phase == 2 ? tick == end : phase == 1 ? tick == 0 :
            tick == 0 || tick == end || (s.lagMode == 3 && tick == 2);
    }

private:
    template<class IO> static void TripleSwap(IO& io) {
        const int handle = io.CurrentSlot();
        io.Slot(io.AltSlot(handle));
        io.Slot(handle % 7 + 2);
        io.Slot(handle);
    }

    template<class IO>
    void Lag(const HypixelBlockSettings& s, IO& io, bool& attack, bool& swap, bool& blocked) {
        switch (s.lagMode) {
        case 0:
            switch (blockTick) {
            case 0:
                io.Blink(false);
                if (!io.IsPlayerBlocking()) swap = true;
                blockTick = 1;
                break;
            case 1:
                io.Blink(true);
                if (io.IsPlayerBlocking()) io.Stop();
                attack = false;
                if (attackDelayMS <= 50) blockTick = 0;
                break;
            default: blockTick = 0; break;
            }
            break;
        case 1:
            switch (blockTick) {
            case 0:
                blocked = true;
                if (!io.IsPlayerBlocking()) swap = true;
                blockTick = 1;
                break;
            case 1:
                if (io.IsPlayerBlocking()) {
                    if (s.c09Instead) TripleSwap(io);
                    else io.Stop();
                }
                attack = false; blockTick = 2;
                break;
            case 2:
                io.Blink(false);
                if (attackDelayMS <= 50) blockTick = 0;
                break;
            default: blockTick = 0; break;
            }
            break;
        case 2:
            switch (blockTick) {
            case 0:
                blocked = true;
                if (!io.IsPlayerBlocking()) swap = true;
                blockTick = 1;
                break;
            case 1:
                if (io.IsPlayerBlocking() && s.fullC09) { TripleSwap(io); io.Stop(); }
                attack = false; blockTick = 2;
                break;
            case 2:
                io.Slot(io.AltSlot(io.CurrentSlot())); io.Slot(io.CurrentSlot()); io.Stop();
                blockTick = 3;
                break;
            case 3:
                io.Blink(false);
                if (attackDelayMS <= 50) blockTick = 0;
                break;
            default: blockTick = 0; break;
            }
            break;
        case 3:
            switch (blockTick) {
            case 0: case 2:
                blocked = true;
                if (!io.IsPlayerBlocking()) swap = true;
                ++blockTick;
                break;
            case 1: case 3:
                if (io.IsPlayerBlocking()) { if (s.fullC09) TripleSwap(io); io.Stop(); }
                if (blockTick == 1) attack = false;
                ++blockTick;
                break;
            case 4:
                io.Blink(false);
                if (attackDelayMS <= 50) blockTick = 0;
                break;
            default: blockTick = 0; break;
            }
            break;
        case 4:
            switch (blockTick) {
            case 0:
                if (!io.IsPlayerBlocking()) swap = true;
                blockTick = 1;
                break;
            case 1:
                io.Blink(true);
                if (io.IsPlayerBlocking()) { if (s.fullC09) TripleSwap(io); io.Stop(); }
                attack = false; blockTick = 2;
                break;
            case 2: case 3:
                if (s.fullC09) { TripleSwap(io); io.Stop(); }
                attack = false; ++blockTick;
                break;
            case 4:
                io.Blink(false);
                if (attackDelayMS <= 50) blockTick = 0;
                break;
            default: blockTick = 0; break;
            }
            break;
        }
    }
};

}
