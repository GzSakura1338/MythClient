#include "minecraft/java/api/game_inventory.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"

#include <string>
#include <vector>

#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/jni_lookup.h"

// The inventory layer.
//
// The one thing worth knowing before reading any of this: a stack is read once.
// Starain's inventory code asks the game a question at a time -- an enchantment
// level here, a class test there -- and because the inventory is walked several
// times per tick those questions add up to thousands of JNI crossings. ReadItemFacts
// answers all of them from a single crossing, and every scoring rule downstream is
// plain C++ over the result.

namespace nova::java {
namespace {

// --- classes ----------------------------------------------------------------
#define NOVA_FORGE1201_INVENTORY
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_INVENTORY

// Item classes, for the kind bitmask.
#define NOVA_FORGE1201_INVENTORY_TABLES
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_INVENTORY_TABLES

// --- srg members ------------------------------------------------------------

struct InventoryHandles {
    jclass    minecraft = nullptr;
    jclass    screen = nullptr;
    jclass    containerScreen = nullptr;
    jclass    inventoryScreen = nullptr;
    jclass    absScreen = nullptr;
    jclass    menu = nullptr;
    jclass    chestMenu = nullptr;
    jclass    slot = nullptr;
    jclass    clickType = nullptr;
    jclass    gameMode = nullptr;
    jclass    player = nullptr;
    jclass    inventory = nullptr;
    jclass    list = nullptr;
    jclass    stack = nullptr;
    jclass    item = nullptr;
    jclass    block = nullptr;
    jclass    state = nullptr;
    jclass    items = nullptr;
    jclass    blocks = nullptr;
    jclass    enchants = nullptr;
    jclass    enchantment = nullptr;
    jclass    helper = nullptr;
    jclass    mobType = nullptr;
    jclass    materials = nullptr;
    jclass    equipSlot = nullptr;
    jclass    component = nullptr;
    jclass    closePacket = nullptr;

    jclass    itemClasses[kItemClassCount] = {};
    jclass    blockClasses[kBlockClassCount] = {};

    jmethodID getInstance = nullptr;
    jmethodID absGetMenu = nullptr;
    jmethodID screenGetTitle = nullptr;
    jmethodID componentGetString = nullptr;
    jmethodID componentTranslatable = nullptr;
    jmethodID menuGetSlot = nullptr;
    jmethodID slotGetItem = nullptr;
    jmethodID chestRowCount = nullptr;
    jmethodID listGet = nullptr;
    jmethodID listSize = nullptr;
    jmethodID playerClose = nullptr;
    jmethodID playerGetInventory = nullptr;
    jmethodID handleClick = nullptr;
    jmethodID stackIsEmpty = nullptr;
    jmethodID stackGetItem = nullptr;
    jmethodID stackGetCount = nullptr;
    jmethodID stackGetName = nullptr;
    jmethodID stackDamageValue = nullptr;
    jmethodID stackMaxDamage = nullptr;
    jmethodID stackMaxStack = nullptr;
    jmethodID stackDestroySpeed = nullptr;
    jmethodID blockItemGetBlock = nullptr;
    jmethodID blockDefaultState = nullptr;
    jmethodID swordGetDamage = nullptr;
    jmethodID armorGetMaterial = nullptr;
    jmethodID armorGetSlot = nullptr;
    jmethodID helperGetLevel = nullptr;
    jmethodID enchantDamageBonus = nullptr;
    jmethodID closeCtor = nullptr;

    jfieldID  screenField = nullptr;
    jfieldID  playerField = nullptr;
    jfieldID  gameModeField = nullptr;
    jfieldID  menuContainerId = nullptr;
    jfieldID  invItems = nullptr;
    jfieldID  invArmor = nullptr;
    jfieldID  invOffhand = nullptr;
    jfieldID  playerMenu = nullptr;

    jobject   clickTypes[4] = {nullptr, nullptr, nullptr, nullptr};

    jobject   itemIdentities[kItemIdentityCount] = {};
    jobject   blockIdentities[kBlockIdentityCount] = {};
    jobject   enchantments[kEnchantmentCount] = {};
    jobject   materialObjects[kMaterialCount] = {};
    jobject   equipSlots[4] = {nullptr, nullptr, nullptr, nullptr};
    jobject   mobTypeUndefined = nullptr;

    // The three block states the tool scores are measured against.
    jobject   stoneState = nullptr;
    jobject   oakLogState = nullptr;
    jobject   dirtState = nullptr;

    jobject   blockBlacklist[kBlockBlacklistCount] = {};

    bool      ok = false;
};

InventoryHandles g_inv;
bool g_invLogged = false;

// SkullBlock is only wanted for the golden-head test, so it resolves the first
// time a block item is read rather than up front with everything else.
jclass g_skullBlock = nullptr;

bool ResolveInventory(JNIEnv* env) {
    if (!env)
        return false;
    if (g_inv.ok)
        return true;

    jobject loader = GetMinecraftClassLoader(env);
    jclass minecraft = nova_base::loadOrFind(env, loader, kMinecraftDot, kMinecraftSig);
    jclass screen = nova_base::loadOrFind(env, loader, kScreenDot, kScreenSig);
    jclass containerScreen = nova_base::loadOrFind(env, loader, kContainerDot, kContainerSig);
    jclass inventoryScreen = nova_base::loadOrFind(env, loader, kInvScreenDot, kInvScreenSig);
    jclass absScreen = nova_base::loadOrFind(env, loader, kAbsScreenDot, kAbsScreenSig);
    jclass menu = nova_base::loadOrFind(env, loader, kMenuDot, kMenuSig);
    jclass chestMenu = nova_base::loadOrFind(env, loader, kChestMenuDot, kChestMenuSig);
    jclass slot = nova_base::loadOrFind(env, loader, kSlotDot, kSlotSig);
    jclass clickType = nova_base::loadOrFind(env, loader, kClickTypeDot, kClickTypeSig);
    jclass gameMode = nova_base::loadOrFind(env, loader, kGameModeDot, kGameModeSig);
    jclass player = nova_base::loadOrFind(env, loader, kPlayerDot, kPlayerSig);
    jclass inventory = nova_base::loadOrFind(env, loader, kInventoryDot, kInventorySig);
    jclass list = nova_base::loadOrFind(env, loader, kListDot, kListSig);
    jclass stack = nova_base::loadOrFind(env, loader, kStackDot, kStackSig);
    jclass item = nova_base::loadOrFind(env, loader, kItemDot, kItemSig);
    jclass block = nova_base::loadOrFind(env, loader, kBlockDot, kBlockSig);
    jclass state = nova_base::loadOrFind(env, loader, kStateDot, kStateSig);
    jclass items = nova_base::loadOrFind(env, loader, kItemsDot, kItemsSig);
    jclass blocks = nova_base::loadOrFind(env, loader, kBlocksDot, kBlocksSig);
    jclass enchants = nova_base::loadOrFind(env, loader, kEnchantsDot, kEnchantsSig);
    jclass enchantment = nova_base::loadOrFind(env, loader, kEnchDot, kEnchSig);
    jclass helper = nova_base::loadOrFind(env, loader, kHelperDot, kHelperSig);
    jclass mobType = nova_base::loadOrFind(env, loader, kMobTypeDot, kMobTypeSig);
    jclass materials = nova_base::loadOrFind(env, loader, kMaterialsDot, kMaterialsSig);
    jclass equipSlot = nova_base::loadOrFind(env, loader, kEquipSlotDot, kEquipSlotSig);
    jclass component = nova_base::loadOrFind(env, loader, kComponentDot, kComponentSig);
    jclass closePacket = nova_base::loadOrFind(env, loader, kClosePacketDot, kClosePacketSig);

    jclass itemClasses[kItemClassCount] = {};
    for (int i = 0; i < kItemClassCount; ++i) {
        itemClasses[i] = nova_base::loadOrFind(env, loader, kItemClasses[i].dot,
                                               kItemClasses[i].sig);
    }
    jclass blockClasses[kBlockClassCount] = {};
    for (int i = 0; i < kBlockClassCount; ++i) {
        blockClasses[i] = nova_base::loadOrFind(env, loader, kBlockClasses[i].dot,
                                                kBlockClasses[i].sig);
    }
    if (loader)
        env->DeleteGlobalRef(loader);

    jmethodID getInstance =
        minecraft ? env->GetStaticMethodID(minecraft, kGetInstance, kGetInstanceDesc) : nullptr;
    jfieldID screenField = minecraft ? env->GetFieldID(minecraft, kScreenField, kScreenSig) : nullptr;
    jfieldID playerField =
        minecraft ? env->GetFieldID(minecraft, kPlayerField, kPlayerFieldSig) : nullptr;
    jfieldID gameModeField =
        minecraft ? env->GetFieldID(minecraft, kGameModeField, kGameModeSig) : nullptr;
    jmethodID absGetMenu =
        absScreen ? env->GetMethodID(absScreen, kAbsGetMenu, kAbsGetMenuDesc) : nullptr;
    jmethodID screenGetTitle =
        screen ? env->GetMethodID(screen, kScreenGetTitle, kScreenGetTitleDesc) : nullptr;
    jmethodID componentGetString =
        component ? env->GetMethodID(component, kComponentGetString, kComponentGetStringDesc)
                  : nullptr;
    jmethodID componentTranslatable =
        component ? env->GetStaticMethodID(component, kComponentTranslatable,
                                           kComponentTranslatableDesc)
                  : nullptr;
    jmethodID menuGetSlot = menu ? env->GetMethodID(menu, kMenuGetSlot, kMenuGetSlotDesc) : nullptr;
    jmethodID slotGetItem = slot ? env->GetMethodID(slot, kSlotGetItem, kSlotGetItemDesc) : nullptr;
    jmethodID chestRowCount =
        chestMenu ? env->GetMethodID(chestMenu, kChestRowCount, kChestRowCountDesc) : nullptr;
    jmethodID listGet = list ? env->GetMethodID(list, kListGet, kListGetDesc) : nullptr;
    jmethodID listSize = list ? env->GetMethodID(list, kListSize, kListSizeDesc) : nullptr;
    jmethodID playerClose = player ? env->GetMethodID(player, kPlayerClose, kPlayerCloseDesc) : nullptr;
    jmethodID playerGetInventory =
        player ? env->GetMethodID(player, kPlayerGetInventory, kPlayerGetInventoryDesc) : nullptr;
    jmethodID handleClick =
        gameMode ? env->GetMethodID(gameMode, kHandleClick, kHandleClickDesc) : nullptr;
    jmethodID stackIsEmpty = stack ? env->GetMethodID(stack, kStackIsEmpty, kStackIsEmptyDesc) : nullptr;
    jmethodID stackGetItem = stack ? env->GetMethodID(stack, kStackGetItem, kStackGetItemDesc) : nullptr;
    jmethodID stackGetCount = stack ? env->GetMethodID(stack, kStackGetCount, kStackGetCountDesc) : nullptr;
    jmethodID stackGetName = stack ? env->GetMethodID(stack, kStackGetName, kStackGetNameDesc) : nullptr;
    jmethodID stackDamageValue =
        stack ? env->GetMethodID(stack, kStackDamageValue, kIntGetterDesc) : nullptr;
    jmethodID stackMaxDamage =
        stack ? env->GetMethodID(stack, kStackMaxDamage, kIntGetterDesc) : nullptr;
    jmethodID stackMaxStack =
        stack ? env->GetMethodID(stack, kStackMaxStack, kIntGetterDesc) : nullptr;
    jmethodID stackDestroySpeed =
        stack ? env->GetMethodID(stack, kStackDestroySpeed, kStackDestroySpeedDesc) : nullptr;
    jmethodID blockItemGetBlock =
        itemClasses[0] ? env->GetMethodID(itemClasses[0], kBlockItemGetBlock,
                                          kBlockItemGetBlockDesc)
                       : nullptr;
    jmethodID blockDefaultState =
        block ? env->GetMethodID(block, kBlockDefaultState, kBlockDefaultStateDesc) : nullptr;
    jmethodID swordGetDamage = nullptr;
    for (int i = 0; i < kItemClassCount; ++i) {
        if (kItemClasses[i].bit == kItemSword && itemClasses[i])
            swordGetDamage = env->GetMethodID(itemClasses[i], kSwordGetDamage, kFloatGetterDesc);
    }
    jmethodID armorGetMaterial = nullptr;
    jmethodID armorGetSlot = nullptr;
    for (int i = 0; i < kItemClassCount; ++i) {
        if (kItemClasses[i].bit == kItemArmor && itemClasses[i]) {
            armorGetMaterial =
                env->GetMethodID(itemClasses[i], kArmorGetMaterial, kArmorGetMaterialDesc);
            armorGetSlot = env->GetMethodID(itemClasses[i], kArmorGetSlot, kArmorGetSlotDesc);
        }
    }
    jmethodID helperGetLevel =
        helper ? env->GetStaticMethodID(helper, kHelperGetLevel, kHelperGetLevelDesc) : nullptr;
    jmethodID enchantDamageBonus =
        enchantment ? env->GetMethodID(enchantment, kEnchantDamageBonus,
                                       kEnchantDamageBonusDesc)
                    : nullptr;
    jmethodID closeCtor =
        closePacket ? env->GetMethodID(closePacket, kMappedConstructor, kCloseCtorDesc) : nullptr;

    jfieldID menuContainerId = menu ? env->GetFieldID(menu, kMenuContainerId, kMappedPrimitiveIntSig) : nullptr;
    jfieldID invItems = inventory ? env->GetFieldID(inventory, kInvItems, kListSig) : nullptr;
    jfieldID invArmor = inventory ? env->GetFieldID(inventory, kInvArmor, kListSig) : nullptr;
    jfieldID invOffhand = inventory ? env->GetFieldID(inventory, kInvOffhand, kListSig) : nullptr;
    jfieldID playerMenu = player ? env->GetFieldID(player, kPlayerMenuField, kPlayerMenuSig) : nullptr;

    const char* clickNames[4] = {kMappedPICKUP, kMappedQUICKMOVE, kMappedSWAP, kMappedTHROW};
    jobject clickTypes[4] = {nullptr, nullptr, nullptr, nullptr};
    if (clickType) {
        for (int i = 0; i < 4; ++i) {
            jfieldID id = env->GetStaticFieldID(clickType, clickNames[i], kClickTypeSig);
            clickTypes[i] = id ? env->GetStaticObjectField(clickType, id) : nullptr;
        }
    }

    jobject itemIdentities[kItemIdentityCount] = {};
    if (items) {
        for (int i = 0; i < kItemIdentityCount; ++i) {
            jfieldID id = env->GetStaticFieldID(items, kItemIdentities[i].field, kItemSig);
            itemIdentities[i] = id ? env->GetStaticObjectField(items, id) : nullptr;
        }
    }
    jobject blockIdentities[kBlockIdentityCount] = {};
    if (blocks) {
        for (int i = 0; i < kBlockIdentityCount; ++i) {
            jfieldID id = env->GetStaticFieldID(blocks, kBlockIdentities[i].field, kBlockSig);
            blockIdentities[i] = id ? env->GetStaticObjectField(blocks, id) : nullptr;
        }
    }
    jobject enchantments[kEnchantmentCount] = {};
    if (enchants) {
        for (int i = 0; i < kEnchantmentCount; ++i) {
            jfieldID id = env->GetStaticFieldID(enchants, kEnchantments[i].field, kEnchSig);
            enchantments[i] = id ? env->GetStaticObjectField(enchants, id) : nullptr;
        }
    }
    jobject materialObjects[kMaterialCount] = {};
    if (materials) {
        for (int i = 0; i < kMaterialCount; ++i) {
            jfieldID id = env->GetStaticFieldID(materials, kMaterials[i].field, kMaterialsSig);
            materialObjects[i] = id ? env->GetStaticObjectField(materials, id) : nullptr;
        }
    }
    jobject equipSlotObjects[4] = {nullptr, nullptr, nullptr, nullptr};
    if (equipSlot) {
        for (int i = 0; i < 4; ++i) {
            jfieldID id = env->GetStaticFieldID(equipSlot, kArmorSlotFields[i], kEquipSlotSig);
            equipSlotObjects[i] = id ? env->GetStaticObjectField(equipSlot, id) : nullptr;
        }
    }
    jobject mobTypeUndefined = nullptr;
    if (mobType) {
        jfieldID id = env->GetStaticFieldID(mobType, kMobTypeUndefined, kMobTypeSig);
        mobTypeUndefined = id ? env->GetStaticObjectField(mobType, id) : nullptr;
    }

    jobject blockBlacklist[kBlockBlacklistCount] = {};
    if (blocks) {
        for (int i = 0; i < kBlockBlacklistCount; ++i) {
            jfieldID id = env->GetStaticFieldID(blocks, kBlockBlacklist[i], kBlockSig);
            blockBlacklist[i] = id ? env->GetStaticObjectField(blocks, id) : nullptr;
        }
    }

    // The three states the tool scores are measured against, built once.
    jobject stoneState = nullptr;
    jobject oakLogState = nullptr;
    jobject dirtState = nullptr;
    if (blocks && blockDefaultState) {
        jfieldID stoneId = env->GetStaticFieldID(blocks, kMappedStone, kBlockSig);
        jfieldID logId = env->GetStaticFieldID(blocks, kMappedOakLog, kBlockSig);
        jfieldID dirtId = env->GetStaticFieldID(blocks, kMappedDirt, kBlockSig);
        jobject stone = stoneId ? env->GetStaticObjectField(blocks, stoneId) : nullptr;
        jobject logBlock = logId ? env->GetStaticObjectField(blocks, logId) : nullptr;
        jobject dirt = dirtId ? env->GetStaticObjectField(blocks, dirtId) : nullptr;
        if (stone) stoneState = env->CallObjectMethod(stone, blockDefaultState);
        if (logBlock) oakLogState = env->CallObjectMethod(logBlock, blockDefaultState);
        if (dirt) dirtState = env->CallObjectMethod(dirt, blockDefaultState);
        if (stone) env->DeleteLocalRef(stone);
        if (logBlock) env->DeleteLocalRef(logBlock);
        if (dirt) env->DeleteLocalRef(dirt);
    }

    if (env->ExceptionCheck())
        env->ExceptionClear();

    // Everything a later NewObject, IsInstanceOf or Call is given has to be here,
    // because a null class or method is a hard crash inside the VM rather than a
    // failed call.
    bool ok = getInstance && screenField && playerField && gameModeField && absGetMenu &&
              screenGetTitle && componentGetString && componentTranslatable && menuGetSlot &&
              slotGetItem && chestRowCount && listGet && listSize && playerClose &&
              playerGetInventory && handleClick &&
              stackIsEmpty && stackGetItem && stackGetCount && stackGetName && stackDamageValue &&
              stackMaxDamage && stackMaxStack && stackDestroySpeed && blockItemGetBlock &&
              blockDefaultState && swordGetDamage && armorGetMaterial && armorGetSlot &&
              helperGetLevel && enchantDamageBonus && closeCtor && menuContainerId && invItems &&
              invArmor && invOffhand && playerMenu && clickTypes[0] && clickTypes[1] &&
              clickTypes[2] && clickTypes[3] && itemIdentities[0] && blockIdentities[0] &&
              enchantments[0] && materialObjects[0] && equipSlotObjects[0] && mobTypeUndefined &&
              stoneState && oakLogState && dirtState && itemClasses[0] &&
              blockBlacklist[0];
    for (int i = 0; i < kItemClassCount && ok; ++i) {
        if (itemClasses[i] == nullptr)
            ok = false;
    }
    for (int i = 0; i < kBlockClassCount && ok; ++i) {
        if (blockClasses[i] == nullptr)
            ok = false;
    }

    if (!ok) {
        if (!g_invLogged) {
            g_invLogged = true;
            LogTo("Base: inventory layer incomplete (menu=%d slot=%d click=%d list=%d "
                  "clickTypes=%d items=%d blocks=%d ench=%d materials=%d equip=%d states=%d)",
                  absGetMenu != nullptr, menuGetSlot != nullptr, handleClick != nullptr,
                  listGet != nullptr, clickTypes[0] != nullptr, itemIdentities[0] != nullptr,
                  blockIdentities[0] != nullptr, enchantments[0] != nullptr,
                  materialObjects[0] != nullptr, equipSlotObjects[0] != nullptr,
                  stoneState != nullptr);
        }
    } else {
        g_inv.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));
        g_inv.screen = static_cast<jclass>(env->NewGlobalRef(screen));
        g_inv.containerScreen = static_cast<jclass>(env->NewGlobalRef(containerScreen));
        g_inv.inventoryScreen = static_cast<jclass>(env->NewGlobalRef(inventoryScreen));
        g_inv.absScreen = static_cast<jclass>(env->NewGlobalRef(absScreen));
        g_inv.menu = static_cast<jclass>(env->NewGlobalRef(menu));
        g_inv.chestMenu = static_cast<jclass>(env->NewGlobalRef(chestMenu));
        g_inv.slot = static_cast<jclass>(env->NewGlobalRef(slot));
        g_inv.clickType = static_cast<jclass>(env->NewGlobalRef(clickType));
        g_inv.gameMode = static_cast<jclass>(env->NewGlobalRef(gameMode));
        g_inv.player = static_cast<jclass>(env->NewGlobalRef(player));
        g_inv.inventory = static_cast<jclass>(env->NewGlobalRef(inventory));
        g_inv.list = static_cast<jclass>(env->NewGlobalRef(list));
        g_inv.stack = static_cast<jclass>(env->NewGlobalRef(stack));
        g_inv.item = static_cast<jclass>(env->NewGlobalRef(item));
        g_inv.block = static_cast<jclass>(env->NewGlobalRef(block));
        g_inv.state = static_cast<jclass>(env->NewGlobalRef(state));
        g_inv.items = static_cast<jclass>(env->NewGlobalRef(items));
        g_inv.blocks = static_cast<jclass>(env->NewGlobalRef(blocks));
        g_inv.enchants = static_cast<jclass>(env->NewGlobalRef(enchants));
        g_inv.enchantment = static_cast<jclass>(env->NewGlobalRef(enchantment));
        g_inv.helper = static_cast<jclass>(env->NewGlobalRef(helper));
        g_inv.mobType = static_cast<jclass>(env->NewGlobalRef(mobType));
        g_inv.materials = static_cast<jclass>(env->NewGlobalRef(materials));
        g_inv.equipSlot = static_cast<jclass>(env->NewGlobalRef(equipSlot));
        g_inv.component = static_cast<jclass>(env->NewGlobalRef(component));
        g_inv.closePacket = static_cast<jclass>(env->NewGlobalRef(closePacket));
        for (int i = 0; i < kItemClassCount; ++i)
            g_inv.itemClasses[i] = static_cast<jclass>(env->NewGlobalRef(itemClasses[i]));
        for (int i = 0; i < kBlockClassCount; ++i)
            g_inv.blockClasses[i] = static_cast<jclass>(env->NewGlobalRef(blockClasses[i]));

        g_inv.getInstance = getInstance;
        g_inv.absGetMenu = absGetMenu;
        g_inv.screenGetTitle = screenGetTitle;
        g_inv.componentGetString = componentGetString;
        g_inv.componentTranslatable = componentTranslatable;
        g_inv.menuGetSlot = menuGetSlot;
        g_inv.slotGetItem = slotGetItem;
        g_inv.chestRowCount = chestRowCount;
        g_inv.listGet = listGet;
        g_inv.listSize = listSize;
        g_inv.playerClose = playerClose;
        g_inv.playerGetInventory = playerGetInventory;
        g_inv.handleClick = handleClick;
        g_inv.stackIsEmpty = stackIsEmpty;
        g_inv.stackGetItem = stackGetItem;
        g_inv.stackGetCount = stackGetCount;
        g_inv.stackGetName = stackGetName;
        g_inv.stackDamageValue = stackDamageValue;
        g_inv.stackMaxDamage = stackMaxDamage;
        g_inv.stackMaxStack = stackMaxStack;
        g_inv.stackDestroySpeed = stackDestroySpeed;
        g_inv.blockItemGetBlock = blockItemGetBlock;
        g_inv.blockDefaultState = blockDefaultState;
        g_inv.swordGetDamage = swordGetDamage;
        g_inv.armorGetMaterial = armorGetMaterial;
        g_inv.armorGetSlot = armorGetSlot;
        g_inv.helperGetLevel = helperGetLevel;
        g_inv.enchantDamageBonus = enchantDamageBonus;
        g_inv.closeCtor = closeCtor;

        g_inv.screenField = screenField;
        g_inv.playerField = playerField;
        g_inv.gameModeField = gameModeField;
        g_inv.menuContainerId = menuContainerId;
        g_inv.invItems = invItems;
        g_inv.invArmor = invArmor;
        g_inv.invOffhand = invOffhand;
        g_inv.playerMenu = playerMenu;

        for (int i = 0; i < 4; ++i)
            g_inv.clickTypes[i] = env->NewGlobalRef(clickTypes[i]);
        for (int i = 0; i < kItemIdentityCount; ++i)
            g_inv.itemIdentities[i] = env->NewGlobalRef(itemIdentities[i]);
        for (int i = 0; i < kBlockIdentityCount; ++i)
            g_inv.blockIdentities[i] = env->NewGlobalRef(blockIdentities[i]);
        for (int i = 0; i < kEnchantmentCount; ++i)
            g_inv.enchantments[i] = env->NewGlobalRef(enchantments[i]);
        for (int i = 0; i < kMaterialCount; ++i)
            g_inv.materialObjects[i] = env->NewGlobalRef(materialObjects[i]);
        for (int i = 0; i < 4; ++i)
            g_inv.equipSlots[i] = env->NewGlobalRef(equipSlotObjects[i]);
        g_inv.mobTypeUndefined = env->NewGlobalRef(mobTypeUndefined);
        g_inv.stoneState = env->NewGlobalRef(stoneState);
        g_inv.oakLogState = env->NewGlobalRef(oakLogState);
        g_inv.dirtState = env->NewGlobalRef(dirtState);
        for (int i = 0; i < kBlockBlacklistCount; ++i)
            g_inv.blockBlacklist[i] = env->NewGlobalRef(blockBlacklist[i]);
        g_inv.ok = true;
    }

    auto drop = [&](jobject ref) { if (ref) env->DeleteLocalRef(ref); };
    drop(minecraft); drop(screen); drop(containerScreen); drop(inventoryScreen); drop(absScreen);
    drop(menu); drop(chestMenu); drop(slot); drop(clickType); drop(gameMode); drop(player);
    drop(inventory); drop(list); drop(stack); drop(item); drop(block); drop(state); drop(items);
    drop(blocks); drop(enchants); drop(enchantment); drop(helper); drop(mobType); drop(materials);
    drop(equipSlot); drop(component); drop(closePacket);
    drop(stoneState); drop(oakLogState); drop(dirtState);
    drop(mobTypeUndefined);
    for (int i = 0; i < kItemClassCount; ++i)
        drop(itemClasses[i]);
    for (int i = 0; i < kBlockClassCount; ++i)
        drop(blockClasses[i]);
    for (int i = 0; i < 4; ++i) { drop(clickTypes[i]); drop(equipSlotObjects[i]); }
    for (int i = 0; i < kItemIdentityCount; ++i)
        drop(itemIdentities[i]);
    for (int i = 0; i < kBlockIdentityCount; ++i)
        drop(blockIdentities[i]);
    for (int i = 0; i < kEnchantmentCount; ++i)
        drop(enchantments[i]);
    for (int i = 0; i < kMaterialCount; ++i)
        drop(materialObjects[i]);
    for (int i = 0; i < kBlockBlacklistCount; ++i)
        drop(blockBlacklist[i]);

    return g_inv.ok;
}

bool Ready(JNIEnv* env) {
    if (!env)
        return false;
    return ResolveInventory(env);
}

jobject Client(JNIEnv* env) {
    jobject instance = env->CallStaticObjectMethod(g_inv.minecraft, g_inv.getInstance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return instance;
}

jobject CurrentPlayer(JNIEnv* env) {
    jobject client = Client(env);
    if (client == nullptr)
        return nullptr;
    jobject player = env->GetObjectField(client, g_inv.playerField);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return player;
}

// A string off a Component, used for titles and translated labels.
bool ComponentText(JNIEnv* env, jobject component, std::string& out) {
    if (component == nullptr || g_inv.componentGetString == nullptr)
        return false;
    jstring text = static_cast<jstring>(env->CallObjectMethod(component, g_inv.componentGetString));
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (text == nullptr)
        return false;

    const char* utf = env->GetStringUTFChars(text, nullptr);
    if (utf == nullptr) {
        env->DeleteLocalRef(text);
        return false;
    }
    out.assign(utf);
    env->ReleaseStringUTFChars(text, utf);
    env->DeleteLocalRef(text);
    return true;
}

// One stack's worth of everything. The caller has already checked the layer is up.
void ReadFacts(JNIEnv* env, jobject stack, ItemFacts& out) {
    out = ItemFacts{};

    if (env->CallBooleanMethod(stack, g_inv.stackIsEmpty) == JNI_TRUE) {
        if (env->ExceptionCheck())
            env->ExceptionClear();
        out.empty = true;
        return;
    }
    out.empty = false;

    out.count = static_cast<int>(env->CallIntMethod(stack, g_inv.stackGetCount));
    out.damageValue = static_cast<int>(env->CallIntMethod(stack, g_inv.stackDamageValue));
    out.maxDamage = static_cast<int>(env->CallIntMethod(stack, g_inv.stackMaxDamage));
    out.maxStack = static_cast<int>(env->CallIntMethod(stack, g_inv.stackMaxStack));
    if (env->ExceptionCheck())
        env->ExceptionClear();

    jobject item = env->CallObjectMethod(stack, g_inv.stackGetItem);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        item = nullptr;
    }
    if (item == nullptr)
        return;

    for (int i = 0; i < kItemClassCount; ++i) {
        if (g_inv.itemClasses[i] && env->IsInstanceOf(item, g_inv.itemClasses[i]) == JNI_TRUE)
            out.kinds |= kItemClasses[i].bit;
    }
    if (env->ExceptionCheck())
        env->ExceptionClear();

    jobject block = nullptr;
    if ((out.kinds & kItemBlock) != 0 && g_inv.blockItemGetBlock) {
        block = env->CallObjectMethod(item, g_inv.blockItemGetBlock);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            block = nullptr;
        }
    }

    for (int i = 0; i < kItemIdentityCount; ++i) {
        if (g_inv.itemIdentities[i] && env->IsSameObject(item, g_inv.itemIdentities[i])) {
            out.item = kItemIdentities[i].value;
            break;
        }
    }
    if ((out.kinds & kItemElytra) != 0) out.item = KnownItem::Elytra;
    if ((out.kinds & kItemFirework) != 0) out.item = KnownItem::FireworkRocket;
    if (out.item == KnownItem::Nothing && block != nullptr) {
        for (int i = 0; i < kBlockIdentityCount; ++i) {
            if (g_inv.blockIdentities[i] && env->IsSameObject(block, g_inv.blockIdentities[i])) {
                out.item = kBlockIdentities[i].value;
                break;
            }
        }
    }

    if (env->ExceptionCheck())
        env->ExceptionClear();

    // Enchantments are only ever consulted for the things that can carry them, so
    // the eleven lookups below are skipped for everything else -- most of an
    // inventory is blocks and food, and asking about their sharpness would be
    // eleven crossings for nothing.
    const bool wantsEnchantments =
        (out.kinds & (kItemSword | kItemAxe | kItemPickaxe | kItemShovel | kItemBow |
                      kItemCrossbow | kItemArmor)) != 0 ||
        out.item == KnownItem::Stick || out.item == KnownItem::SlimeBall;

    if (wantsEnchantments) {
        for (int i = 0; i < kEnchantmentCount; ++i) {
            if (g_inv.enchantments[i] == nullptr)
                continue;
            const jint level = env->CallStaticIntMethod(g_inv.helper, g_inv.helperGetLevel,
                                                       g_inv.enchantments[i], stack);
            out.*(kEnchantments[i].slot) = static_cast<int>(level);
        }
        if (env->ExceptionCheck())
            env->ExceptionClear();
    }

    if (out.sharpness > 0 && g_inv.enchantments[0] && g_inv.mobTypeUndefined) {
        out.sharpnessBonus = static_cast<float>(env->CallFloatMethod(
            g_inv.enchantments[0], g_inv.enchantDamageBonus, static_cast<jint>(out.sharpness),
            g_inv.mobTypeUndefined));
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            out.sharpnessBonus = 0.0f;
        }
    }

    if ((out.kinds & kItemSword) != 0 && g_inv.swordGetDamage) {
        out.swordDamage = static_cast<float>(env->CallFloatMethod(item, g_inv.swordGetDamage));
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            out.swordDamage = 0.0f;
        }
    }

    // The three tools are measured against the block each is meant for.
    if ((out.kinds & (kItemPickaxe | kItemAxe | kItemShovel)) != 0 && g_inv.stackDestroySpeed) {
        jobject target = nullptr;
        if ((out.kinds & kItemPickaxe) != 0)
            target = g_inv.stoneState;
        else if ((out.kinds & kItemAxe) != 0)
            target = g_inv.oakLogState;
        else
            target = g_inv.dirtState;
        if (target) {
            out.destroySpeed = static_cast<float>(
                env->CallFloatMethod(stack, g_inv.stackDestroySpeed, target));
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                out.destroySpeed = 0.0f;
            }
        }
    }

    if ((out.kinds & kItemArmor) != 0) {
        if (g_inv.armorGetMaterial) {
            jobject material = env->CallObjectMethod(item, g_inv.armorGetMaterial);
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                material = nullptr;
            }
            if (material) {
                for (int i = 0; i < kMaterialCount; ++i) {
                    if (g_inv.materialObjects[i] &&
                        env->IsSameObject(material, g_inv.materialObjects[i])) {
                        out.material = kMaterials[i].value;
                        break;
                    }
                }
                env->DeleteLocalRef(material);
            }
        }
        if (g_inv.armorGetSlot) {
            jobject where = env->CallObjectMethod(item, g_inv.armorGetSlot);
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                where = nullptr;
            }
            if (where) {
                for (int i = 0; i < 4; ++i) {
                    if (g_inv.equipSlots[i] && env->IsSameObject(where, g_inv.equipSlots[i])) {
                        out.armorSlot = i;
                        break;
                    }
                }
                env->DeleteLocalRef(where);
            }
        }
    }

    // A BlockItem whose block is a skull is a golden head on the servers this is
    // written for, which the scoring treats as a special case. The class is only
    // wanted here, so it resolves on first sight rather than up front.
    if (block != nullptr) {
        if (g_skullBlock == nullptr) {
            jobject loader = GetMinecraftClassLoader(env);
            jclass skull = nova_base::loadOrFind(env, loader,
                                                 kMappedSkullBlockDot,
                                                 kMappedSkullBlockSig);
            if (loader)
                env->DeleteGlobalRef(loader);
            if (skull) {
                g_skullBlock = static_cast<jclass>(env->NewGlobalRef(skull));
                env->DeleteLocalRef(skull);
            }
        }
        if (g_skullBlock &&
            env->IsInstanceOf(block, g_skullBlock) == JNI_TRUE) {
            out.kinds |= kItemSkullBlock;
        }
        for (int i = 0; i < kBlockBlacklistCount; ++i) {
            if (g_inv.blockBlacklist[i] && env->IsSameObject(block, g_inv.blockBlacklist[i])) {
                out.blacklistedBlock = true;
                break;
            }
        }
        for (int i = 0; i < kBlockClassCount; ++i) {
            if (g_inv.blockClasses[i] &&
                env->IsInstanceOf(block, g_inv.blockClasses[i]) == JNI_TRUE) {
                out.blockKinds |= kBlockClasses[i].bit;
            }
        }
        if (env->ExceptionCheck())
            env->ExceptionClear();
        env->DeleteLocalRef(block);
    }

    if (env->ExceptionCheck())
        env->ExceptionClear();
    env->DeleteLocalRef(item);
}

}  // namespace

bool ReadItemFacts(jobject stack, ItemFacts& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::ReadItemFacts(stack, out);
    if (compat::IsLegacyLunar()) return compat::lunar::ReadItemFacts(stack, out);
    out = ItemFacts{};
    if (stack == nullptr)
        return false;

    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;

    ReadFacts(attach.env, stack, out);
    return true;
}

bool ItemDisplayName(jobject stack, std::string& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::ItemDisplayName(stack, out);
    if (compat::IsLegacyLunar()) return compat::lunar::ItemDisplayName(stack, out);
    out.clear();
    if (stack == nullptr)
        return false;

    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject component = env->CallObjectMethod(stack, g_inv.stackGetName);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (component == nullptr)
        return false;

    const bool ok = ComponentText(env, component, out);
    env->DeleteLocalRef(component);
    return ok;
}

bool InventorySlot(int index, jobject& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::InventoryStack(index, false, out);
    if (compat::IsLegacyLunar()) return compat::lunar::InventoryStack(index, false, out);
    out = nullptr;
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject player = CurrentPlayer(env);
    if (player == nullptr)
        return false;
    jobject inventory = env->CallObjectMethod(player, g_inv.playerGetInventory);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (inventory == nullptr)
        return false;

    jobject list = env->GetObjectField(inventory, g_inv.invItems);
    env->DeleteLocalRef(inventory);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (list == nullptr)
        return false;

    out = env->CallObjectMethod(list, g_inv.listGet, static_cast<jint>(index));
    env->DeleteLocalRef(list);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        out = nullptr;
        return false;
    }
    return out != nullptr;
}

bool InventoryArmor(int index, jobject& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::InventoryStack(index, true, out);
    if (compat::IsLegacyLunar()) return compat::lunar::InventoryStack(index, true, out);
    out = nullptr;
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject player = CurrentPlayer(env);
    if (player == nullptr)
        return false;
    jobject inventory = env->CallObjectMethod(player, g_inv.playerGetInventory);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (inventory == nullptr)
        return false;

    jobject list = env->GetObjectField(inventory, g_inv.invArmor);
    env->DeleteLocalRef(inventory);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (list == nullptr)
        return false;

    out = env->CallObjectMethod(list, g_inv.listGet, static_cast<jint>(index));
    env->DeleteLocalRef(list);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        out = nullptr;
        return false;
    }
    return out != nullptr;
}

bool InventoryOffhand(int index, jobject& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::InventoryOffhand(index, out);
    if (compat::IsLegacyLunar()) { out = nullptr; return false; }
    out = nullptr;
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject player = CurrentPlayer(env);
    if (player == nullptr)
        return false;
    jobject inventory = env->CallObjectMethod(player, g_inv.playerGetInventory);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (inventory == nullptr)
        return false;

    jobject list = env->GetObjectField(inventory, g_inv.invOffhand);
    env->DeleteLocalRef(inventory);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (list == nullptr)
        return false;

    out = env->CallObjectMethod(list, g_inv.listGet, static_cast<jint>(index));
    env->DeleteLocalRef(list);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        out = nullptr;
        return false;
    }
    return out != nullptr;
}

namespace {

// Both counts and the three element readers walk Inventory -> list -> element, so
// the walk lives here and the callers only differ in which field they start from.
jobject InventoryList(JNIEnv* env, jfieldID field) {
    jobject player = CurrentPlayer(env);
    if (player == nullptr)
        return nullptr;
    jobject inventory = env->CallObjectMethod(player, g_inv.playerGetInventory);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    if (inventory == nullptr)
        return nullptr;
    jobject list = env->GetObjectField(inventory, field);
    env->DeleteLocalRef(inventory);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return list;
}

int InventoryListSize(JNIEnv* env, jfieldID field) {
    jobject list = InventoryList(env, field);
    if (list == nullptr)
        return 0;
    const jint size = env->CallIntMethod(list, g_inv.listSize);
    env->DeleteLocalRef(list);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return 0;
    }
    return static_cast<int>(size);
}

}  // namespace

int InventoryItemCount() {
    if (compat::UsesModernRuntime()) return compat::mc263::InventoryCount(false);
    if (compat::IsLegacyLunar()) return compat::lunar::InventoryCount(false);
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return 0;
    return InventoryListSize(attach.env, g_inv.invItems);
}

int InventoryArmorCount() {
    if (compat::UsesModernRuntime()) return compat::mc263::InventoryCount(true);
    if (compat::IsLegacyLunar()) return compat::lunar::InventoryCount(true);
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return 0;
    return InventoryListSize(attach.env, g_inv.invArmor);
}

bool InventoryHasOffhand() { return !compat::IsLegacyLunar(); }

bool InventoryHasItem(KnownItem item) {
    if (compat::UsesModernRuntime()) return compat::mc263::InventoryHasItem(item);
    if (compat::IsLegacyLunar()) return compat::lunar::InventoryHasItem(item);
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject target = nullptr;
    for (int i = 0; i < kItemIdentityCount; ++i) {
        if (kItemIdentities[i].value == item) {
            target = g_inv.itemIdentities[i];
            break;
        }
    }
    if (target == nullptr)
        return false;

    jobject player = CurrentPlayer(env);
    if (player == nullptr)
        return false;
    jobject inventory = env->CallObjectMethod(player, g_inv.playerGetInventory);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (inventory == nullptr)
        return false;

    // Two lists rather than one: the original's getAllItems is items + armor, and
    // the search stops at the first hit either way.
    const jfieldID lists[2] = {g_inv.invItems, g_inv.invArmor};
    bool found = false;
    for (const jfieldID field : lists) {
        jobject list = InventoryList(env, field);
        if (list == nullptr)
            continue;

        const jint size = env->CallIntMethod(list, g_inv.listSize);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            env->DeleteLocalRef(list);
            continue;
        }

        for (jint i = 0; i < size && !found; ++i) {
            jobject stack = env->CallObjectMethod(list, g_inv.listGet, i);
            if (env->ExceptionCheck() || stack == nullptr) {
                env->ExceptionClear();
                if (stack) env->DeleteLocalRef(stack);
                continue;
            }

            const jboolean empty = env->CallBooleanMethod(stack, g_inv.stackIsEmpty);
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                env->DeleteLocalRef(stack);
                continue;
            }
            if (empty == JNI_FALSE) {
                jobject held = env->CallObjectMethod(stack, g_inv.stackGetItem);
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                    held = nullptr;
                }
                if (held != nullptr) {
                    if (env->IsSameObject(held, target))
                        found = true;
                    env->DeleteLocalRef(held);
                }
            }
            env->DeleteLocalRef(stack);
            if (env->ExceptionCheck())
                env->ExceptionClear();
        }

        env->DeleteLocalRef(list);
        if (found)
            break;
    }

    env->DeleteLocalRef(inventory);
    return found;
}

jobject CurrentScreen() {
    if (compat::UsesModernRuntime()) return compat::mc263::CurrentScreen();
    if (compat::IsLegacyLunar()) return compat::lunar::CurrentScreen();
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return nullptr;
    JNIEnv* env = attach.env;

    jobject client = Client(env);
    if (client == nullptr)
        return nullptr;
    jobject screen = env->GetObjectField(client, g_inv.screenField);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return screen;
}

bool ScreenIsContainerScreen(jobject screen) {
    if (compat::UsesModernRuntime()) return compat::mc263::ScreenIsContainerScreen(screen);
    if (compat::IsLegacyLunar()) return compat::lunar::ScreenIsContainerScreen(screen);
    if (screen == nullptr)
        return false;
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    const jboolean match = attach.env->IsInstanceOf(screen, g_inv.containerScreen);
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    return match == JNI_TRUE;
}

bool ScreenIsInventoryScreen(jobject screen) {
    if (compat::UsesModernRuntime()) return compat::mc263::ScreenIsInventoryScreen(screen);
    if (compat::IsLegacyLunar()) return compat::lunar::ScreenIsInventoryScreen(screen);
    if (screen == nullptr)
        return false;
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    const jboolean match = attach.env->IsInstanceOf(screen, g_inv.inventoryScreen);
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    return match == JNI_TRUE;
}

bool ScreenMenu(jobject screen, jobject& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::ScreenMenu(screen, out);
    if (compat::IsLegacyLunar()) return compat::lunar::ScreenMenu(screen, out);
    out = nullptr;
    if (screen == nullptr)
        return false;
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    JNIEnv* env = attach.env;

    if (env->IsInstanceOf(screen, g_inv.absScreen) != JNI_TRUE) {
        if (env->ExceptionCheck())
            env->ExceptionClear();
        return false;
    }
    out = env->CallObjectMethod(screen, g_inv.absGetMenu);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        out = nullptr;
        return false;
    }
    return out != nullptr;
}

bool MenuContainerId(jobject menu, int& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::MenuContainerId(menu, out);
    if (compat::IsLegacyLunar()) return compat::lunar::MenuContainerId(menu, out);
    out = -1;
    if (menu == nullptr)
        return false;
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    out = static_cast<int>(attach.env->GetIntField(menu, g_inv.menuContainerId));
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        out = -1;
        return false;
    }
    return true;
}

bool MenuIsPlayerInventory(jobject menu) {
    if (compat::UsesModernRuntime()) return compat::mc263::MenuIsPlayerInventory(menu);
    if (compat::IsLegacyLunar()) return compat::lunar::MenuIsPlayerInventory(menu);
    if (menu == nullptr)
        return false;
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject player = CurrentPlayer(env);
    if (player == nullptr)
        return false;
    jobject playerMenu = env->GetObjectField(player, g_inv.playerMenu);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (playerMenu == nullptr)
        return false;
    const jboolean same = env->IsSameObject(menu, playerMenu);
    env->DeleteLocalRef(playerMenu);
    return same == JNI_TRUE;
}

bool MenuIsChest(jobject menu) {
    if (compat::UsesModernRuntime()) return compat::mc263::MenuIsChest(menu);
    if (compat::IsLegacyLunar()) return compat::lunar::MenuIsChest(menu);
    if (menu == nullptr)
        return false;
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    const jboolean match = attach.env->IsInstanceOf(menu, g_inv.chestMenu);
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    return match == JNI_TRUE;
}

bool MenuRowCount(jobject menu, int& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::MenuRowCount(menu, out);
    if (compat::IsLegacyLunar()) return compat::lunar::MenuRowCount(menu, out);
    out = 0;
    if (menu == nullptr)
        return false;
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    JNIEnv* env = attach.env;

    if (env->IsInstanceOf(menu, g_inv.chestMenu) != JNI_TRUE) {
        if (env->ExceptionCheck())
            env->ExceptionClear();
        return false;
    }
    out = static_cast<int>(env->CallIntMethod(menu, g_inv.chestRowCount));
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        out = 0;
        return false;
    }
    return true;
}

bool MenuSlotStack(jobject menu, int slot, jobject& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::MenuSlotStack(menu, slot, out);
    if (compat::IsLegacyLunar()) return compat::lunar::MenuSlotStack(menu, slot, out);
    out = nullptr;
    if (menu == nullptr)
        return false;
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject slotObject = env->CallObjectMethod(menu, g_inv.menuGetSlot, static_cast<jint>(slot));
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (slotObject == nullptr)
        return false;

    out = env->CallObjectMethod(slotObject, g_inv.slotGetItem);
    env->DeleteLocalRef(slotObject);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        out = nullptr;
        return false;
    }
    return out != nullptr;
}

bool PlayerMenu(jobject& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::PlayerMenu(out);
    if (compat::IsLegacyLunar()) return compat::lunar::PlayerMenu(out);
    out = nullptr;
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject player = CurrentPlayer(env);
    if (player == nullptr)
        return false;
    out = env->GetObjectField(player, g_inv.playerMenu);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        out = nullptr;
        return false;
    }
    return out != nullptr;
}

bool PlayerMenuContainerId(int& out) {
    out = -1;
    jobject menu = nullptr;
    if (!PlayerMenu(menu))
        return false;
    const bool ok = MenuContainerId(menu, out);
    JniAttach attach;
    if (attach)
        attach.env->DeleteLocalRef(menu);
    return ok;
}

bool ScreenTitle(jobject screen, std::string& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::ScreenTitle(screen, out);
    if (compat::IsLegacyLunar()) return compat::lunar::ScreenTitle(screen, out);
    out.clear();
    if (screen == nullptr)
        return false;
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject title = env->CallObjectMethod(screen, g_inv.screenGetTitle);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (title == nullptr)
        return false;

    const bool ok = ComponentText(env, title, out);
    env->DeleteLocalRef(title);
    return ok;
}

bool TranslatedText(const char* key, std::string& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::TranslatedText(key, out);
    if (compat::IsLegacyLunar()) return compat::lunar::TranslatedText(key, out);
    out.clear();
    if (key == nullptr)
        return false;
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jstring name = env->NewStringUTF(key);
    if (name == nullptr)
        return false;
    jobject component =
        env->CallStaticObjectMethod(g_inv.component, g_inv.componentTranslatable, name);
    env->DeleteLocalRef(name);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (component == nullptr)
        return false;

    const bool ok = ComponentText(env, component, out);
    env->DeleteLocalRef(component);
    return ok;
}

bool ClickMenuSlot(int containerId, int slot, int button, ClickKind click) {
    if (compat::UsesModernRuntime()) return compat::mc263::ClickMenuSlot(containerId, slot, button, click);
    if (compat::IsLegacyLunar()) return compat::lunar::ClickMenuSlot(containerId, slot, button, click);
    const int index = static_cast<int>(click);
    if (index < 0 || index > 3)
        return false;

    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject client = Client(env);
    if (client == nullptr)
        return false;
    jobject gameMode = env->GetObjectField(client, g_inv.gameModeField);
    jobject player = env->GetObjectField(client, g_inv.playerField);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        if (gameMode) env->DeleteLocalRef(gameMode);
        if (player) env->DeleteLocalRef(player);
        return false;
    }
    if (gameMode == nullptr || player == nullptr) {
        if (gameMode) env->DeleteLocalRef(gameMode);
        if (player) env->DeleteLocalRef(player);
        return false;
    }

    env->CallVoidMethod(gameMode, g_inv.handleClick, static_cast<jint>(containerId),
                        static_cast<jint>(slot), static_cast<jint>(button), g_inv.clickTypes[index],
                        player);
    env->DeleteLocalRef(gameMode);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

bool CloseContainer() {
    if (compat::UsesModernRuntime()) return compat::mc263::CloseContainer();
    if (compat::IsLegacyLunar()) return compat::lunar::CloseContainer();
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject player = CurrentPlayer(env);
    if (player == nullptr)
        return false;
    env->CallVoidMethod(player, g_inv.playerClose);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

namespace {

// The packet classes the inventory module watches are resolved on first use and
// held. Resolving them per packet would be ruinous -- GetMinecraftClassLoader
// walks every thread in the JVM to find the game's loader -- and the modules ask
// about every outgoing packet.
struct PacketClassCache {
    const char* dot;
    const char* sig;
    jclass      ref;
};

PacketClassCache g_packetClasses[] = {
    {kMappedServerboundMovePlayerPacketDot,
     kMappedServerboundMovePlayerPacketSig, nullptr},
    {kMappedServerboundUseItemPacketDot,
     kMappedServerboundUseItemPacketSig, nullptr},
    {kMappedServerboundUseItemOnPacketDot,
     kMappedServerboundUseItemOnPacketSig, nullptr},
    {kMappedServerboundInteractPacketDot,
     kMappedServerboundInteractPacketSig, nullptr},
    {kMappedServerboundPlayerActionPacketDot,
     kMappedServerboundPlayerActionPacketSig, nullptr},
};
constexpr int kPacketClassCount =
    static_cast<int>(sizeof(g_packetClasses) / sizeof(g_packetClasses[0]));

jclass PacketClass(JNIEnv* env, int index) {
    if (index < 0 || index >= kPacketClassCount)
        return nullptr;
    PacketClassCache& entry = g_packetClasses[index];
    if (entry.ref)
        return entry.ref;

    jobject loader = GetMinecraftClassLoader(env);
    jclass type = nova_base::loadOrFind(env, loader, entry.dot, entry.sig);
    if (loader)
        env->DeleteGlobalRef(loader);
    if (type == nullptr)
        return nullptr;
    entry.ref = static_cast<jclass>(env->NewGlobalRef(type));
    env->DeleteLocalRef(type);
    return entry.ref;
}

bool PacketIsCached(JNIEnv* env, jobject packet, int index) {
    if (packet == nullptr)
        return false;
    jclass type = PacketClass(env, index);
    if (type == nullptr)
        return false;
    const jboolean match = env->IsInstanceOf(packet, type);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return match == JNI_TRUE;
}

}

bool PacketIsContainerClose(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::ContainerClose);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPacket(packet, compat::lunar::PacketKind::ContainerClose);
    if (packet == nullptr)
        return false;
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    if (g_inv.closePacket == nullptr)
        return false;
    const jboolean match = attach.env->IsInstanceOf(packet, g_inv.closePacket);
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    return match == JNI_TRUE;
}

bool PacketIsMovePlayer(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::MovePlayer);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPacket(packet, compat::lunar::PacketKind::MovePlayer);
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    return PacketIsCached(attach.env, packet, 0);
}

bool PacketIsUseItem(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::UseItemOnly);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPacket(packet, compat::lunar::PacketKind::UseItemOnly);
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    return PacketIsCached(attach.env, packet, 1);
}

bool PacketIsUseItemOn(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::UseItemOn);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPacket(packet, compat::lunar::PacketKind::UseItemOn);
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    return PacketIsCached(attach.env, packet, 2);
}

bool PacketIsInteract(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::Interact);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPacket(packet, compat::lunar::PacketKind::Interact);
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    return PacketIsCached(attach.env, packet, 3);
}

bool PacketIsPlayerAction(jobject packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPacket(packet, compat::mc263::PacketKind::PlayerAction);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPacket(packet, compat::lunar::PacketKind::PlayerAction);
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    return PacketIsCached(attach.env, packet, 4);
}

bool SendContainerClose(int containerId) {
    if (compat::UsesModernRuntime()) return compat::mc263::SendContainerClose(containerId);
    if (compat::IsLegacyLunar()) return compat::lunar::SendContainerClose(containerId);
    JniAttach attach;
    if (!attach || !Ready(attach.env))
        return false;
    JNIEnv* env = attach.env;

    if (g_inv.closePacket == nullptr || g_inv.closeCtor == nullptr)
        return false;
    jobject packet = env->NewObject(g_inv.closePacket, g_inv.closeCtor,
                                    static_cast<jint>(containerId));
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (packet == nullptr)
        return false;

    const bool sent = SendPacket(packet);
    env->DeleteLocalRef(packet);
    return sent;
}

}
