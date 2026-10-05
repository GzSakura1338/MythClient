#pragma once
#include <atomic>
#include <cstdint>
#include "bind/PhysicalKey.h"

namespace nova::bind::unload {
// Shared by render, window and host threads. Keep the key, capture suppression
// and edge latch in one atomic value so a rebind cannot race an unload poll.
inline std::atomic<unsigned> state{VK_DELETE};
inline std::atomic<const void*> property{nullptr};
inline constexpr unsigned editing = 1u << 16, waitRelease = 1u << 17, held = 1u << 18;
inline int Key(unsigned value) { return static_cast<std::int16_t>(value & 0xFFFFu); }
inline int Key() { return Key(state.load()); }
inline void SetKey(int key) {
    unsigned old = state.load();
    const unsigned code = static_cast<std::uint16_t>(key);
    while (!state.compare_exchange_weak(old, code | (old & editing) | waitRelease)) {}
}
inline void BeginCapture(const void* target) {
    if (target && target == property.load()) state.fetch_or(editing | waitRelease);
}
inline void EndCapture() {
    unsigned old = state.load();
    while (old & editing) {
        if (state.compare_exchange_weak(old, (old & ~editing) | waitRelease)) break;
    }
}
inline bool Observe(unsigned old, bool down, bool foreground) {
    if (!Key(old) || (old & editing)) return false;
    unsigned next = old;
    bool trigger = false;
    if (!down) next &= ~(held | waitRelease);
    else if (!foreground) next |= held | waitRelease;
    else {
        trigger = !(old & (held | waitRelease));
        next |= held;
    }
    // Discard a sample if another thread changed the key/capture/latch.
    return state.compare_exchange_strong(old, next) && trigger;
}
inline bool Input(int key, bool down) {
    const unsigned current = state.load();
    return key == Key(current) && Observe(current, down, true);
}
inline bool Poll() {
    const unsigned current = state.load();
    DWORD process = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &process);
    return Observe(current, IsPhysicalBindingDown(Key(current)), process == GetCurrentProcessId());
}
}
