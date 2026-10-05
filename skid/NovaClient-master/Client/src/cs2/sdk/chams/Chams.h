#pragma once
#include "cs2/sdk/players/Players.h"
#include <cstddef>

namespace nova::cs2::sdk::chams {
// Build14184 scene draw record. Only material and packed RGBA are overridden.
struct Mesh {
    std::byte prefix[0x18];
    std::uintptr_t scene;
    void* material;
    std::byte middle[0x28];
    std::uint32_t color;
    std::byte tail[0x1C];
};
static_assert(sizeof(Mesh) == 0x70 && offsetof(Mesh, material) == 0x20 && offsetof(Mesh, color) == 0x50);
struct Settings {
    bool enabled = false, illuminate = false, throughWalls = false;
    std::uint32_t visible = 0xFF00FF00, hidden = 0xFF0000FF;
};
struct Material { void* binding = nullptr; void* data = nullptr; bool owned = false; };
struct Materials { Material slots[4]{}; }; // White visible/hidden, Illuminate visible/hidden.
struct Binding {
    void* target = nullptr;
    bool (*enemy)(std::uintptr_t scene) = nullptr;
    bool (*create)(Materials&) = nullptr;
    void (*release)(Materials&) = nullptr;
};
inline constexpr std::uintptr_t pawnVtable = 0x1C85068, sceneOwner = 0xB8;
bool IsEnemy(const players::Memory&, std::uintptr_t client, std::uintptr_t engine, std::uintptr_t scene);
const char* LastSelectionStatus();
Binding ResolveBinding();
}
