#include <atomic>
#include <cassert>
#include <cstdio>
#include <utility>
#include "minecraft/modules/misc/antibot/AntiBot.h"
#include "minecraft/java/api/game_player_list.h"

namespace fixture {
struct Entity { std::optional<bool> listed; } target;
int reads = 0;
}

namespace nova::java {
EntityHandle::~EntityHandle() = default;
EntityHandle::EntityHandle(EntityHandle&& other) noexcept : obj(std::exchange(other.obj, nullptr)) {}
EntityHandle& EntityHandle::operator=(EntityHandle&& other) noexcept {
    obj = std::exchange(other.obj, nullptr); return *this;
}
EntityHandle AdoptEntity(void* ref) { EntityHandle entity; entity.obj = ref; return entity; }
std::optional<bool> OnlinePlayerListContains(const EntityHandle& entity) {
    ++fixture::reads;
    return static_cast<fixture::Entity*>(entity.raw())->listed;
}
}

namespace nova::module {
namespace { std::atomic<bool> active{false}; }
// @PRODUCTION_ANTIBOT_API@
}

int main() {
    using namespace nova::module;
    using namespace fixture;
    auto entity = nova::java::AdoptEntity(&target);
    target.listed = false;
    assert(!antibot::IsBot(entity) && reads == 0);
    active = true;
    assert(antibot::IsBot(entity));
    target.listed = true;
    assert(!antibot::IsBot(entity));
    target.listed = false;
    assert(antibot::IsBot(entity)); // Removal takes effect immediately, no stale allowlist.
    target.listed.reset();
    assert(!antibot::IsBot(entity)); // Disconnected / failed read is not a bot verdict.
    const int before = reads;
    assert(!antibot::IsBot(nova::java::EntityHandle{}) && reads == before);
    active = false;
    target.listed = false;
    assert(!antibot::IsBot(entity) && reads == before);
    active = true;
    assert(antibot::IsBot(entity));
    // No IsPlayer(), health, name, visibility or age API exists in this fixture:
    // Starain's predicate applies to any entity's UUID, including mobs/NPCs.
    std::puts("AntiBot: source predicate, enable gate, live membership changes and unavailable data passed");
}
