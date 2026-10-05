#pragma once
#include <cstdint>

namespace nova::cs2::sdk::hud::offsets {
// Build 14184 client schema; guarded by the players client/engine fingerprints.
inline constexpr std::uintptr_t ping = 0x838, playerName = 0x6FC, velocity = 0x3F8;
// engine2 ProcessTick +0x69CD0 writes Msg_Tick to network client +0x39C.
// The dumper's +0x24C is deltaTick, not the latest received server tick.
inline constexpr std::uintptr_t serverTick = 0x39C, serverCount = 0x244;
// ProcessServerInfo +0x6A280 logs the connection's retry address through these.
inline constexpr std::uintptr_t connection = 0x168, connectionAddress = 0x28;
}
