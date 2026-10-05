#include "cs2/sdk/aim/AimSnapshot.h"
#include "cs2/sdk/aim/Offsets.h"
#include "cs2/sdk/players/EntityAccess.h"
#include "cs2/sdk/weapons/Offsets.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <atomic>

namespace nova::cs2::sdk::aim {
namespace {
using Address = std::uintptr_t;
namespace p = players::offsets;
namespace w = weapons::offsets;
std::atomic<const char*> shotStatus{"not sampled"};
bool Finite(Vec3 a) { return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z) &&
    std::abs(a.x) < 1000000 && std::abs(a.y) < 1000000 && std::abs(a.z) < 1000000; }
bool Copy(void*, Address address, void* output, std::size_t size) {
    if (address < 0x10000 || address > 0x00007FFFFFFFFFFFULL - size) return false;
    __try { std::memcpy(output, reinterpret_cast<const void*>(address), size); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool String(const players::Memory& m, Address address, char* out, unsigned length) {
    for (unsigned i = 0; i < length; ++i) { if (!m.read(address + i, out[i])) return false; if (!out[i]) return true; }
    return false;
}
bool Hitboxes(Address client, const players::Memory& m, Address pawn, Point requested,
    std::array<HitboxPoint, 64>& points, unsigned& size) {
    size = 0;
    using GetSet = Address (__fastcall*)(Address, unsigned);
    using Transforms = int (__fastcall*)(Address, Address, Transform*, int);
    using Lookup = int (__fastcall*)(Address, const char*);
    const Address set = reinterpret_cast<GetSet>(client + offsets::getSet)(pawn, 0);
    int count = 0; Address data = 0;
    if (!set || !m.read(set + offsets::setCount, count) || count <= 0 || count > 64 ||
        !m.read(set + offsets::setData, data) || !data) return false;
    std::array<Vec3, 64> locals{}; std::array<bool, 64> selected{}, preferred{};
    for (int i = 0; i < count; ++i) {
        const auto box = data + i * offsets::hitboxStride;
        int group = 0; Address name = 0; char bone[64]{};
        if (!m.read(box + offsets::group, group) || !m.read(box + offsets::boneName, name) || !name || !String(m, name, bone, sizeof(bone))) continue;
        const bool pelvis = std::strcmp(bone, "pelvis") == 0;
        // Include head, torso, arms and legs as fallback points, not equipment.
        if ((group < 1 || group > 7) && !pelvis) continue;
        if (reinterpret_cast<Lookup>(client + offsets::lookupBone)(pawn, bone) < 0) continue;
        preferred[i] = requested == Point::Head ? group == 1 : requested == Point::Chest ? group == 2 : pelvis;
        Vec3 lo, hi; std::uint8_t shape = 0;
        if (!m.read(box + offsets::minBounds, lo) || !m.read(box + offsets::maxBounds, hi) ||
            !m.read(box + offsets::shape, shape) || shape > 2 || !Finite(lo) || !Finite(hi)) continue;
        // Sphere: center is minBounds. Box/capsule: midpoint of its endpoints.
        locals[i] = shape == 1 ? lo : Vec3{(lo.x + hi.x) * .5f, (lo.y + hi.y) * .5f, (lo.z + hi.z) * .5f};
        selected[i] = true;
    }
    if (std::none_of(selected.begin(), selected.end(), [](bool v) { return v; })) return false;
    std::array<Transform, 64> transforms{};
    const int produced = reinterpret_cast<Transforms>(client + offsets::getTransforms)(pawn, set, transforms.data(), count);
    if (produced != count) return false;
    for (int i = 0; i < count; ++i)
        if (selected[i] && players::TransformPoint(locals[i], transforms[i], points[size].position)) {
            points[size].preferred = preferred[i]; ++size;
        }
    Address after = 0; int afterCount = 0;
    return size && m.read(set + offsets::setData, after) && after == data && m.read(set + offsets::setCount, afterCount) && afterCount == count;
}
TraceResult TraceLine(Address client, Address manager, Address pawn, Vec3 eye, Vec3 point) {
    // Build 14184 RVA 0x827320: RCX=manager, RDX=start, R8=end, R9=ignored entity,
    // stack arg5=64-bit mask, arg6=192-byte trace output. Ignore return value.
    using Line = void (__fastcall*)(Address, const Vec3*, const Vec3*, Address, std::uint64_t, void*);
    alignas(16) unsigned char result[offsets::traceResultSize]{};
    reinterpret_cast<Line>(client + offsets::traceLine)(manager, &eye, &point, pawn, offsets::traceMask, result);
    return DecodeTrace(result);
}
Visibility Trace(Address client, Address manager, Address pawn, Address target, Vec3 eye, Vec3 point) {
    const auto r = TraceLine(client, manager, pawn, eye, point);
    return ClassifyTrace(target, r.hit, r.fraction, r.startSolid);
}
bool RayContext(const weapons::Sample& sample, const players::Memory& m, Address client,
    Address& manager, Candidate& local, Vec3& eye, int& count) {
    const auto engine = reinterpret_cast<Address>(GetModuleHandleW(L"engine2.dll"));
    const auto& id = sample.weapon.identity;
    Address check = 0, service = 0, owner = 0, table = 0, getter = 0, physics = 0;
    int signOn = 0;
    if (!client || !engine || sample.weapon.clip <= 0 || sample.weapon.reload || sample.weapon.switchingSilencer ||
        !m.read(engine + p::networkClient, check) || check != id.network ||
        !m.read(id.network + p::signOnState, signOn) || signOn != 6 ||
        !m.read(client + p::entitySystem, check) || check != id.world ||
        !ReadCandidate(m, id.world, id.pawnHandle, local) || local.pawn != id.pawn ||
        !m.read(client + p::localPawn, check) || check != id.pawn ||
        !m.read(id.network + p::maxClients, count) || count < 1 || count > 128 ||
        !m.read(id.pawn + w::weaponServices, service) || !service ||
        !m.read(service + offsets::serviceOwner, owner) || owner != id.pawn ||
        !m.read(service, table) || !table || !m.read(table + 0x100, getter) || getter != client + offsets::shootPosition ||
        !m.read(client + offsets::traceManager, manager) || !manager || !m.read(manager, physics) || !physics) return false;
    using Shoot = Vec3* (__fastcall*)(Address, Vec3*);
    eye = {std::numeric_limits<float>::quiet_NaN(), 0, 0};
    reinterpret_cast<Shoot>(getter)(service, &eye);
    return Finite(eye);
}
bool CaptureShot(const weapons::Sample& sample, weapons::Angle view, float range, bool ignoreTeam, std::uint32_t& handle) {
    handle = 0;
    const auto client = reinterpret_cast<Address>(GetModuleHandleW(L"client.dll"));
    const players::Memory m{nullptr, Copy}; const auto& id = sample.weapon.identity;
    Address manager = 0; Candidate local; Vec3 eye, end; int count = 0;
    if (!RayContext(sample, m, client, manager, local, eye, count) || !ShotEnd(eye, view, sample.punch, range, end)) { shotStatus.store("origin/session"); return false; }
    const auto hit = TracePlayers({reinterpret_cast<ShotTraceBinding::Filter>(client + offsets::traceFilter),
        reinterpret_cast<ShotTraceBinding::World>(client + offsets::traceWorld),
        reinterpret_cast<ShotTraceBinding::Players>(client + offsets::clipPlayers)}, manager, id.pawn, eye, end);
    if (!std::isfinite(hit.fraction) || hit.fraction < 0 || hit.fraction > 1) { shotStatus.store("invalid fraction"); return false; }
    if (!hit.hit || hit.hit == id.pawn || hit.startSolid) {
        shotStatus.store(!hit.hit ? "no collision entity" : hit.hit == id.pawn ? "local pawn" : "solid flags"); return true;
    }
    Address identity = 0, controller = 0, controllerIdentity = 0, name = 0, check = 0;
    std::uint32_t stored = 0, flags = 0, owner = 0, pawn = 0; char kind[40]{};
    // Require a real controller -> full pawn handle -> same first-hit entity.
    // Props and stale/reused entity indices cannot masquerade as a player.
    if (!m.read(hit.hit + p::identity, identity) || !identity ||
        !m.read(identity + 0x10, stored) || !m.read(identity + 0x30, flags)) { shotStatus.store("hit identity"); return true; }
    const auto candidateHandle = stored - ((flags & 1u) << 15);
    Candidate candidate;
    if (!ReadCandidate(m, id.world, candidateHandle, candidate) || candidate.pawn != hit.hit) {
        if (m.read(identity + 0x20, name) && name && String(m, name, kind, sizeof(kind)))
            shotStatus.store(!std::strcmp(kind, "worldent") ? "world geometry" :
                (!std::strcmp(kind, "player") || !std::strcmp(kind, "cs_player_pawn")) ? "player state/identity" : "non-player entity");
        else shotStatus.store("non-player/dead/immune");
        return true;
    }
    if (ignoreTeam && players::SameTeam(local.team, candidate.team)) { shotStatus.store("teammate"); return true; }
    if (!m.read(hit.hit + offsets::pawnController, owner) ||
        !players::ResolveEntity(m, id.world, owner, controller) ||
        !m.read(controller + p::identity, controllerIdentity) || !controllerIdentity ||
        !m.read(controllerIdentity + 0x20, name) || !name || !String(m, name, kind, sizeof(kind)) ||
        std::strcmp(kind, "cs_player_controller") || !m.read(controller + p::controllerPawn, pawn) || pawn != candidateHandle) { shotStatus.store("controller ownership"); return true; }
    if (!players::ResolveEntity(m, id.world, candidateHandle, check) || check != hit.hit ||
        !m.read(client + p::entitySystem, check) || check != id.world ||
        !m.read(client + p::localPawn, check) || check != id.pawn) return false;
    shotStatus.store("target"); handle = candidateHandle; return true;
}
bool CaptureNative(const weapons::Sample& sample, Query query, Snapshot& out) {
    out = {};
    const auto client = reinterpret_cast<Address>(GetModuleHandleW(L"client.dll"));
    const auto engine = reinterpret_cast<Address>(GetModuleHandleW(L"engine2.dll"));
    const players::Memory m{nullptr, Copy}; const auto& id = sample.weapon.identity;
    Address check = 0, manager = 0; Candidate local; int count = 0;
    if (!RayContext(sample, m, client, manager, local, out.eye, count)) return false;
    struct Ranked { Candidate candidate; float score; };
    std::array<Ranked, 128> candidates{}; unsigned size = 0;
    constexpr double rad = 3.14159265358979323846 / 180.;
    const Vec3 forward{static_cast<float>(std::cos(sample.view.pitch * rad) * std::cos(sample.view.yaw * rad)),
        static_cast<float>(std::cos(sample.view.pitch * rad) * std::sin(sample.view.yaw * rad)), static_cast<float>(-std::sin(sample.view.pitch * rad))};
    Address controllerChunk = 0;
    if (!m.read(id.world + 0x10, controllerChunk) || !controllerChunk) return false;
    for (int i = 1; i <= count; ++i) {
        Address controller = 0, identity = controllerChunk + i * p::stride, name = 0, back = 0;
        std::uint32_t handle = 0; char kind[40]{};
        if (!m.read(identity, controller) || !controller || !m.read(controller + p::identity, back) || back != identity ||
            !m.read(identity + 0x20, name) || !name || !String(m, name, kind, sizeof(kind)) || std::strcmp(kind, "cs_player_controller") ||
            !m.read(controller + p::controllerPawn, handle) || handle == id.pawnHandle) continue;
        Candidate target;
        if (!ReadCandidate(m, id.world, handle, target) || (query.ignoreTeam && players::SameTeam(local.team, target.team)) ||
            std::any_of(candidates.begin(), candidates.begin() + size, [&](const Ranked& r) { return r.candidate.handle == handle; })) continue;
        const Vec3 delta{target.origin.x - out.eye.x, target.origin.y - out.eye.y, target.origin.z - out.eye.z};
        const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
        if (!std::isfinite(length) || length < .001f) continue;
        // Origin is only a cheap ranking hint. Final FOV and aim use a hitbox.
        const float score = handle == query.preferred ? -2.f : -(delta.x * forward.x + delta.y * forward.y + delta.z * forward.z) / length;
        candidates[size++] = {target, score};
    }
    std::sort(candidates.begin(), candidates.begin() + size, [](const Ranked& a, const Ranked& b) {
        return a.score != b.score ? a.score < b.score : a.candidate.handle < b.candidate.handle;
    });
    struct Context { Address client, manager, world, pawn; const players::Memory& memory; Query query; Vec3 eye; };
    Context context{client, manager, id.world, id.pawn, m, query, out.eye};
    std::array<Candidate, maxTargets> ordered{};
    for (unsigned i = 0; i < size; ++i) ordered[i] = candidates[i].candidate;
    CollectTargets({ordered.data(), size}, &context, [](void* data, const Candidate& candidate, Target& target) {
        const auto& c = *static_cast<Context*>(data);
        std::array<HitboxPoint, 64> points{}; unsigned count = 0;
        Candidate before, after;
        if (!ReadCandidate(c.memory, c.world, candidate.handle, before) || before.pawn != candidate.pawn ||
            before.team != candidate.team || !Hitboxes(c.client, c.memory, candidate.pawn, c.query.point, points, count)) return false;
        struct Ray { const Context& context; Address target; } ray{c, candidate.pawn};
        Vec3 fovPoint;
        const bool visible = SelectPoint(c.eye, {points.data(), count}, c.query, &ray, [](void* data, Vec3 point) {
            const auto& r = *static_cast<Ray*>(data);
            return Trace(r.context.client, r.context.manager, r.context.pawn, r.target, r.context.eye, point);
        }, target.point, &fovPoint);
        if (visible) target.fovPoint = fovPoint;
        // Movement is expected during animation updates. Validate identity and
        // liveness again without requiring the origin to remain bit-identical.
        return visible && ReadCandidate(c.memory, c.world, candidate.handle, after) &&
            after.pawn == candidate.pawn && after.team == candidate.team;
    }, out);
    return m.read(client + p::localPawn, check) && check == id.pawn &&
        m.read(client + p::entitySystem, check) && check == id.world &&
        m.read(engine + p::networkClient, check) && check == id.network;
}
}
bool ShotEnd(Vec3 eye, weapons::Angle view, weapons::Angle punch, float range, Vec3& end) {
    end = {};
    if (!Finite(eye) || !std::isfinite(view.pitch) || !std::isfinite(view.yaw) ||
        !std::isfinite(punch.pitch) || !std::isfinite(punch.yaw) || !std::isfinite(range) || range <= 0 || range > 100000) return false;
    constexpr double rad = 3.14159265358979323846 / 180.;
    const double pitch = (view.pitch + punch.pitch) * rad, yaw = (view.yaw + punch.yaw) * rad;
    end = {eye.x + static_cast<float>(std::cos(pitch) * std::cos(yaw) * range),
        eye.y + static_cast<float>(std::cos(pitch) * std::sin(yaw) * range), eye.z - static_cast<float>(std::sin(pitch) * range)};
    return Finite(end);
}
bool SelectPoint(Vec3 eye, std::span<const HitboxPoint> points, const Query& query,
    void* context, CheckVisibility visible, Vec3& out, Vec3* fovPoint) {
    if (!visible || !Finite(eye) || points.size() > 64 || !std::isfinite(query.fov) || query.fov < .1f || query.fov > 180 ||
        !std::isfinite(query.reference.pitch) || !std::isfinite(query.reference.yaw)) return false;
    constexpr double radians = 3.14159265358979323846 / 180.;
    const double pitch = query.reference.pitch * radians, yaw = query.reference.yaw * radians;
    const double fx = std::cos(pitch) * std::cos(yaw), fy = std::cos(pitch) * std::sin(yaw), fz = -std::sin(pitch);
    const double cutoff = std::cos(query.fov * radians);
    struct Ranked { unsigned index; double score; };
    std::array<Ranked, 64> ranked{}; unsigned count = 0;
    for (unsigned i = 0; i < points.size(); ++i) {
        const auto p = points[i].position;
        if (!Finite(p)) continue;
        const double x = static_cast<double>(p.x) - eye.x, y = static_cast<double>(p.y) - eye.y, z = static_cast<double>(p.z) - eye.z;
        const double length = std::hypot(x, y, z);
        if (length < .001) continue;
        const double score = std::clamp((x * fx + y * fy + z * fz) / length, -1., 1.);
        ranked[count++] = {i, score};
    }
    std::sort(ranked.begin(), ranked.begin() + count, [&](const Ranked& a, const Ranked& b) {
        return a.score != b.score ? a.score > b.score : a.index < b.index;
    });
    std::array<std::optional<Visibility>, 64> visibility{};
    const auto isVisible = [&](unsigned index) {
        auto& value = visibility[index];
        if (!value) value = visible(context, points[index].position);
        return *value == Visibility::Visible;
    };
    int acquired = -1;
    for (unsigned i = 0; i < count; ++i) {
        if (ranked[i].score + 1e-9 < cutoff) break;
        if (isVisible(ranked[i].index)) { acquired = static_cast<int>(ranked[i].index); break; }
    }
    if (acquired < 0) return false;
    out = points[acquired].position;
    if (fovPoint) *fovPoint = out;
    for (unsigned i = 0; i < count; ++i) {
        const auto index = ranked[i].index;
        if (points[index].preferred && isVisible(index)) { out = points[index].position; break; }
    }
    return true;
}
void CollectTargets(std::span<const Candidate> candidates, void* context, ResolveTarget resolve, Snapshot& out) {
    out.count = 0;
    if (!resolve) return;
    for (const auto& candidate : candidates.first((std::min)(candidates.size(), std::size_t{maxTargets}))) {
        Target target;
        if (!candidate.handle || !resolve(context, candidate, target) || !Finite(target.point) ||
            (target.fovPoint && !Finite(*target.fovPoint))) continue;
        if (std::any_of(out.targets.begin(), out.targets.begin() + out.count,
            [&](const Target& t) { return t.handle == candidate.handle; })) continue;
        target.handle = candidate.handle;
        out.targets[out.count++] = target;
    }
}
bool ReadCandidate(const players::Memory& m, Address world, std::uint32_t handle, Candidate& out) {
    out = {}; Address scene = 0; int health = 0; std::uint8_t life = 0, team = 0, dormant = 0, immune = 0;
    if (!players::ResolveEntity(m, world, handle, out.pawn) ||
        !m.read(out.pawn + p::health, health) || health <= 0 || health > 10000 ||
        !m.read(out.pawn + p::lifeState, life) || life ||
        !m.read(out.pawn + p::team, team) || (team != 2 && team != 3) ||
        !m.read(out.pawn + offsets::immunity, immune) || immune ||
        !m.read(out.pawn + p::sceneNode, scene) || !scene || !m.read(scene + p::dormant, dormant) || dormant ||
        !m.read(scene + p::origin, out.origin) || !Finite(out.origin)) return false;
    out.handle = handle; out.team = team; return true;
}
bool TransformPoint(Vec3 v, const Transform& t, Vec3& out) {
    return players::TransformPoint(v, t, out);
}
TraceResult DecodeTrace(std::span<const unsigned char, 192> bytes) {
    TraceResult result;
    std::memcpy(&result.hit, bytes.data() + offsets::traceHit, sizeof(result.hit));
    std::memcpy(&result.fraction, bytes.data() + offsets::traceFraction, sizeof(result.fraction));
    result.startSolid = bytes[offsets::traceStartSolid] != 0;
    // Build 14184 RVA 0x1773970 sets +0xBC for every completed point ray
    // (shape type +0x28 == 0), including ordinary hits. It is NOT allSolid.
    return result;
}
TraceResult TracePlayers(const ShotTraceBinding& binding, Address manager, Address ignored, Vec3 start, Vec3 end) {
    if (!binding.filter || !binding.world || !binding.players || !manager || !ignored || !Finite(start) || !Finite(end))
        return {0, std::numeric_limits<float>::quiet_NaN(), false};
    // Same two stages as the native crosshair query (0xC9AB10): world/props
    // with layer 3, then gameplay player hitboxes with layer 4. The generic
    // 0x827320 helper strips contents bit 2 and cannot replace this step.
    alignas(16) unsigned char filter[80]{};
    alignas(16) unsigned char result[offsets::traceResultSize]{};
    binding.filter(filter, ignored, offsets::traceMask, 3, 15);
    binding.world(manager, &start, &end, filter, result);
    const auto world = DecodeTrace(result);
    if (!std::isfinite(world.fraction) || world.fraction < 0 || world.fraction > 1 || world.startSolid) return world;
    binding.filter(filter, ignored, offsets::traceMask, 4, 15);
    // Identical ray endpoints in both passes. -1 tells the native player pass
    // to use the world fraction, so a player behind a nearer wall never wins.
    binding.players(&start, &end, filter, result, 0.f, 60.f, -1.f);
    return DecodeTrace(result);
}
Visibility ClassifyTrace(Address target, Address hit, float fraction, bool startSolid) {
    if (!target || !std::isfinite(fraction) || fraction < 0 || fraction > 1) return Visibility::Unknown;
    if (startSolid) return Visibility::Blocked;
    if (hit == target) return Visibility::Visible;
    return hit == 0 && fraction == 1.f ? Visibility::Visible : Visibility::Blocked;
}
Capture ResolveCapture() {
    if (!players::Available()) return nullptr;
    const auto client = reinterpret_cast<Address>(GetModuleHandleW(L"client.dll"));
    if (!client) return nullptr;
    constexpr unsigned char set[]{0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x81,0xEC,0x40,0x01};
    constexpr unsigned char transforms[]{0x48,0x89,0x5C,0x24,0x18,0x56,0x57,0x41,0x54,0x41,0x56,0x41,0x57,0x48,0x83,0xEC,0x20};
    constexpr unsigned char lookup[]{0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0x89,0x30,0x03,0x00,0x00,0x48,0x8B,0xDA};
    constexpr unsigned char shoot[]{0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0x49,0x38,0x48,0x8B,0xDA,0x48,0x8B,0x01};
    constexpr unsigned char trace[]{0x48,0x8B,0xC4,0x48,0x89,0x58,0x08,0x48,0x89,0x68,0x10,0x48,0x89,0x70,0x18,0x57};
    __try {
        if (std::memcmp(reinterpret_cast<void*>(client + offsets::getSet), set, sizeof(set)) ||
            std::memcmp(reinterpret_cast<void*>(client + offsets::getTransforms), transforms, sizeof(transforms)) ||
            std::memcmp(reinterpret_cast<void*>(client + offsets::lookupBone), lookup, sizeof(lookup)) ||
            std::memcmp(reinterpret_cast<void*>(client + offsets::shootPosition), shoot, sizeof(shoot)) ||
            std::memcmp(reinterpret_cast<void*>(client + offsets::traceLine), trace, sizeof(trace))) return nullptr;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    return CaptureNative;
}
ShotCapture ResolveShotCapture() {
    if (!ResolveCapture()) return nullptr;
    const auto client = reinterpret_cast<Address>(GetModuleHandleW(L"client.dll"));
    constexpr unsigned char filterBytes[]{0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xEC,0x20};
    constexpr unsigned char worldBytes[]{0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x6C,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x81,0xEC,0x80};
    constexpr unsigned char playerBytes[]{0x48,0x8B,0xC4,0x48,0x89,0x58,0x08,0x48,0x89,0x70,0x10,0x48,0x89,0x78,0x20,0x55};
    __try {
        if (std::memcmp(reinterpret_cast<void*>(client + offsets::traceFilter), filterBytes, sizeof(filterBytes)) ||
            std::memcmp(reinterpret_cast<void*>(client + offsets::traceWorld), worldBytes, sizeof(worldBytes)) ||
            std::memcmp(reinterpret_cast<void*>(client + offsets::clipPlayers), playerBytes, sizeof(playerBytes))) return nullptr;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    return CaptureShot;
}
const char* ShotStatus() { return shotStatus.load(); }
}
