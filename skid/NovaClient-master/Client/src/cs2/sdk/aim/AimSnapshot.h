#pragma once
#include "cs2/sdk/weapons/Weapons.h"
#include "cs2/sdk/players/Transform.h"
#include <array>
#include <optional>
#include <span>

namespace nova::cs2::sdk::aim {
using Vec3 = players::Vec3;
enum class Point : unsigned { Head, Chest, Pelvis };
enum class Visibility { Unknown, Blocked, Visible };
struct Target {
    std::uint32_t handle = 0;
    Vec3 point;
    // Visible body point used for acquisition, independent of the aim region.
    std::optional<Vec3> fovPoint;
};
inline constexpr unsigned maxTargets = 128;
struct Snapshot { Vec3 eye; std::array<Target, maxTargets> targets{}; unsigned count = 0; };
struct Query {
    // Preferred region, never an exclusive hitbox filter.
    Point point = Point::Chest; bool ignoreTeam = false; std::uint32_t preferred = 0;
    float fov = 180; weapons::Angle reference;
};
using Capture = bool (*)(const weapons::Sample&, Query, Snapshot&);
// First collision on the final shot ray. A valid miss returns true with handle=0.
using ShotCapture = bool (*)(const weapons::Sample&, weapons::Angle finalView, float range, bool ignoreTeam, std::uint32_t& handle);
ShotCapture ResolveShotCapture();
const char* ShotStatus();
bool ShotEnd(Vec3 eye, weapons::Angle view, weapons::Angle punch, float range, Vec3& end);
struct HitboxPoint { Vec3 position; bool preferred = false; };
using CheckVisibility = Visibility (*)(void*, Vec3);
// A visible body point must enter FOV. Then prefer the requested visible region,
// even outside FOV; fall back to the acquisition point when that region is hidden.
bool SelectPoint(Vec3 eye, std::span<const HitboxPoint>, const Query&, void*, CheckVisibility, Vec3&, Vec3* fovPoint = nullptr);
// Returns null unless the same build/hash gate as Players and every entry match.
Capture ResolveCapture();
struct Candidate { std::uintptr_t pawn = 0; std::uint32_t handle = 0; Vec3 origin; int team = 0; };
using ResolveTarget = bool (*)(void*, const Candidate&, Target&);
void CollectTargets(std::span<const Candidate>, void*, ResolveTarget, Snapshot&);
bool ReadCandidate(const players::Memory&, std::uintptr_t world, std::uint32_t handle, Candidate&);
using Transform = players::Transform;
bool TransformPoint(Vec3 local, const Transform&, Vec3& world);
struct TraceResult { std::uintptr_t hit = 0; float fraction = 0; bool startSolid = false; };
TraceResult DecodeTrace(std::span<const unsigned char, 192>);
struct ShotTraceBinding {
    using Filter = std::uintptr_t (*)(void*, std::uintptr_t ignored, std::uint64_t mask, unsigned char layer, unsigned char flags);
    using World = void (*)(std::uintptr_t manager, const Vec3*, const Vec3*, void* filter, void* result);
    using Players = void (*)(const Vec3*, const Vec3*, void* filter, void* result, float minimum, float maximum, float fraction);
    Filter filter = nullptr; World world = nullptr; Players players = nullptr;
};
TraceResult TracePlayers(const ShotTraceBinding&, std::uintptr_t manager, std::uintptr_t ignored, Vec3 start, Vec3 end);
Visibility ClassifyTrace(std::uintptr_t target, std::uintptr_t hit, float fraction, bool startSolid);
}
