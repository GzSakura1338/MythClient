#include "cs2/sdk/players/Players.h"
#include "cs2/sdk/players/Offsets.h"
#include "cs2/sdk/players/Transform.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace nova::cs2::sdk::players {
namespace {
using Address = std::uintptr_t;
struct Identity {
    Address entity = 0, unknown = 0;
    std::uint32_t handle = 0, nameIndex = 0;
    Address name = 0, designer = 0, unknown28 = 0;
    std::uint32_t flags = 0;
};
static_assert(offsetof(Identity, handle) == 0x10 && offsetof(Identity, flags) == 0x30);
bool String(const Memory& memory, Address address, std::string& output) {
    output.clear();
    for (unsigned i = 0; i < 128; ++i) {
        char c = 0;
        if (!memory.read(address + i, c)) { output.clear(); return false; }
        if (!c) return true;
        output += static_cast<unsigned char>(c) < 32 || c == 127 ? ' ' : c;
    }
    output.clear(); return false;
}
bool Entry(const Memory& memory, Address system, unsigned index, Identity& result, Address& slot) {
    if (!index || index > offsets::indexMask) return false;
    Address chunk = 0;
    if (!memory.read(system + 0x10 + (index / offsets::chunkSize) * sizeof(Address), chunk) || !chunk) return false;
    slot = chunk + (index % offsets::chunkSize) * offsets::stride;
    Address back = 0;
    return memory.read(slot, result) && result.entity && result.handle != 0xFFFFFFFF &&
        (result.handle & offsets::indexMask) == index &&
        memory.read(result.entity + offsets::identity, back) && back == slot;
}
std::uint32_t Handle(const Identity& entry) {
    return entry.handle - ((entry.flags & 1u) << 15);
}
bool Resolve(const Memory& memory, Address system, std::uint32_t handle, Identity& result, Address& slot) {
    return handle != 0xFFFFFFFF && Entry(memory, system, handle & offsets::indexMask, result, slot) && Handle(result) == handle;
}
bool Finite(Vec3 value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) &&
        std::abs(value.x) < 1000000 && std::abs(value.y) < 1000000 && std::abs(value.z) < 1000000;
}
}
bool ReadSnapshot(const Memory& memory, Address client, Address engine, Snapshot& result) {
    result = {};
    Address network = 0, system = 0, local = 0, localPawn = 0;
    int signOn = 0, count = 0;
    if (!memory.read(engine + offsets::networkClient, network) || !network ||
        !memory.read(network + offsets::signOnState, signOn) || signOn != 6 ||
        !memory.read(network + offsets::maxClients, count) || count < 1 || count > 128 ||
        !memory.read(client + offsets::entitySystem, system) || !system ||
        !memory.read(client + offsets::localController, local) || !local ||
        !memory.read(client + offsets::localPawn, localPawn) ||
        !memory.read(client + offsets::viewMatrix, result.matrix)) return false;
    for (const auto& row : result.matrix) for (float value : row) if (!std::isfinite(value)) return false;
    std::uint8_t ownTeam = 0;
    if (!memory.read(local + offsets::team, ownTeam)) return false;
    result.localTeam = ownTeam;
    for (int i = 1; i <= count; ++i) {
        Identity controller, pawn; Address controllerSlot = 0, pawnSlot = 0;
        if (!Entry(memory, system, i, controller, controllerSlot) || controller.entity == local) continue;
        std::string kind;
        if (!String(memory, controller.designer, kind) || kind != "cs_player_controller") continue;
        std::uint32_t pawnHandle = 0;
        if (!memory.read(controller.entity + offsets::controllerPawn, pawnHandle) ||
            !Resolve(memory, system, pawnHandle, pawn, pawnSlot) || pawn.entity == localPawn) continue;
        // Build 14181 also uses this designer name for live controller-owned
        // pawns. Do not confuse it with an unowned menu/precache entity: the
        // controller handle, serial, health and session checks remain required.
        if (!String(memory, pawn.designer, kind) ||
            (kind != "player" && kind != "c_cs_player_for_precache")) continue;
        if (std::any_of(result.players.begin(), result.players.end(), [&](const auto& value) { return value.handle == pawnHandle; })) continue;
        Player player; player.handle = pawnHandle;
        Address scene = 0, collision = 0, name = 0;
        std::uint8_t life = 0, team = 0, dormant = 0;
        if (!memory.read(pawn.entity + offsets::health, player.health) || player.health <= 0 || player.health > 10000 ||
            !memory.read(pawn.entity + offsets::lifeState, life) || life != 0 ||
            !memory.read(pawn.entity + offsets::team, team) || (team != 2 && team != 3) ||
            !memory.read(pawn.entity + offsets::sceneNode, scene) || !scene ||
            !memory.read(scene + offsets::dormant, dormant) || dormant ||
            !memory.read(scene + offsets::origin, player.origin) || !Finite(player.origin) ||
            !memory.read(pawn.entity + offsets::collision, collision) || !collision ||
            !memory.read(collision + offsets::mins, player.mins) || !Finite(player.mins) ||
            !memory.read(collision + offsets::maxs, player.maxs) || !Finite(player.maxs)) continue;
        const auto& lo = player.mins; const auto& hi = player.maxs;
        if (lo.x >= hi.x || lo.y >= hi.y || lo.z >= hi.z || hi.x - lo.x > 256 || hi.y - lo.y > 256 || hi.z - lo.z > 256) continue;
        // CS2 sometimes publishes zero max health; never hide a valid live
        // player because an optional bar denominator isn't ready.
        if (!memory.read(pawn.entity + offsets::maximumHealth, player.maximum) || player.maximum <= 0 || player.maximum > 10000)
            player.maximum = 100;
        Transform transform;
        if (memory.read(scene + offsets::nodeToWorld, transform)) {
            player.transformed = true;
            for (int corner = 0; corner < 8; ++corner) {
                const Vec3 local{corner & 1 ? hi.x : lo.x, corner & 2 ? hi.y : lo.y, corner & 4 ? hi.z : lo.z};
                if (!TransformPoint(local, transform, player.worldCorners[corner])) { player.transformed = false; break; }
            }
        }
        if (memory.read(pawn.entity + offsets::armor, player.armor)) player.armor = std::clamp(player.armor, 0, 100);
        if (memory.read(controller.entity + offsets::controllerName, name) && name) String(memory, name, player.name);
        player.team = team;
        // Recheck identity and the relationship after copying, so deletion,
        // respawn and reuse of an entity slot discard this frame's sample.
        Identity controllerAfter, pawnAfter; Address a = 0, b = 0;
        std::uint32_t handleAfter = 0;
        if (!Entry(memory, system, i, controllerAfter, a) || a != controllerSlot ||
            controllerAfter.entity != controller.entity || Handle(controllerAfter) != Handle(controller) ||
            !memory.read(controller.entity + offsets::controllerPawn, handleAfter) || handleAfter != pawnHandle ||
            !Resolve(memory, system, pawnHandle, pawnAfter, b) || b != pawnSlot || pawnAfter.entity != pawn.entity) continue;
        result.players.push_back(std::move(player));
    }
    Address systemAfter = 0, networkAfter = 0, localAfter = 0;
    if (!memory.read(client + offsets::entitySystem, systemAfter) || systemAfter != system ||
        !memory.read(client + offsets::localController, localAfter) || localAfter != local ||
        !memory.read(engine + offsets::networkClient, networkAfter) || networkAfter != network ||
        !memory.read(network + offsets::signOnState, signOn) || signOn != 6) { result = {}; return false; }
    return true;
}
}
