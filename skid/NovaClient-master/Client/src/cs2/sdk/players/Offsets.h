#pragma once
#include <cstdint>

namespace nova::cs2::sdk::players::offsets {
// Build 14184: local a2x/scros extraction, 2026-09-25. Keep all versioned
// addresses together; never apply them before checking both module hashes.
inline constexpr int build = 14184;
inline constexpr unsigned clientSize = 43606016, engineSize = 9900032;
inline constexpr char clientHash[] = "EB465AB30D0E2639FA1ED1ABCB02353D35B70F6E3202990B75BAC11647CE6893";
inline constexpr char engineHash[] = "B63B2AB7CAE8115E9BF05562E3289E2AA9E65D3798633CC160B653BC55EE6B52";
inline constexpr std::uintptr_t entitySystem = 0x27130E8, localController = 0x2535598;
inline constexpr std::uintptr_t localPawn = 0x255E658, viewMatrix = 0x25639A0;
inline constexpr std::uintptr_t buildNumber = 0x61D1E8, networkClient = 0x91B1C0;
inline constexpr std::uintptr_t signOnState = 0x230, maxClients = 0x240;
inline constexpr std::uintptr_t identity = 0x10, controllerPawn = 0x92C, controllerName = 0x878;
inline constexpr std::uintptr_t controllerArmor = 0x93C; // Replicated value, useful for read-only diagnostics.
inline constexpr std::uintptr_t sceneNode = 0x330, collision = 0x340, maximumHealth = 0x348;
inline constexpr std::uintptr_t health = 0x34C, lifeState = 0x354, team = 0x3E7, armor = 0x1ECC;
inline constexpr std::uintptr_t origin = 0xC8, dormant = 0x103, mins = 0x40, maxs = 0x4C;
inline constexpr std::uintptr_t nodeToWorld = 0x10; // CGameSceneNode::m_nodeToWorld, CTransformWS.
// Non-Schema identity layout used by the entity system, validated per entry.
inline constexpr unsigned chunkSize = 512, stride = 0x70, indexMask = 0x7FFF;
}
