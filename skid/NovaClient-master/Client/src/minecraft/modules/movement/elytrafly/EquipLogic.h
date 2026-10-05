#pragma once
#include <utility>
namespace nova::module::elytrafly {
struct Trigger {
    bool desired = false, lastDown = false, previousHold = true;
    int previousKey = 0;
    void Tick(bool enabled, int key, bool hold, bool down, bool focused, bool screen) {
        if (key != previousKey || hold != previousHold) {
            desired = false; lastDown = down; previousKey = key; previousHold = hold;
        }
        if (!enabled || !key || !focused) desired = false;
        else if (!screen && down && !lastDown) desired = !desired;
        if (hold && !down) desired = false;
        lastDown = down;
    }
};
// Restore the physical selection at the NEXT tick, after vanilla has finished
// processing the item use. Never overwrite a user's intervening scroll/number key.
struct Selection {
    int previous = -1, used = -1;
    void Capture(int oldSlot, int useSlot) { previous = oldSlot; used = useSlot; }
    void Reset() { previous = used = -1; }
    template<class IO> void Restore(IO& io) {
        const int old = std::exchange(previous, -1), use = std::exchange(used, -1);
        int current = -1;
        if (old >= 0 && old != use && io.Selected(current) && current == use && io.Select(old)) io.Sync();
    }
};
}
