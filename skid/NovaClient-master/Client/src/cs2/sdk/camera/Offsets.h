#pragma once
#include <cstdint>

namespace nova::cs2::sdk::camera::offsets {
// Build 14184; binary evidence: docs/cs2-build14184-mapping.md.
inline constexpr std::uintptr_t worldFov = 0x88A640, baseFov = 0xA67E80;
inline constexpr std::uintptr_t worldFovVirtual = 0x118, baseFovVirtual = 0x120;
inline constexpr std::uintptr_t cameraServices = 0x1328, serviceOwner = 0x38;
// C_CSPlayerPawn::m_bIsScoped, Build 14184 client schema.
inline constexpr std::uintptr_t isScoped = 0x1EA0;
// Intermediate OverrideView; later setup code can still change projection.
inline constexpr std::uintptr_t overrideView = 0xD02650, overrideReturn = 0xB9C867;
inline constexpr std::uintptr_t modeVtable = 0x1C9AB90, modeSlot = 0x28;
inline constexpr std::uintptr_t viewFov = 0x498, viewAngles = 0x4B8;
inline constexpr std::uintptr_t input = 0x2236440, thirdPerson = 0x229, inputAngles = 0x13A8;
// Active controller Pawn can be C_CSObserverPawn after death.
inline constexpr std::uintptr_t activePawn = 0x6BC, observerServices = 0x1308;
inline constexpr std::uintptr_t observerMode = 0x48, observerTarget = 0x4C, viewEntity = 0xA4;
// CCSCameraManager + 0x28: cinematic camera consumed by OverrideView.
inline constexpr std::uintptr_t cinematicCamera = 0x25764A8;
inline constexpr std::uint32_t dataDrivenCameraType = 0x22F42C0;
inline constexpr std::uintptr_t playerVtable = 0x1C85068;
inline constexpr std::uintptr_t setupView = 0xB9C5D0, viewAspect = 0x4D4;
}
