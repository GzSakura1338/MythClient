#pragma once
#include "cs2/sdk/players/Players.h"
#include "cs2/sdk/weapons/Weapons.h"

namespace nova::cs2::sdk::camera {
// Read-only ownership check. Optionally sample scope state from the same Pawn;
// the output is only published after ownership has been rechecked.
bool IsLocalCamera(const players::Memory&, std::uintptr_t client, std::uintptr_t engine, std::uintptr_t service,
    bool* scoped = nullptr);
struct NativeFov { float base = 0; bool scoped = false; };
struct PlayerView {
    std::uintptr_t pawn = 0, service = 0, active = 0;
    std::uint32_t handle = 0;
    bool scoped = false, spectating = false;
    bool operator==(const PlayerView&) const = default;
};
// Only a living player or an in-eye/chase observer target; rejects cinematics.
bool ReadPlayerView(const players::Memory&, std::uintptr_t client, std::uintptr_t engine, PlayerView&);
const char* LastViewStatus();
struct View {
    float* fov = nullptr;
    weapons::Angle* angles = nullptr;
    NativeFov native;
    weapons::Angle unshaken;
    bool unshakenValid = false;
    std::uintptr_t pawn = 0;
    std::uint32_t pawnHandle = 0;
    float aspect = 4.f / 3.f;
};
struct Binding {
    void* target = nullptr;
    bool (*captureView)(void* mode, void* view, View&) = nullptr;
};
Binding ResolveBinding();
// Read-only render data; includes current zoom and FOV Control projection.
bool ReadVerticalProjection(const players::Memory&, std::uintptr_t client, std::uintptr_t engine, float& scale);
bool CaptureVerticalProjection(float& scale);
}
