#pragma once
#include <cstdint>

namespace nova::module::autorod {
struct KeyState {
    int key = 0;
    bool lastDown = false, active = false;

    bool Update(int binding, bool down, bool allowed) {
        if (binding != key) {
            key = binding; active = false; lastDown = down;
        }
        if (!allowed || key == 0) { active = false; lastDown = down; return false; }
        if (down && !lastDown) active = true;
        if (!down) active = false;
        lastDown = down;
        return active;
    }
};

struct State {
    std::uint64_t nextAttempt = 0;
    bool running = false;
    int originalSlot = -1, usedSlot = -1;
    bool waitingToUse = false;
    std::uint64_t useAt = 0;
    bool rodWaiting = false;
    int rodSlot = -1;
    std::uint64_t rodReturnAt = 0;

    bool OwnsItemUse(bool activeHold) const {
        return activeHold || running || originalSlot >= 0 || waitingToUse || rodWaiting;
    }

    template<class IO>
    bool Restore(IO& io) {
        if (originalSlot < 0) return true;
        if (!io.CanSwitch()) return false;
        int selected = -1;
        if (!io.Selected(selected)) return false;
        // Preserve a manual selection made after the use. Still synchronize it.
        if (selected == usedSlot && !io.Select(originalSlot)) return false;
        if (!io.Sync()) return false;
        originalSlot = usedSlot = -1;
        return true;
    }

    template<class IO>
    bool Use(IO& io, int slot, std::uint64_t now, std::uint64_t rodDelay) {
        const bool rod = io.IsRod(slot);
        // Legacy sendUseItem can return false after casting a rod successfully
        // because neither the stack size nor its damage changed. Track attempts.
        if (rod) { rodWaiting = true; rodSlot = slot; rodReturnAt = now + rodDelay; }
        return io.Use();
    }

    template<class IO>
    bool Tick(IO& io, bool allowed, std::uint64_t delay, std::uint64_t now,
              std::uint64_t switchDelay = 0, std::uint64_t rodDelay = 500) {
        if (running) return false;
        running = true;
        struct Finish {
            State& state;
            ~Finish() { state.running = false; }
        } finish{*this};
        if (rodWaiting) {
            int selected = -1;
            if (!allowed || !io.Selected(selected) || selected != rodSlot || !io.IsRod(selected)) {
                rodWaiting = false;
                Restore(io);
                return false;
            }
            if (now < rodReturnAt || !io.CanSwitch()) return false;
            rodWaiting = false;
            nextAttempt = now + delay;
            // Reel in while still holding the rod. Restoration is a later
            // action interval, never part of this use call.
            return io.Use();
        }
        if (waitingToUse) {
            int selected = -1;
            if (!allowed || !io.Selected(selected) || selected != usedSlot || !io.Eligible(selected)) {
                waitingToUse = false;
                Restore(io);
                return false;
            }
            if (now < useAt || !io.CanSwitch()) return false;
            waitingToUse = false;
            nextAttempt = now + delay;
            return Use(io, selected, now, rodDelay);
        }
        // Tick count alone is not a network boundary: delayed or cancelled
        // movement and actions from other modules can keep slot changes unsafe.
        // Release/disable/menu gates stop new uses, but must not skip cleanup.
        if (!Restore(io) || !allowed || now < nextAttempt) return false;
        int original = -1;
        if (!io.Selected(original) || original < 0 || original > 8) return false;
        int item = -1;
        for (int slot = 0; item < 0 && slot < 9; ++slot)
            if (io.Eligible(slot)) item = slot;
        if (item < 0) return false;
        if (!io.CanSwitch()) return false;

        nextAttempt = now + delay;
        if (item != original) {
            originalSlot = original;
            usedSlot = item;
            if (!io.Select(item)) return false;
        }
        if (!io.Sync()) return false;
        if (item != original && switchDelay > 0) {
            useAt = now + switchDelay;
            waitingToUse = true;
            return false;
        }
        // Never restore here: sending a held-slot change after use in the same
        // tick violates vanilla action ordering. Even failed uses may send packets.
        return Use(io, item, now, rodDelay);
    }
};
}
