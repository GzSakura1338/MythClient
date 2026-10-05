#pragma once
#include <cstdint>

namespace nova::cs2::sdk::weapons::offsets {
// client.dll build 14184 only; gated by players' client AND engine SHA256.
// IDA evidence and ABI notes: docs/cs2-recoil-control-implementation.md.
inline constexpr std::uintptr_t commit = 0xBEFF40, commitReturn = 0xB7C518;
inline constexpr std::uintptr_t inputFrame = 0xB7C070, inputFinalize = 0xB78AE0;
inline constexpr std::uintptr_t mouseInputAllowed = 0xCFC150;
inline constexpr std::uintptr_t weaponData = 0x388, fullAuto = 0x72D, revolver = 0x71E, range = 0x830;
inline constexpr std::uintptr_t waitForNoAttack = 0x1EC0;
inline constexpr std::uintptr_t inputPointer = 0x2236440, globalsPointer = 0x2229F88;
inline constexpr std::uintptr_t inputSuppression = 0x21FA030;
inline constexpr std::uintptr_t sampleCount = 0xB50, samples = 0xB58;
inline constexpr std::uintptr_t sampleStride = 0x440, sampleAngle = 0x430;
inline constexpr std::uintptr_t buttons = 0x10, buttonHeld = 0x32;
inline constexpr std::uintptr_t weaponServices = 0x12F0, activeWeapon = 0x60;
inline constexpr std::uintptr_t punchService = 0x1598, serviceOwner = 0x38, shotsFired = 0x1EB4;
inline constexpr std::uintptr_t itemDefinition = 0x1290 + 0x50 + 0x1BA, owner = 0x520;
inline constexpr std::uintptr_t mode = 0x1A00, recoilIndex = 0x1A28, clip = 0x1928;
inline constexpr std::uintptr_t reload = 0x1A3C, silencer = 0x1A4D, silencerEnd = 0x1A50;
inline constexpr std::uintptr_t lastShot = 0x1B68, burstRemaining = 0x1F24;
inline constexpr std::uintptr_t punchTime = 0x852C10, effectivePunch = 0x852990;
inline constexpr std::uintptr_t inaccuracy = 0x80D1E0, spread = 0x80E4D0;
// Build 14184 schema: C_BasePlayerWeapon / CCSPlayer_WeaponServices / C_CSPlayerPawn.
inline constexpr std::uintptr_t nextPrimaryTick = 0x1918, nextPrimaryFraction = 0x191C;
inline constexpr std::uintptr_t nextAttack = 0xD0, scoped = 0x1EA0;
// Native network tick interval, also verified in engine2 ProcessTick.
inline constexpr double interval = 0.015625;
}
