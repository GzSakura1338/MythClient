#pragma once

#include <string>
#include <vector>

#include "minecraft/java/api/game_inventory.h"

// The scoring rules the inventory modules share, ported from Starain's
// InventoryUtils.
//
// Everything here is answered from a Snapshot: one walk of the inventory, every
// stack reduced to the fact block the layer above already reads in a single call.
// No rule below touches the JVM, which is the point -- Starain runs these same
// rules over forty stacks several times a tick, and doing that across the JNI
// boundary would cost thousands of crossings a tick.

namespace nova::inventory {

long long NowMillis();

// Starain's TimerUtils. The clock starts at zero rather than at construction, so
// the first Delay() on a fresh timer is always true -- which is what the modules
// rely on for their first action after a reset.
struct Timer {
    long long last = 0;

    void Reset() { last = NowMillis(); }
    [[nodiscard]] bool Delay(long long millis) const { return NowMillis() - last >= millis; }
};

// The fact block and the vocabularies it uses live with the layer that reads them;
// the rules below are written against them directly.
using java::BlockKind;
using java::ItemFacts;
using java::ItemKind;
using java::KnownItem;
using java::KnownMaterial;
using enum java::BlockKind;
using enum java::ItemKind;

struct Entry {
    int         slot = -1;  // Inventory.items index for items, armor index for armor
    ItemFacts   facts;
    std::string name;
};

struct Snapshot {
    std::vector<Entry> items;    // Inventory.items, 36 entries, empties included
    std::vector<Entry> armor;    // Inventory.armor, 4 entries
    std::vector<Entry> offhand;  // Inventory.offhand, 1 entry
};

// Reads the whole inventory once. False when the inventory cannot be read at all,
// in which case the snapshot is empty rather than half-filled.
bool Scan(Snapshot& out);

// --- scoring -----------------------------------------------------------------
//
// Each returns 0 for a stack the rule does not apply to, exactly as the original
// does, so a caller can compare two stacks without checking what they are first.

float Protection(const ItemFacts& facts);
float SwordDamage(const ItemFacts& facts);
float AxeDamage(const ItemFacts& facts);
float ToolScore(const ItemFacts& facts);
float CrossbowScore(const ItemFacts& facts);
float PunchBowScore(const ItemFacts& facts);
float PowerBowScore(const ItemFacts& facts);

bool IsGodItem(const ItemFacts& facts);
bool IsGodAxe(const ItemFacts& facts);
bool IsSharpnessAxe(const ItemFacts& facts);
bool IsPunchBow(const ItemFacts& facts, const std::string& name);
bool IsPowerBow(const ItemFacts& facts, const std::string& name);
bool IsEnchantedGApple(const ItemFacts& facts);
bool IsEndCrystal(const ItemFacts& facts);
bool IsKBBall(const ItemFacts& facts);
bool IsKBStick(const ItemFacts& facts);
bool IsGoldenHead(const ItemFacts& facts);

// The two halves of the original's isItemValid / isValidStack, split because one
// needs the stack and the other only its name.
bool IsNameValid(const ItemFacts& facts, const std::string& name);
bool IsValidBuildingBlock(const ItemFacts& facts, const std::string& name);

// True when the item is worth keeping at all, ignoring anything about count.
bool IsCommonItemUseful(const ItemFacts& facts);

// --- whole-inventory queries -------------------------------------------------

const Entry* At(const Snapshot& snapshot, int slot);

float BestArmorScore(const Snapshot& snapshot, int armorSlot);
float CurrentArmorScore(const Snapshot& snapshot, int armorSlot);

float BestSwordDamage(const Snapshot& snapshot);
float BestPickaxeScore(const Snapshot& snapshot);
float BestAxeScore(const Snapshot& snapshot);
float BestShovelScore(const Snapshot& snapshot);
float BestCrossbowScore(const Snapshot& snapshot);
float BestPunchBowScore(const Snapshot& snapshot);
float BestPowerBowScore(const Snapshot& snapshot);

// Slot of the best candidate, or -1 when there is none.
int BestSword(const Snapshot& snapshot);
int BestPickaxe(const Snapshot& snapshot);
int BestAxe(const Snapshot& snapshot);
int BestShapeAxe(const Snapshot& snapshot);
int BestShovel(const Snapshot& snapshot);
int BestCrossbow(const Snapshot& snapshot);
int BestPunchBow(const Snapshot& snapshot);
int BestPowerBow(const Snapshot& snapshot);

int BestBlock(const Snapshot& snapshot);
int WorstBlock(const Snapshot& snapshot);
int BestProjectile(const Snapshot& snapshot);
int WorstProjectile(const Snapshot& snapshot);
int WorstArrow(const Snapshot& snapshot);
int FindFishingRod(const Snapshot& snapshot);

int FindItem(const Snapshot& snapshot, KnownItem item);
int CountItem(const Snapshot& snapshot, KnownItem item);
bool HasItem(const Snapshot& snapshot, KnownItem item);
int CountBuildingBlocks(const Snapshot& snapshot);

int FirstEmptyInventorySlot(const Snapshot& snapshot);
int FirstEmptyHotbarSlot(const Snapshot& snapshot);

// True when a stack's name says the server is showing a menu we must not touch.
bool ShouldDisableFeatures(const Snapshot& snapshot);

}
