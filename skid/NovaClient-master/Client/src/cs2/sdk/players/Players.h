#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace nova::cs2::sdk::players {
constexpr bool SameTeam(int localTeam, int playerTeam) {
    return (localTeam == 2 || localTeam == 3) && localTeam == playerTeam;
}
struct Vec3 { float x = 0, y = 0, z = 0; };
using Matrix = std::array<std::array<float, 4>, 4>;
struct Player {
    std::uint32_t handle = 0;
    Vec3 origin, mins, maxs;
    int health = 0, maximum = 0, armor = 0;
    int team = 0;
    std::string name;
    std::array<Vec3, 8> worldCorners{};
    bool transformed = false;
};
struct Snapshot { Matrix matrix{}; std::vector<Player> players; int localTeam = 0; };
// Injectable read-only address space for deterministic snapshot tests. No
// engine methods, retained game pointers or writes to game memory.
struct Memory {
    void* context = nullptr;
    bool (*copy)(void*, std::uintptr_t, void*, std::size_t) = nullptr;
    template<class T> bool read(std::uintptr_t address, T& value) const {
        value = {};
        return copy && address >= 0x10000 && address + sizeof(T) > address && copy(context, address, &value, sizeof(T));
    }
};
bool ReadSnapshot(const Memory& memory, std::uintptr_t client, std::uintptr_t engine, Snapshot& result);
bool Initialize();
void Shutdown();
bool Available();
// Fingerprint gate shared by native adapters for their additional DLLs.
bool MatchesModule(const wchar_t* name, unsigned imageSize, const char* sha256);
const char* Status();
bool Capture(Snapshot& result);
}
