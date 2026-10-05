#pragma once
#include <cstddef>

namespace nova::cs2::sdk::radar {
// Build 14184, IDA: CCSGO_HudRadar player update. See README.md.
inline constexpr std::size_t updateRva = 0xEB96E0;
inline constexpr std::size_t displayFlags = 0x17808; // 96264, bit 0 bypasses spotted visibility.
inline constexpr unsigned char showAll = 1;
void* UpdateTarget(); // Requires the existing module SHA256/build gate.
}
