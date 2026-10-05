#pragma once
#include <cmath>
#include <cstdint>
#include "minecraft/utils/inventory/HotbarUse.h"

namespace nova::module::autopot {
struct State {
    std::uint64_t nextAttempt = 0;
    bool running = false;

    template<class IO>
    bool Tick(IO& io, bool allowed, float health, double maximumHealth, std::uint64_t now) {
        if (!allowed || running || !std::isfinite(health) || !std::isfinite(maximumHealth) ||
            health <= 0 || health > maximumHealth || now < nextAttempt) return false;
        int original = -1;
        if (!io.Selected(original) || original < 0 || original > 8) return false;
        int potion = io.Healing(original) ? original : -1;
        for (int slot = 0; potion < 0 && slot < 9; ++slot)
            if (slot != original && io.Healing(slot)) potion = slot;
        if (potion < 0) return false;

        // A thrown potion needs time to land and update health. Also rate-limit
        // failed attempts, so a blocked use cannot generate a switch every tick.
        nextAttempt = now + 500;
        running = true;
        struct Finish {
            State& state;
            ~Finish() { state.running = false; }
        } finish{*this};
        return inventory::UseHotbarItem(io, original, potion);
    }
};
}
