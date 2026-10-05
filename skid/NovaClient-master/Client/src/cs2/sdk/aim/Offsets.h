#pragma once
#include <cstddef>
#include <cstdint>

namespace nova::cs2::sdk::aim::offsets {
// Build 14184 only; client and engine fingerprints owned by sdk/players.
inline constexpr std::uintptr_t getSet = 0x9408B0, getTransforms = 0x946A20, lookupBone = 0x9529A0;
inline constexpr std::uintptr_t shootPosition = 0xA7F980, traceLine = 0x827320, traceManager = 0x2221248;
inline constexpr std::uintptr_t traceFilter = 0x3520D0, traceWorld = 0x2006D0, clipPlayers = 0xAC9CA0;
inline constexpr std::uintptr_t immunity = 0x3508, serviceOwner = 0x38;
inline constexpr std::uintptr_t pawnController = 0x14BC;
inline constexpr std::uintptr_t traceHit = 0x8, traceFraction = 0xAC, traceStartSolid = 0xBB;
inline constexpr std::size_t traceResultSize = 192;
inline constexpr std::uintptr_t setCount = 0x10, setData = 0x18, hitboxStride = 0x70;
inline constexpr std::uintptr_t boneName = 0x10, minBounds = 0x18, maxBounds = 0x24, group = 0x38, shape = 0x3C;
// Bullet contents; gameplay player hitboxes also require ClipTraceToPlayers.
inline constexpr std::uint64_t traceMask = 0x1C300B;
}
