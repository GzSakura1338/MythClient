#pragma once

#include "base.h"
#include "minecraft/java/api/game_entity.h"

namespace nova::java {

// --- the player's inventory -------------------------------------------------
//
// Everything the inventory modules ask of a stack is answered from one read. A
// stack is touched by so many scoring rules -- eight enchantment lookups, a
// class test, a material test, a damage test -- that asking the game one question
// at a time would put dozens of JNI calls on every slot of every tick, and the
// whole inventory is walked repeatedly. ReadItemFacts costs one call and answers
// all of them, so the scoring itself lives in C++ and stays cheap.

// What the item is, as far as placement and switching care.
enum ItemKind : unsigned {
    kItemOther        = 0u,
    kItemBlock        = 1u << 0,
    kItemSword        = 1u << 1,
    kItemAxe          = 1u << 2,
    kItemPickaxe      = 1u << 3,
    kItemShovel       = 1u << 4,
    kItemBow          = 1u << 5,
    kItemCrossbow     = 1u << 6,
    kItemArrow        = 1u << 7,
    kItemFishingRod   = 1u << 8,
    kItemArmor        = 1u << 9,
    kItemHead         = 1u << 10,  // PlayerHeadItem
    kItemNameBlock    = 1u << 11,  // ItemNameBlockItem -- a placeable that renames
    kItemBook         = 1u << 12,
    kItemBottle       = 1u << 13,  // ExperienceBottleItem
    kItemFirework     = 1u << 14,
    kItemSkullBlock   = 1u << 15,  // the block of a BlockItem is a SkullBlock
    kItemElytra       = 1u << 16,
};

// Which vanilla item it is, for the identity tests the scoring rules make. The
// order is arbitrary but stable; Nothing means "none of them".
enum class KnownItem : int {
    Nothing = 0,
    GoldenAxe,
    SlimeBall,
    Stick,
    EnchantedGoldenApple,
    GoldenApple,
    EndCrystal,
    TotemOfUndying,
    Compass,
    WaterBucket,
    LavaBucket,
    Arrow,
    Snowball,
    Egg,
    FishingRod,
    WoodenAxe,
    StoneAxe,
    IronAxe,
    DiamondAxe,
    WheatSeeds,
    BeetrootSeeds,
    MelonSeeds,
    PumpkinSeeds,
    FlintAndSteel,
    Bow,
    EnderPearl,
    FireCharge,
    EnchantingTableBlock,
    CobwebBlock,
    StoneBlock,
    OakLogBlock,
    DirtBlock,
    WindCharge,
    Elytra,
    FireworkRocket,
    Mace,
};

// Which armour material it is, or Nothing.
enum class KnownMaterial : int {
    Nothing = 0,
    Leather,
    Chain,
    Iron,
    Gold,
    Diamond,
    Netherite,
};

// The armour slot a piece belongs to, matching EquipmentSlot's order, or -1.
enum ArmorSlot : int {
    kArmorHead = 0,
    kArmorChest = 1,
    kArmorLegs = 2,
    kArmorFeet = 3,
    kArmorNone = -1,
};

// What the block a BlockItem places is, for the rules that refuse certain
// building blocks.
enum BlockKind : unsigned {
    kBlockFlower = 1u << 0,
    kBlockBush   = 1u << 1,
    kBlockFungus = 1u << 2,
    kBlockCrop   = 1u << 3,
    kBlockSlab   = 1u << 4,
};

struct ItemFacts {
    bool     empty = true;
    int      count = 0;
    int      damageValue = 0;
    int      maxDamage = 0;
    int      maxStack = 0;
    unsigned kinds = kItemOther;
    KnownItem item = KnownItem::Nothing;
    KnownMaterial material = KnownMaterial::Nothing;
    int      armorSlot = kArmorNone;

    // Trimming speed against the block the tool is meant for; the scoring rules
    // compare tools by it. Zero when the item is not a tool.
    float    destroySpeed = 0.0f;
    // SwordItem.getDamage, only meaningful when kinds has kItemSword.
    float    swordDamage = 0.0f;

    int      sharpness = 0;
    int      knockback = 0;
    int      punch = 0;
    int      power = 0;
    int      infinity = 0;
    int      flame = 0;
    int      efficiency = 0;
    int      protection = 0;
    int      quickCharge = 0;
    int      multishot = 0;
    int      piercing = 0;

    // The sharpness damage bonus, which the game computes for us rather than
    // exposing as a table.
    float    sharpnessBonus = 0.0f;

    // True when the block this item places is one of the forty-seven the scoring
    // refuses to count as a building block.
    bool     blacklistedBlock = false;
    unsigned blockKinds = 0;
};

// Reads every field above in one JNI call. False when the stack cannot be read.
bool ReadItemFacts(jobject stack, ItemFacts& out);

// Native deep snapshots for reversible inventory transactions (1.9+ profiles).
// Copy returns a caller-owned local ref; Matches includes count and components/NBT.
bool CopyItemStack(jobject stack, jobject& out);
bool ItemStacksMatch(jobject left, jobject right);
// Deep identity snapshot with count normalized to one; keeps components/NBT.
bool ReadItemIdentity(jobject stack, jobject& out, bool& empty);

// ItemStack.getDisplayName().getString(). The only call here that reaches into a
// Java string; the callers that repeat it cache the answer.
bool ItemDisplayName(jobject stack, std::string& out);

// --- inventory slots --------------------------------------------------------
//
// Slot numbering is the menu's, not the player's: 0..8 are the hotbar, 9..35 the
// main rows, 36..39 the armour, 40 the offhand.

bool InventorySlot(int index, jobject& out);      // Inventory.items
bool InventoryArmor(int index, jobject& out);     // Inventory.armor
bool InventoryOffhand(int index, jobject& out);   // Inventory.offhand
int  InventoryItemCount();                        // items.size()
int  InventoryArmorCount();                       // armor.size()

// True when any non-empty stack in the player's inventory is the given vanilla
// item. This is the identity scan the originals call hasItem: one getItem and one
// pointer test per slot, with no fact block read, so it stays cheap enough to run
// once a tick for something as small as a compass.
bool InventoryHasItem(KnownItem item);
bool InventoryHasOffhand();

// --- the open container -----------------------------------------------------

// The screen currently open, or null. The caller owns the local reference.
jobject CurrentScreen();

bool ScreenIsContainerScreen(jobject screen);
bool ScreenIsInventoryScreen(jobject screen);

// AbstractContainerScreen.getMenu(), plus the two facts the modules want from it.
bool ScreenMenu(jobject screen, jobject& out);
bool MenuContainerId(jobject menu, int& out);
bool MenuIsPlayerInventory(jobject menu);
bool MenuIsChest(jobject menu);
bool MenuRowCount(jobject menu, int& out);
bool MenuSlotStack(jobject menu, int slot, jobject& out);

// Player.inventoryMenu, the menu the click helper wants when the player is not
// looking at a container.
bool PlayerMenu(jobject& out);
bool PlayerMenuContainerId(int& out);

// Screen.getTitle().getString().
bool ScreenTitle(jobject screen, std::string& out);
// Component.translatable(key).getString().
bool TranslatedText(const char* key, std::string& out);

// --- actions ----------------------------------------------------------------

// MultiPlayerGameMode.handleInventoryMouseClick(containerId, slot, button,
// clickType, player). `click` is a ClickKind below.
enum class ClickKind : int {
    Pickup = 0,
    QuickMove,
    Swap,
    Throw,
};

bool ClickMenuSlot(int containerId, int slot, int button, ClickKind click);

bool CloseContainer();

// Opens the actual player inventory; returns its local screen reference.
bool OpenPlayerInventory(jobject& screen);
// False on read failure as well as when the cursor carries an item.
bool InventoryCursorEmpty();

// --- packet classification, for the module that watches the outgoing stream ---

bool PacketIsContainerClose(jobject packet);
bool PacketIsMovePlayer(jobject packet);
bool PacketIsUseItem(jobject packet);
bool PacketIsUseItemOn(jobject packet);
bool PacketIsInteract(jobject packet);
bool PacketIsPlayerAction(jobject packet);

// ServerboundContainerClosePacket(int containerId).
bool SendContainerClose(int containerId);

}
