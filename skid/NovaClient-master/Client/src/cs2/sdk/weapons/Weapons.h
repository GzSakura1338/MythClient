#pragma once
#include "cs2/sdk/players/Players.h"
#include <cstdint>

namespace nova::cs2::sdk::weapons {
struct Angle { float pitch = 0, yaw = 0, roll = 0; };
struct Time { int tick = 0; float fraction = 0; };
struct Identity {
    std::uintptr_t network = 0, world = 0, pawn = 0, weapon = 0;
    std::uint32_t pawnHandle = 0, weaponHandle = 0;
    std::uint16_t item = 0;
    int mode = 0;
    bool silencer = false;
    bool operator==(const Identity&) const = default;
};
struct Weapon {
    Identity identity;
    std::uintptr_t punchService = 0;
    int shots = 0, clip = 0, burst = 0;
    float recoilIndex = 0, lastShot = 0;
    bool reload = false, switchingSilencer = false;
    double nextAttack = 0;
    float cone = 0;
    bool scoped = false, accuracyValid = false;
};
struct Sample {
    Weapon weapon;
    std::uint32_t frame = 0, ordinal = 0;
    double time = 0;
    Angle view, punch;
    bool attack = false;
};
bool IsFirearm(std::uint16_t item);
bool IsSniper(std::uint16_t item);
bool ReadWeapon(const players::Memory&, std::uintptr_t client, std::uintptr_t engine, Weapon&);
// Only called at the verified final input-angle commit on the game input thread.
bool Capture(Angle* currentInputAngle, Sample&);
struct FireState {
    bool valid = false, allowed = false, fullAuto = false, revolver = false;
    float range = 0;
};
bool ReadFireState(const players::Memory&, const Sample&, FireState&);
const char* FireStatus();
struct AttackRequest { Angle* angle = nullptr; float fraction = 0; bool down = false, previous = false; };
// A rejected native sample is temporary; only a write exception faults the adapter.
enum class AttackResult { Applied, Released, NoSample, StaleSample, SampleRejected, WriteFault };
inline const char* AttackStatus(AttackResult result) {
    switch (result) {
    case AttackResult::Applied: return "applied";
    case AttackResult::Released: return "released";
    case AttackResult::NoSample: return "input sample unavailable";
    case AttackResult::StaleSample: return "input sample changed";
    case AttackResult::SampleRejected: return "input history invalid or full";
    case AttackResult::WriteFault: return "input write exception";
    }
    return "unknown input result";
}
struct Binding {
    void* target = nullptr;
    void* sourceReturn = nullptr;
    bool (*capture)(Angle*, Sample&) = nullptr;
    void* frameTarget = nullptr;
    bool (*fireState)(const Sample&, FireState&) = nullptr;
    AttackResult (*attack)(void* input, const AttackRequest&) = nullptr;
};
Binding ResolveBinding();
}
