#include "minecraft/java/api/game_inventory.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"

namespace nova::java {
namespace {
using compat::mc263::access::Frame;
using compat::mc263::access::Member;
struct Mapping {
    Member show{}, inventory{}, carried{}, inventoryField{}, status{}, openStatus{};
};
#define NOVA_INVENTORY_SCREEN_MAPPING
#define NOVA_INVENTORY_SCREEN(key, owner, name, desc) .key = {owner, name, desc},
constexpr Mapping lunar{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr Mapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr Mapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_INVENTORY_SCREEN
#undef NOVA_INVENTORY_SCREEN_MAPPING
const Mapping& Names() { return compat::IsLegacyLunar() ? lunar : compat::UsesModernRuntime() ? modern : forge; }
}

bool OpenPlayerInventory(jobject& screen) {
    screen = nullptr;
    Frame f;
    if (!f || compat::ScreenOpen()) return false;
    const auto& n = Names();
    auto player = f.player();
    if (!player || !InventoryCursorEmpty()) return false;
    auto created = f.create(n.inventory, player);
    if (!created || !f.check()) return false;
    if (compat::IsLegacyLunar()) {
        // Same inventory-open notification that vanilla 1.8 sends before GuiInventory.
        auto action = f.stat(n.openStatus);
        auto packet = action ? f.create(n.status, action) : nullptr;
        if (!packet || !f.check() || !SendPacket(packet)) return false;
    }
    if (!f.invoke(f.client(), n.show, created)) return false;
    auto actual = CurrentScreen();
    if (!actual || !f.check() || !f.env->IsSameObject(created, actual)) return false;
    screen = f.keep(actual);
    return screen != nullptr;
}

bool InventoryCursorEmpty() {
    Frame f;
    if (!f) return false;
    const auto& n = Names();
    jobject container = nullptr;
    if (compat::IsLegacyLunar()) container = f.get(f.player(), n.inventoryField);
    else if (!PlayerMenu(container)) return false;
    if (!container || !f.check()) return false;
    auto stack = f.call(container, n.carried);
    if (!f.check()) return false;
    ItemFacts facts;
    return ReadItemFacts(stack, facts) && facts.empty;
}
}
