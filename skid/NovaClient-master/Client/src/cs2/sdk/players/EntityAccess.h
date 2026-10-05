#pragma once
#include "cs2/sdk/players/Players.h"
#include "cs2/sdk/players/Offsets.h"

namespace nova::cs2::sdk::players {
// Shared serial-aware resolver for game-thread readers; no retained pointers.
inline bool ResolveEntity(const Memory& m, std::uintptr_t world, std::uint32_t handle, std::uintptr_t& entity) {
    entity = 0;
    std::uintptr_t chunk = 0, back = 0;
    std::uint32_t stored = 0, flags = 0;
    const auto index = handle & offsets::indexMask;
    if (!world || handle == 0xFFFFFFFF || !index ||
        !m.read(world + 0x10 + (index / offsets::chunkSize) * 8, chunk) || !chunk) return false;
    const auto slot = chunk + (index % offsets::chunkSize) * offsets::stride;
    return m.read(slot, entity) && entity && m.read(slot + 0x10, stored) &&
        m.read(slot + 0x30, flags) && stored - ((flags & 1u) << 15) == handle &&
        m.read(entity + offsets::identity, back) && back == slot;
}
}
