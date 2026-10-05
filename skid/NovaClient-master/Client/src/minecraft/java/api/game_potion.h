#pragma once
#include "minecraft/java/api/game_itemuse.h"

namespace nova::java {
// Main game thread only. Slots are hotbar indices 0..8.
bool HotbarHasHealingSplashPotion(int slot);
bool InventoryHasHealingSplashPotion(int slot); // Inventory indices 0..35.
// Revalidates the held stack and uses the vanilla controller once, without aiming.
bool UseSelectedHealingPotion();
}
