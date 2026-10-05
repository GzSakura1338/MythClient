#pragma once

namespace nova::inventory {
// IO owns eligibility checks. Always restore and synchronize after attempting
// a temporary selection, including failures and exceptions from the use call.
template<class IO>
bool UseHotbarItem(IO& io, int original, int slot) {
    struct Restore {
        IO& io;
        int original;
        bool switched;
        ~Restore() {
            if (switched) { io.Select(original); io.Sync(); }
        }
    } restore{io, original, slot != original};
    if (slot != original && !io.Select(slot)) return false;
    if (!io.Sync()) return false;
    return io.Use();
}
}
