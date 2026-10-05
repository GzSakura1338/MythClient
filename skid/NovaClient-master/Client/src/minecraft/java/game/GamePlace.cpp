#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_slotspoof.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"

#include <atomic>
#include <string_view>

#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_network.h"
#include "minecraft/java/api/jni_lookup.h"

namespace nova::java {
namespace {

#define NOVA_FORGE1201_PLACE
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_PLACE

struct PlaceHandles {
    jclass    minecraft = nullptr;
    jmethodID getInstance = nullptr;
    jfieldID  options = nullptr;
    jfieldID  useKey = nullptr;
    jmethodID isDown = nullptr;
    jfieldID  rightClickDelay = nullptr;
    jfieldID  player = nullptr;
    jmethodID getMainHandItem = nullptr;
    jmethodID getItem = nullptr;
    jmethodID isEmpty = nullptr;
    jmethodID getXRot = nullptr;
    jclass    blockItem = nullptr;
};

PlaceHandles g_place;
bool g_placeLogged = false;

bool Resolve(JNIEnv* env) {
    jobject loader = GetMinecraftClassLoader(env);
    jclass minecraft = nova_base::loadOrFind(env, loader, kMinecraftDot, kMinecraftSig);
    jclass options = nova_base::loadOrFind(env, loader, kOptionsDot, kOptionsSig);
    jclass keyMapping = nova_base::loadOrFind(env, loader, kKeyMappingDot, kKeyMappingSig);
    jclass blockItem = nova_base::loadOrFind(env, loader, kBlockItemDot, kBlockItemSig);
    jclass living = nova_base::loadOrFind(env, loader, kLivingDot, kLivingSig);
    jclass entity = nova_base::loadOrFind(env, loader, kEntityDot, kEntitySig);
    jclass stack = nova_base::loadOrFind(env, loader, kStackDot, kStackSig);
    if (loader)
        env->DeleteGlobalRef(loader);

    if (!minecraft || !options || !keyMapping || !blockItem || !living || !entity || !stack) {
        if (minecraft) env->DeleteLocalRef(minecraft);
        if (options) env->DeleteLocalRef(options);
        if (keyMapping) env->DeleteLocalRef(keyMapping);
        if (blockItem) env->DeleteLocalRef(blockItem);
        if (living) env->DeleteLocalRef(living);
        if (entity) env->DeleteLocalRef(entity);
        if (stack) env->DeleteLocalRef(stack);
        return false;
    }

    jmethodID getInstance = env->GetStaticMethodID(minecraft, kGetInstance, kGetInstanceDesc);
    jfieldID optionsField = env->GetFieldID(minecraft, kOptionsField, kOptionsSig);
    jfieldID useKey = env->GetFieldID(options, kUseKeyField, kKeyMappingSig);
    jmethodID isDown = env->GetMethodID(keyMapping, kIsDown, kIsDownDesc);
    jfieldID delay = env->GetFieldID(minecraft, kDelayField, kDelayDesc);
    jfieldID player = env->GetFieldID(minecraft, kPlayerField, kPlayerSig);
    jmethodID mainHand = env->GetMethodID(living, kMainHand, kMainHandDesc);
    jmethodID getItem = env->GetMethodID(stack, kGetItem, kGetItemDesc);
    jmethodID isEmpty = env->GetMethodID(stack, kIsEmpty, kIsEmptyDesc);
    jmethodID getXRot = env->GetMethodID(entity, kGetXRot, kGetXRotDesc);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (!getInstance || !optionsField || !useKey || !isDown || !delay || !player || !mainHand ||
        !getItem || !isEmpty || !getXRot) {
        if (!g_placeLogged) {
            g_placeLogged = true;
            LogTo("Base: place unavailable (getInstance=%d, options=%d, useKey=%d, isDown=%d, "
                  "delay=%d, player=%d, mainHand=%d, getItem=%d, isEmpty=%d, xRot=%d)",
                  getInstance != nullptr, optionsField != nullptr, useKey != nullptr,
                  isDown != nullptr, delay != nullptr, player != nullptr, mainHand != nullptr,
                  getItem != nullptr, isEmpty != nullptr, getXRot != nullptr);
        }
        env->DeleteLocalRef(minecraft);
        env->DeleteLocalRef(options);
        env->DeleteLocalRef(keyMapping);
        env->DeleteLocalRef(blockItem);
        env->DeleteLocalRef(living);
        env->DeleteLocalRef(entity);
        env->DeleteLocalRef(stack);
        return false;
    }

    g_place.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));
    g_place.blockItem = static_cast<jclass>(env->NewGlobalRef(blockItem));
    g_place.getInstance = getInstance;
    g_place.options = optionsField;
    g_place.useKey = useKey;
    g_place.isDown = isDown;
    g_place.rightClickDelay = delay;
    g_place.player = player;
    g_place.getMainHandItem = mainHand;
    g_place.getItem = getItem;
    g_place.isEmpty = isEmpty;
    g_place.getXRot = getXRot;

    env->DeleteLocalRef(minecraft);
    env->DeleteLocalRef(options);
    env->DeleteLocalRef(keyMapping);
    env->DeleteLocalRef(blockItem);
    env->DeleteLocalRef(living);
    env->DeleteLocalRef(entity);
    env->DeleteLocalRef(stack);
    return g_place.minecraft && g_place.blockItem;
}

jobject Client(JNIEnv* env) {
    jobject instance = env->CallStaticObjectMethod(g_place.minecraft, g_place.getInstance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return instance;
}

// --- scaffold ---------------------------------------------------------------
//
// A second, self-contained handle set. The Resolve above is shared with the fast
// place module and is left alone; the scaffold needs a much wider slice of the
// world, and keeping it separate means a missing scaffold member cannot take the
// existing placement path down with it.

#define NOVA_FORGE1201_PLACE_TABLES
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_PLACE_TABLES

struct ScaffoldHandles {
    jclass    level = nullptr;
    jclass    gameMode = nullptr;
    jclass    player = nullptr;
    jclass    inventory = nullptr;
    jclass    blockState = nullptr;
    jclass    shape = nullptr;
    jclass    blocks = nullptr;
    jclass    pos = nullptr;
    jclass    direction = nullptr;
    jclass    vec = nullptr;
    jclass    blockHit = nullptr;
    jclass    hand = nullptr;
    jclass    result = nullptr;
    jclass    rotPacket = nullptr;
    jclass    swingPacket = nullptr;
    jclass    flower = nullptr;
    jclass    bush = nullptr;
    jclass    fungus = nullptr;
    jclass    crop = nullptr;
    jclass    slab = nullptr;
    jclass    standingWall = nullptr;

    jfieldID  levelField = nullptr;
    jfieldID  gameModeField = nullptr;
    jfieldID  inventorySelected = nullptr;

    jmethodID getInventory = nullptr;
    jmethodID invGetItem = nullptr;
    jmethodID stackCount = nullptr;
    jmethodID stackName = nullptr;
    jmethodID componentString = nullptr;
    jmethodID blockItemBlock = nullptr;
    jmethodID levelGetState = nullptr;
    jmethodID stateCollision = nullptr;
    jmethodID stateMenu = nullptr;
    jmethodID stateReplaceable = nullptr;
    jmethodID shapeEmpty = nullptr;
    jmethodID useItemOn = nullptr;
    jmethodID consumesAction = nullptr;
    jmethodID keySetDown = nullptr;
    jmethodID deltaMovement = nullptr;
    jmethodID setDeltaMovement = nullptr;
    jmethodID setSprinting = nullptr;
    jmethodID getSpeed = nullptr;
    jmethodID isSprinting = nullptr;
    jmethodID posCtor = nullptr;
    jmethodID vecCtor = nullptr;
    jmethodID blockHitCtor = nullptr;
    jmethodID rotCtor = nullptr;
    jmethodID swingCtor = nullptr;
    jmethodID swingHand = nullptr;

    jfieldID  vecX = nullptr;
    jfieldID  vecY = nullptr;
    jfieldID  vecZ = nullptr;

    jobject   mainHand = nullptr;
    jobject   keyJump = nullptr;
    jobject   keyShift = nullptr;
    jobject   facing[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    int       facingOpposite[6] = {0, 0, 0, 0, 0, 0};
    int       facingStepX[6] = {0, 0, 0, 0, 0, 0};
    int       facingStepY[6] = {0, 0, 0, 0, 0, 0};
    int       facingStepZ[6] = {0, 0, 0, 0, 0, 0};
    jobject   blacklist[kBlacklistCount] = {};

    bool      ok = false;
};

ScaffoldHandles g_scaffold;
bool g_scaffoldLogged = false;

std::atomic<int> g_tickSkip{0};

bool ResolveScaffold(JNIEnv* env) {
    if (!env)
        return false;
    if (g_scaffold.ok)
        return true;

    jobject loader = GetMinecraftClassLoader(env);
    jclass level = nova_base::loadOrFind(env, loader, kLevelDot, kLevelSig);
    jclass gameMode = nova_base::loadOrFind(env, loader, kGameModeDot, kGameModeSig);
    jclass player = nova_base::loadOrFind(env, loader, kPlayerDot, kPlayerEntitySig);
    jclass inventory = nova_base::loadOrFind(env, loader, kInventoryDot, kInventorySig);
    jclass blockState = nova_base::loadOrFind(env, loader, kBlockStateDot, kBlockStateSig);
    jclass shape = nova_base::loadOrFind(env, loader, kShapeDot, kShapeSig);
    jclass blocks = nova_base::loadOrFind(env, loader, kBlocksDot, kBlocksSig);
    jclass pos = nova_base::loadOrFind(env, loader, kPosDot, kPosSig);
    jclass direction = nova_base::loadOrFind(env, loader, kDirectionDot, kDirectionSig);
    jclass vec = nova_base::loadOrFind(env, loader, kVecDot, kVecSig);
    jclass blockHit = nova_base::loadOrFind(env, loader, kBlockHitDot, kBlockHitSig);
    jclass hand = nova_base::loadOrFind(env, loader, kHandDot, kHandSig);
    jclass result = nova_base::loadOrFind(env, loader, kResultDot, kResultSig);
    jclass rotPacket = nova_base::loadOrFind(env, loader, kRotPacketDot, kRotPacketSig);
    jclass swingPacket = nova_base::loadOrFind(env, loader, kSwingPacketDot, kSwingPacketSig);
    jclass standingWall = nova_base::loadOrFind(env, loader, kStandWallDot, kStandWallSig);
    jclass localPlayer = nova_base::loadOrFind(env, loader, kLocalPlayerDot, kLocalPlayerSig);

    jclass flower = nova_base::loadOrFind(env, loader,
                                          kMappedFlowerBlockDot,
                                          kMappedFlowerBlockSig);
    jclass bush = nova_base::loadOrFind(env, loader,
                                        kMappedBushBlockDot,
                                        kMappedBushBlockSig);
    jclass fungus = nova_base::loadOrFind(env, loader,
                                          kMappedFungusBlockDot,
                                          kMappedFungusBlockSig);
    jclass crop = nova_base::loadOrFind(env, loader,
                                        kMappedCropBlockDot,
                                        kMappedCropBlockSig);
    jclass slab = nova_base::loadOrFind(env, loader,
                                        kMappedSlabBlockDot,
                                        kMappedSlabBlockSig);
    if (loader)
        env->DeleteGlobalRef(loader);

    jfieldID levelField = env->GetFieldID(g_place.minecraft, kLevelField, kClientLevelSig);
    jfieldID gameModeField = env->GetFieldID(g_place.minecraft, kGameModeField, kGameModeSig);
    jfieldID inventorySelected = inventory ? env->GetFieldID(inventory, kInvSelected,
                                                             kInvSelectedDesc) : nullptr;

    jmethodID getInventory = player ? env->GetMethodID(player, kGetInventory, kInventoryDesc) : nullptr;
    jmethodID invGetItem = inventory ? env->GetMethodID(inventory, kInvGetItem, kInvGetItemDesc) : nullptr;

    // The first Resolve does not need ItemStack's size or name, KeyMapping.setDown,
    // the entity's motion, or the two option keys, so they are fetched here. The
    // class loader has to be asked again because the handle above was released.
    jobject lateLoader = GetMinecraftClassLoader(env);
    jclass stack = nova_base::loadOrFind(env, lateLoader, kStackDot, kStackSig);
    jclass keyMapping = nova_base::loadOrFind(env, lateLoader, kKeyMappingDot, kKeyMappingSig);
    jclass options = nova_base::loadOrFind(env, lateLoader, kOptionsDot, kOptionsSig);
    jclass entity = nova_base::loadOrFind(env, lateLoader, kEntityDot, kEntitySig);
    if (lateLoader)
        env->DeleteGlobalRef(lateLoader);

    jmethodID stackCount = stack ? env->GetMethodID(stack, kStackCount, kGetStepDesc) : nullptr;
    jmethodID stackName = stack ? env->GetMethodID(stack, kStackName, kStackNameDesc) : nullptr;
    jmethodID blockItemBlock =
        g_place.blockItem
            ? env->GetMethodID(g_place.blockItem, kBlockItemBlock, kBlockItemBlockDesc)
            : nullptr;
    jmethodID levelGetState =
        level ? env->GetMethodID(level, kLevelGetState, kLevelGetStateDesc) : nullptr;
    jmethodID stateCollision =
        blockState ? env->GetMethodID(blockState, kStateCollision, kStateCollisionDesc) : nullptr;
    jmethodID stateMenu =
        blockState ? env->GetMethodID(blockState, kStateMenu, kStateMenuDesc) : nullptr;
    jmethodID stateReplaceable =
        blockState ? env->GetMethodID(blockState, kStateReplaceable, kIsSprintingDesc) : nullptr;
    jmethodID shapeEmpty = shape ? env->GetMethodID(shape, kShapeEmpty, kShapeEmptyDesc) : nullptr;
    jmethodID useItemOn =
        gameMode ? env->GetMethodID(gameMode, kUseItemOn, kUseItemOnDesc) : nullptr;
    jmethodID consumesAction =
        result ? env->GetMethodID(result, kConsumesAction, kConsumesActionDesc) : nullptr;
    jmethodID keySetDown =
        keyMapping ? env->GetMethodID(keyMapping, kKeySetDown, kKeySetDownDesc) : nullptr;
    jmethodID deltaMovement =
        entity ? env->GetMethodID(entity, kDeltaMovement, kDeltaMovementDesc) : nullptr;
    jmethodID setDeltaMovement =
        entity ? env->GetMethodID(entity, kSetDeltaMovement, kSetDeltaMovementDesc) : nullptr;
    jmethodID setSprinting =
        entity ? env->GetMethodID(entity, kSetSprinting, kSetSprintingDesc) : nullptr;

    jmethodID isSprinting =
        entity ? env->GetMethodID(entity, kIsSprinting, kIsSprintingDesc) : nullptr;
    jmethodID getSpeed = player ? env->GetMethodID(player, kGetSpeed, kGetSpeedDesc) : nullptr;
    jmethodID posCtor = pos ? env->GetMethodID(pos, kPosCtor, kPosCtorDesc) : nullptr;
    jmethodID vecCtor = vec ? env->GetMethodID(vec, kVecCtor, kVecCtorDesc) : nullptr;
    jmethodID blockHitCtor =
        blockHit ? env->GetMethodID(blockHit, kBlockHitCtor, kBlockHitCtorDesc) : nullptr;
    jmethodID rotCtor = rotPacket ? env->GetMethodID(rotPacket, kBlockHitCtor, kRotCtorDesc) : nullptr;
    jmethodID swingCtor =
        swingPacket ? env->GetMethodID(swingPacket, kBlockHitCtor, kSwingCtorDesc) : nullptr;
    jmethodID swingHand =
        localPlayer ? env->GetMethodID(localPlayer, kSwingHand, kSwingHandDesc) : nullptr;

    jfieldID vecX = vec ? env->GetFieldID(vec, kVecX, kMappedPrimitiveDoubleSig) : nullptr;
    jfieldID vecY = vec ? env->GetFieldID(vec, kVecY, kMappedPrimitiveDoubleSig) : nullptr;
    jfieldID vecZ = vec ? env->GetFieldID(vec, kVecZ, kMappedPrimitiveDoubleSig) : nullptr;
    jfieldID keyJumpField =
        options ? env->GetFieldID(options, kKeyJumpField, kKeyMappingSig) : nullptr;
    jfieldID keyShiftField =
        options ? env->GetFieldID(options, kKeyShiftField, kKeyMappingSig) : nullptr;

    jfieldID mainHandField =
        hand ? env->GetStaticFieldID(hand, kMappedMAINHAND, kHandSig) : nullptr;

    jobject mainHand = mainHandField ? env->GetStaticObjectField(hand, mainHandField) : nullptr;

    jobject keyJump = nullptr;
    jobject keyShift = nullptr;
    if (keyJumpField || keyShiftField) {
        jobject client = Client(env);
        jobject optionsInstance = client ? env->GetObjectField(client, g_place.options) : nullptr;
        if (client)
            env->DeleteLocalRef(client);
        if (env->ExceptionCheck())
            env->ExceptionClear();
        if (optionsInstance) {
            if (keyJumpField)
                keyJump = env->GetObjectField(optionsInstance, keyJumpField);
            if (keyShiftField)
                keyShift = env->GetObjectField(optionsInstance, keyShiftField);
            env->DeleteLocalRef(optionsInstance);
        }
        if (env->ExceptionCheck())
            env->ExceptionClear();
    }

    jobject facing[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    int facingOpposite[6] = {0, 1, 3, 2, 5, 4};
    int facingStepX[6] = {0, 0, 0, 0, 0, 0};
    int facingStepY[6] = {0, 0, 0, 0, 0, 0};
    int facingStepZ[6] = {0, 0, 0, 0, 0, 0};
    if (direction) {
        jmethodID stepX = env->GetMethodID(direction, kGetStepX, kGetStepDesc);
        jmethodID stepY = env->GetMethodID(direction, kGetStepY, kGetStepDesc);
        jmethodID stepZ = env->GetMethodID(direction, kGetStepZ, kGetStepDesc);
        jmethodID opposite = env->GetMethodID(direction, kGetOpposite, kGetOppositeDesc);
        jmethodID ordinal = env->GetMethodID(direction, kMappedOrdinal, kGetStepDesc);
        if (env->ExceptionCheck())
            env->ExceptionClear();
        for (int i = 0; i < 6; ++i) {
            jfieldID id = env->GetStaticFieldID(direction, kFacingNames[i], kDirectionSig);
            jobject value = id ? env->GetStaticObjectField(direction, id) : nullptr;
            facing[i] = value;
            if (value && stepX) facingStepX[i] = env->CallIntMethod(value, stepX);
            if (value && stepY) facingStepY[i] = env->CallIntMethod(value, stepY);
            if (value && stepZ) facingStepZ[i] = env->CallIntMethod(value, stepZ);
            if (value && opposite && ordinal) {
                jobject inverted = env->CallObjectMethod(value, opposite);
                if (inverted) {
                    facingOpposite[i] = env->CallIntMethod(inverted, ordinal);
                    env->DeleteLocalRef(inverted);
                }
            }
        }
    }

    jobject blacklist[kBlacklistCount] = {};
    if (blocks) {
        for (int i = 0; i < kBlacklistCount; ++i) {
            jfieldID id = env->GetStaticFieldID(blocks, kBlacklistFields[i], kMappedBlockSig);
            blacklist[i] = id ? env->GetStaticObjectField(blocks, id) : nullptr;
        }
    }

    if (env->ExceptionCheck())
        env->ExceptionClear();

    const bool ok = levelField && gameModeField && inventorySelected && getInventory &&
                    invGetItem && stackCount && stackName && blockItemBlock && levelGetState &&
                    stateCollision && stateMenu && stateReplaceable && shapeEmpty && useItemOn &&
                    consumesAction && keySetDown && deltaMovement && setDeltaMovement &&
                    setSprinting && isSprinting && getSpeed &&
                    posCtor && vecCtor && blockHitCtor && rotCtor && swingCtor && swingHand &&
                    vecX && vecY && vecZ && mainHand && keyJump && keyShift && facing[0] &&
                    blacklist[0] &&
                    // The two packet classes and the block types are held and used
                    // later; NewObject or IsInstanceOf on a null class is a hard
                    // crash rather than a failed call, so they have to gate the
                    // whole layer just like the method ids do.
                    rotPacket && swingPacket && flower && bush && fungus && crop && slab &&
                    standingWall;
    if (!ok) {
        if (!g_scaffoldLogged) {
            g_scaffoldLogged = true;
            LogTo("Base: scaffold layer incomplete -> level=%d gameMode=%d slot=%d inv=%d "
                  "invItem=%d count=%d name=%d blockItem=%d state=%d collision=%d menu=%d "
                  "replaceable=%d shapeEmpty=%d useItemOn=%d consumes=%d setDown=%d motion=%d "
                  "sprint=%d speed=%d posCtor=%d vecCtor=%d hitCtor=%d rotCtor=%d swingCtor=%d "
                  "swingHand=%d vec=%d/%d/%d mainHand=%d jump=%d shift=%d facing=%d blacklist=%d",
                  levelField != nullptr, gameModeField != nullptr, inventorySelected != nullptr,
                  getInventory != nullptr, invGetItem != nullptr, stackCount != nullptr,
                  stackName != nullptr, blockItemBlock != nullptr, levelGetState != nullptr,
                  stateCollision != nullptr, stateMenu != nullptr, stateReplaceable != nullptr,
                  shapeEmpty != nullptr, useItemOn != nullptr, consumesAction != nullptr,
                  keySetDown != nullptr, deltaMovement != nullptr, isSprinting != nullptr,
                  getSpeed != nullptr, posCtor != nullptr, vecCtor != nullptr,
                  blockHitCtor != nullptr, rotCtor != nullptr, swingCtor != nullptr,
                  swingHand != nullptr,
                  vecX != nullptr, vecY != nullptr, vecZ != nullptr, mainHand != nullptr,
                  keyJump != nullptr, keyShift != nullptr, facing[0] != nullptr,
                  blacklist[0] != nullptr);
        }
    } else {
        g_scaffold.level = static_cast<jclass>(env->NewGlobalRef(level));
        g_scaffold.gameMode = static_cast<jclass>(env->NewGlobalRef(gameMode));
        g_scaffold.player = static_cast<jclass>(env->NewGlobalRef(player));
        g_scaffold.inventory = static_cast<jclass>(env->NewGlobalRef(inventory));
        g_scaffold.blockState = static_cast<jclass>(env->NewGlobalRef(blockState));
        g_scaffold.shape = static_cast<jclass>(env->NewGlobalRef(shape));
        g_scaffold.blocks = static_cast<jclass>(env->NewGlobalRef(blocks));
        g_scaffold.pos = static_cast<jclass>(env->NewGlobalRef(pos));
        g_scaffold.direction = static_cast<jclass>(env->NewGlobalRef(direction));
        g_scaffold.vec = static_cast<jclass>(env->NewGlobalRef(vec));
        g_scaffold.blockHit = static_cast<jclass>(env->NewGlobalRef(blockHit));
        g_scaffold.hand = static_cast<jclass>(env->NewGlobalRef(hand));
        g_scaffold.result = static_cast<jclass>(env->NewGlobalRef(result));
        g_scaffold.rotPacket = static_cast<jclass>(env->NewGlobalRef(rotPacket));
        g_scaffold.swingPacket = static_cast<jclass>(env->NewGlobalRef(swingPacket));
        g_scaffold.flower = static_cast<jclass>(env->NewGlobalRef(flower));
        g_scaffold.bush = static_cast<jclass>(env->NewGlobalRef(bush));
        g_scaffold.fungus = static_cast<jclass>(env->NewGlobalRef(fungus));
        g_scaffold.crop = static_cast<jclass>(env->NewGlobalRef(crop));
        g_scaffold.slab = static_cast<jclass>(env->NewGlobalRef(slab));
        g_scaffold.standingWall = static_cast<jclass>(env->NewGlobalRef(standingWall));

        g_scaffold.levelField = levelField;
        g_scaffold.gameModeField = gameModeField;
        g_scaffold.inventorySelected = inventorySelected;
        g_scaffold.getInventory = getInventory;
        g_scaffold.invGetItem = invGetItem;
        g_scaffold.stackCount = stackCount;
        g_scaffold.stackName = stackName;
        g_scaffold.blockItemBlock = blockItemBlock;
        g_scaffold.levelGetState = levelGetState;
        g_scaffold.stateCollision = stateCollision;
        g_scaffold.stateMenu = stateMenu;
        g_scaffold.stateReplaceable = stateReplaceable;
        g_scaffold.shapeEmpty = shapeEmpty;
        g_scaffold.useItemOn = useItemOn;
        g_scaffold.consumesAction = consumesAction;
        g_scaffold.keySetDown = keySetDown;
        g_scaffold.deltaMovement = deltaMovement;
        g_scaffold.setDeltaMovement = setDeltaMovement;
        g_scaffold.setSprinting = setSprinting;
        g_scaffold.getSpeed = getSpeed;
        g_scaffold.isSprinting = isSprinting;
        g_scaffold.posCtor = posCtor;
        g_scaffold.vecCtor = vecCtor;
        g_scaffold.blockHitCtor = blockHitCtor;
        g_scaffold.rotCtor = rotCtor;
        g_scaffold.swingCtor = swingCtor;
        g_scaffold.swingHand = swingHand;
        g_scaffold.vecX = vecX;
        g_scaffold.vecY = vecY;
        g_scaffold.vecZ = vecZ;
        g_scaffold.mainHand = env->NewGlobalRef(mainHand);
        g_scaffold.keyJump = env->NewGlobalRef(keyJump);
        g_scaffold.keyShift = env->NewGlobalRef(keyShift);
        for (int i = 0; i < 6; ++i) {
            g_scaffold.facing[i] = env->NewGlobalRef(facing[i]);
            g_scaffold.facingOpposite[i] = facingOpposite[i];
            g_scaffold.facingStepX[i] = facingStepX[i];
            g_scaffold.facingStepY[i] = facingStepY[i];
            g_scaffold.facingStepZ[i] = facingStepZ[i];
        }
        for (int i = 0; i < kBlacklistCount; ++i)
            g_scaffold.blacklist[i] = env->NewGlobalRef(blacklist[i]);
        g_scaffold.ok = true;
    }

    if (level) env->DeleteLocalRef(level);
    if (gameMode) env->DeleteLocalRef(gameMode);
    if (player) env->DeleteLocalRef(player);
    if (inventory) env->DeleteLocalRef(inventory);
    if (blockState) env->DeleteLocalRef(blockState);
    if (shape) env->DeleteLocalRef(shape);
    if (blocks) env->DeleteLocalRef(blocks);
    if (pos) env->DeleteLocalRef(pos);
    if (direction) env->DeleteLocalRef(direction);
    if (vec) env->DeleteLocalRef(vec);
    if (blockHit) env->DeleteLocalRef(blockHit);
    if (hand) env->DeleteLocalRef(hand);
    if (result) env->DeleteLocalRef(result);
    if (rotPacket) env->DeleteLocalRef(rotPacket);
    if (swingPacket) env->DeleteLocalRef(swingPacket);
    if (standingWall) env->DeleteLocalRef(standingWall);
    if (localPlayer) env->DeleteLocalRef(localPlayer);
    if (flower) env->DeleteLocalRef(flower);
    if (bush) env->DeleteLocalRef(bush);
    if (fungus) env->DeleteLocalRef(fungus);
    if (crop) env->DeleteLocalRef(crop);
    if (slab) env->DeleteLocalRef(slab);
    if (stack) env->DeleteLocalRef(stack);
    if (keyMapping) env->DeleteLocalRef(keyMapping);
    if (options) env->DeleteLocalRef(options);
    if (entity) env->DeleteLocalRef(entity);
    if (mainHand) env->DeleteLocalRef(mainHand);
    if (keyJump) env->DeleteLocalRef(keyJump);
    if (keyShift) env->DeleteLocalRef(keyShift);
    for (int i = 0; i < 6; ++i)
        if (facing[i]) env->DeleteLocalRef(facing[i]);
    for (int i = 0; i < kBlacklistCount; ++i)
        if (blacklist[i]) env->DeleteLocalRef(blacklist[i]);

    return g_scaffold.ok;
}

// Lazy wrappers so every public entry below reads the same way the other layers do.
bool ScaffoldReady(JNIEnv* env) {
    if (!g_place.minecraft && !Resolve(env))
        return false;
    return ResolveScaffold(env);
}

jobject ScaffoldClient(JNIEnv* env) { return Client(env); }

jobject ScaffoldLevel(JNIEnv* env) {
    jobject client = Client(env);
    if (client == nullptr)
        return nullptr;
    jobject level = env->GetObjectField(client, g_scaffold.levelField);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return level;
}

jobject ScaffoldPlayer(JNIEnv* env) {
    jobject client = Client(env);
    if (client == nullptr)
        return nullptr;
    jobject player = env->GetObjectField(client, g_place.player);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return player;
}

jobject MakeBlockPos(JNIEnv* env, const BlockPos& pos) {
    return env->NewObject(g_scaffold.pos, g_scaffold.posCtor, static_cast<jint>(pos.x),
                          static_cast<jint>(pos.y), static_cast<jint>(pos.z));
}

jobject MakeVec3(JNIEnv* env, const rotation::Vec3& value) {
    return env->NewObject(g_scaffold.vec, g_scaffold.vecCtor, static_cast<jdouble>(value.x),
                          static_cast<jdouble>(value.y), static_cast<jdouble>(value.z));
}

jobject CurrentBlockState(JNIEnv* env, jobject level, const BlockPos& pos) {
    jobject blockPos = MakeBlockPos(env, pos);
    if (blockPos == nullptr)
        return nullptr;
    jobject state = env->CallObjectMethod(level, g_scaffold.levelGetState, blockPos);
    env->DeleteLocalRef(blockPos);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return state;
}

// The block half of isValidStack. The display-name test is separate because it is
// the only part that has to reach into Java strings.
bool BlockIsPlaceable(JNIEnv* env, jobject block) {
    if (block == nullptr)
        return false;
    if (g_scaffold.flower == nullptr || g_scaffold.bush == nullptr ||
        g_scaffold.fungus == nullptr || g_scaffold.crop == nullptr || g_scaffold.slab == nullptr)
        return false;

    if (env->IsInstanceOf(block, g_scaffold.flower) == JNI_TRUE ||
        env->IsInstanceOf(block, g_scaffold.bush) == JNI_TRUE ||
        env->IsInstanceOf(block, g_scaffold.fungus) == JNI_TRUE ||
        env->IsInstanceOf(block, g_scaffold.crop) == JNI_TRUE ||
        env->IsInstanceOf(block, g_scaffold.slab) == JNI_TRUE) {
        return false;
    }
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }

    for (int i = 0; i < kBlacklistCount; ++i) {
        if (g_scaffold.blacklist[i] && env->IsSameObject(block, g_scaffold.blacklist[i]))
            return false;
    }
    return true;
}

bool ItemIsPlaceable(JNIEnv* env, jobject stack) {
    if (stack == nullptr || g_place.isEmpty == nullptr || g_place.getItem == nullptr)
        return false;

    if (env->CallBooleanMethod(stack, g_place.isEmpty) == JNI_TRUE)
        return false;

    jobject item = env->CallObjectMethod(stack, g_place.getItem);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (item == nullptr)
        return false;

    const bool isBlockItem = env->IsInstanceOf(item, g_place.blockItem) == JNI_TRUE;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(item);
        return false;
    }
    if (!isBlockItem) {
        env->DeleteLocalRef(item);
        return false;
    }

    // A wall-standing block places on the side of a block rather than on top of
    // one, which is not what the scaffold wants.
    if (g_scaffold.standingWall &&
        env->IsInstanceOf(item, g_scaffold.standingWall) == JNI_TRUE) {
        env->DeleteLocalRef(item);
        return false;
    }

    jobject block = env->CallObjectMethod(item, g_scaffold.blockItemBlock);
    env->DeleteLocalRef(item);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (block == nullptr)
        return false;

    const bool placeable = BlockIsPlaceable(env, block);
    env->DeleteLocalRef(block);
    return placeable;
}

// Starain rejects items whose display name carries "Click" or the Chinese word for
// it -- server shops hand those out and placing one opens a menu. Reading the name
// is the one place this layer touches a Java string, so the answer is remembered
// per Item: the check would otherwise run nine times a tick.
struct NameVerdict {
    jobject item = nullptr;
    bool    blocked = false;
};

constexpr int kNameCacheSize = 32;
NameVerdict g_nameCache[kNameCacheSize];
int g_nameCacheNext = 0;

// The name is read off the stack (a stack can carry a custom name) but remembered
// against the item, which is what actually repeats across the hotbar.
bool ItemNameIsBlocked(JNIEnv* env, jobject stack, jobject item) {
    if (item == nullptr || stack == nullptr)
        return false;

    for (int i = 0; i < kNameCacheSize; ++i) {
        if (g_nameCache[i].item && env->IsSameObject(g_nameCache[i].item, item))
            return g_nameCache[i].blocked;
    }

    bool blocked = false;
    jobject component = env->CallObjectMethod(stack, g_scaffold.stackName);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        component = nullptr;
    }
    if (component != nullptr) {
        jclass componentClass = env->GetObjectClass(component);
        jmethodID getString =
            componentClass ? env->GetMethodID(componentClass, kComponentString, kComponentStringDesc)
                           : nullptr;
        if (env->ExceptionCheck())
            env->ExceptionClear();
        if (getString) {
            jstring text = static_cast<jstring>(env->CallObjectMethod(component, getString));
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                text = nullptr;
            }
            if (text != nullptr) {
                const char* utf = env->GetStringUTFChars(text, nullptr);
                if (utf != nullptr) {
                    const std::string name(utf);
                    env->ReleaseStringUTFChars(text, utf);
                    blocked = name.find("Click") != std::string::npos ||
                              name.find("\xe7\x82\xb9\xe5\x87\xbb") != std::string::npos;
                }
                env->DeleteLocalRef(text);
            }
        }
        if (componentClass)
            env->DeleteLocalRef(componentClass);
        env->DeleteLocalRef(component);
    }

    NameVerdict& slot = g_nameCache[g_nameCacheNext];
    g_nameCacheNext = (g_nameCacheNext + 1) % kNameCacheSize;
    if (slot.item)
        env->DeleteGlobalRef(slot.item);
    slot.item = env->NewGlobalRef(item);
    slot.blocked = blocked;
    return blocked;
}

}

bool RightClickDelay(int& out) {
    if (compat::UsesModernRuntime()) { return compat::mc263::RightClickDelay(out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::RightClickDelay(out); }
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_place.minecraft && !Resolve(env))
        return false;

    jobject client = Client(env);
    if (client == nullptr)
        return false;

    out = env->GetIntField(client, g_place.rightClickDelay);
    const bool clean = !env->ExceptionCheck();
    if (env->ExceptionCheck())
        env->ExceptionClear();
    env->DeleteLocalRef(client);
    return clean;
}

bool SetRightClickDelay(int value) {
    if (compat::UsesModernRuntime()) { return compat::mc263::SetRightClickDelay(value); }
    if (compat::IsLegacyLunar()) { return compat::lunar::SetRightClickDelay(value); }
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_place.minecraft && !Resolve(env))
        return false;

    jobject client = Client(env);
    if (client == nullptr)
        return false;

    env->SetIntField(client, g_place.rightClickDelay, value);
    const bool clean = !env->ExceptionCheck();
    if (env->ExceptionCheck())
        env->ExceptionClear();
    env->DeleteLocalRef(client);
    return clean;
}

bool UseKeyDown() {
    if (compat::UsesModernRuntime()) { return compat::mc263::KeyDown(compat::mc263::InputKey::Use); }
    if (compat::IsLegacyLunar()) { return compat::lunar::KeyDown(compat::lunar::InputKey::Use); }
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_place.minecraft && !Resolve(env))
        return false;

    jobject client = Client(env);
    if (client == nullptr)
        return false;

    jobject options = env->GetObjectField(client, g_place.options);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (options == nullptr)
        return false;

    jobject key = env->GetObjectField(options, g_place.useKey);
    env->DeleteLocalRef(options);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (key == nullptr)
        return false;

    const jboolean down = env->CallBooleanMethod(key, g_place.isDown);
    env->DeleteLocalRef(key);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return down == JNI_TRUE;
}

bool HoldingBlockItem() {
    if (compat::UsesModernRuntime()) { return compat::mc263::HoldingBlockItem(); }
    if (compat::IsLegacyLunar()) { return compat::lunar::HoldingBlockItem(); }
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_place.minecraft && !Resolve(env))
        return false;

    jobject client = Client(env);
    if (client == nullptr)
        return false;

    jobject player = env->GetObjectField(client, g_place.player);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (player == nullptr)
        return false;

    jobject stack = env->CallObjectMethod(player, g_place.getMainHandItem);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (stack == nullptr)
        return false;

    const jboolean empty = env->CallBooleanMethod(stack, g_place.isEmpty);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(stack);
        return false;
    }
    if (empty == JNI_TRUE) {
        env->DeleteLocalRef(stack);
        return false;
    }

    jobject item = env->CallObjectMethod(stack, g_place.getItem);
    env->DeleteLocalRef(stack);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (item == nullptr)
        return false;

    const jboolean isBlock = env->IsInstanceOf(item, g_place.blockItem);
    env->DeleteLocalRef(item);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return isBlock == JNI_TRUE;
}

float PlayerPitch() {
    if (compat::UsesModernRuntime()) { float yaw = 0, pitch = 0; compat::mc263::PlayerRotation(yaw, pitch); return pitch; }
    if (compat::IsLegacyLunar()) { float yaw = 0, pitch = 0; compat::lunar::PlayerRotation(yaw, pitch); return pitch; }
    JniAttach attach;
    if (!attach)
        return 0.0f;
    JNIEnv* env = attach.env;

    if (!g_place.minecraft && !Resolve(env))
        return 0.0f;

    jobject client = Client(env);
    if (client == nullptr)
        return 0.0f;

    jobject player = env->GetObjectField(client, g_place.player);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return 0.0f;
    }
    if (player == nullptr)
        return 0.0f;

    const jfloat pitch = env->CallFloatMethod(player, g_place.getXRot);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return 0.0f;
    }
    return static_cast<float>(pitch);
}

// --- scaffold ---------------------------------------------------------------

bool BlockStateCollisionEmpty(const BlockPos& pos) {
    if (compat::UsesModernRuntime()) return compat::mc263::ReadBlockProperty(pos, compat::mc263::BlockProperty::CollisionEmpty);
    if (compat::IsLegacyLunar()) return compat::lunar::ReadBlockProperty(pos, compat::lunar::BlockProperty::CollisionEmpty);
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject level = ScaffoldLevel(env);
    if (level == nullptr)
        return false;
    jobject state = CurrentBlockState(env, level, pos);
    jobject blockPos = MakeBlockPos(env, pos);
    if (state == nullptr || blockPos == nullptr) {
        if (state) env->DeleteLocalRef(state);
        if (blockPos) env->DeleteLocalRef(blockPos);
        env->DeleteLocalRef(level);
        return false;
    }

    jobject shape = env->CallObjectMethod(state, g_scaffold.stateCollision, level, blockPos);
    env->DeleteLocalRef(blockPos);
    env->DeleteLocalRef(state);
    env->DeleteLocalRef(level);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (shape == nullptr)
        return false;

    const jboolean empty = env->CallBooleanMethod(shape, g_scaffold.shapeEmpty);
    env->DeleteLocalRef(shape);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return empty == JNI_TRUE;
}

bool BlockStateOpensMenu(const BlockPos& pos) {
    if (compat::UsesModernRuntime()) return compat::mc263::ReadBlockProperty(pos, compat::mc263::BlockProperty::OpensMenu);
    if (compat::IsLegacyLunar()) return compat::lunar::ReadBlockProperty(pos, compat::lunar::BlockProperty::OpensMenu);
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject level = ScaffoldLevel(env);
    if (level == nullptr)
        return false;
    jobject state = CurrentBlockState(env, level, pos);
    jobject blockPos = MakeBlockPos(env, pos);
    if (state == nullptr || blockPos == nullptr) {
        if (state) env->DeleteLocalRef(state);
        if (blockPos) env->DeleteLocalRef(blockPos);
        env->DeleteLocalRef(level);
        return false;
    }

    jobject provider = env->CallObjectMethod(state, g_scaffold.stateMenu, level, blockPos);
    env->DeleteLocalRef(blockPos);
    env->DeleteLocalRef(state);
    env->DeleteLocalRef(level);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (provider != nullptr) {
        env->DeleteLocalRef(provider);
        return true;
    }
    return false;
}

bool ScaffoldExcludedRegistryName(std::string_view name) {
    if (name.empty()) return true;
    for (const auto excluded : {"minecraft:chest", "minecraft:trapped_chest", "minecraft:anvil",
            "minecraft:crafting_table", "minecraft:furnace", "minecraft:lit_furnace",
            "minecraft:hopper", "minecraft:dropper", "minecraft:dispenser",
            "minecraft:noteblock", "minecraft:note_block", "minecraft:jukebox"})
        if (name == excluded) return true;
    return false;
}

bool ScaffoldExcludedObject(JNIEnv* env, jobject blockOrState) {
    if (!env || !blockOrState || env->PushLocalFrame(8) != JNI_OK) {
        if (env && env->ExceptionCheck()) env->ExceptionClear();
        return true;
    }
    // Block.toString() contains its registry ID on 1.8 and newer profiles;
    // 1.7 reads Block.blockRegistry in its runtime adapter instead.
    // BlockState adds properties after the closing brace. Object.toString is
    // a Java platform method, so its name does not depend on game mappings.
    jclass type = env->GetObjectClass(blockOrState);
    jmethodID method = type ? env->GetMethodID(type, "toString", "()Ljava/lang/String;") : nullptr;
    auto text = method ? static_cast<jstring>(env->CallObjectMethod(blockOrState, method)) : nullptr;
    bool excluded = true;
    if (!env->ExceptionCheck() && text) {
        const char* bytes = env->GetStringUTFChars(text, nullptr);
        if (bytes) {
            std::string_view value(bytes);
            const auto begin = value.find("Block{");
            const auto end = begin == std::string_view::npos ? std::string_view::npos : value.find('}',begin+6);
            if (end != std::string_view::npos) {
                const auto id = value.substr(begin+6,end-begin-6);
                excluded = ScaffoldExcludedRegistryName(id);
            }
            env->ReleaseStringUTFChars(text,bytes);
        }
    }
    if (env->ExceptionCheck()) env->ExceptionClear();
    env->PopLocalFrame(nullptr);
    return excluded;
}

bool BlockStateScaffoldExcluded(const BlockPos& pos) {
    if (compat::UsesModernRuntime()) return compat::mc263::ReadBlockProperty(pos, compat::mc263::BlockProperty::ScaffoldExcluded);
    if (compat::IsLegacyLunar()) return compat::lunar::ReadBlockProperty(pos, compat::lunar::BlockProperty::ScaffoldExcluded);
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env)) return true;
    auto* env = attach.env;
    jobject level = ScaffoldLevel(env);
    if (!level) return true;
    jobject state = CurrentBlockState(env,level,pos);
    const bool excluded = ScaffoldExcludedObject(env,state);
    if (state) env->DeleteLocalRef(state);
    env->DeleteLocalRef(level);
    return excluded;
}

bool BlockStateReplaceable(const BlockPos& pos) {
    if (compat::UsesModernRuntime()) return compat::mc263::ReadBlockProperty(pos, compat::mc263::BlockProperty::Replaceable);
    if (compat::IsLegacyLunar()) return compat::lunar::ReadBlockProperty(pos, compat::lunar::BlockProperty::Replaceable);
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject level = ScaffoldLevel(env);
    if (level == nullptr)
        return false;
    jobject state = CurrentBlockState(env, level, pos);
    env->DeleteLocalRef(level);
    if (state == nullptr)
        return false;

    const jboolean replaceable = env->CallBooleanMethod(state, g_scaffold.stateReplaceable);
    env->DeleteLocalRef(state);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return replaceable == JNI_TRUE;
}

namespace {

// Inventory access shared by the four hotbar entry points below.
jobject CurrentInventory(JNIEnv* env) {
    jobject player = ScaffoldPlayer(env);
    if (player == nullptr)
        return nullptr;
    jobject inventory = env->CallObjectMethod(player, g_scaffold.getInventory);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return inventory;
}

}  // namespace

bool SelectedSlot(int& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::SelectedSlot(out);
    if (compat::IsLegacyLunar()) return compat::lunar::SelectedSlot(out);
    out = -1;
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject inventory = CurrentInventory(env);
    if (inventory == nullptr)
        return false;

    const jint slot = env->GetIntField(inventory, g_scaffold.inventorySelected);
    env->DeleteLocalRef(inventory);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    out = static_cast<int>(slot);
    return true;
}

bool SetSelectedSlot(int value) {
    if (!CanChangePhysicalSlot()) return false;
    if (compat::UsesModernRuntime()) return compat::mc263::SetSelectedSlot(value);
    if (compat::IsLegacyLunar()) return compat::lunar::SetSelectedSlot(value);
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject inventory = CurrentInventory(env);
    if (inventory == nullptr)
        return false;

    env->SetIntField(inventory, g_scaffold.inventorySelected, static_cast<jint>(value));
    env->DeleteLocalRef(inventory);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

bool HotbarSlotPlaceable(int slot) {
    if (compat::UsesModernRuntime()) return compat::mc263::HotbarSlotPlaceable(slot);
    if (compat::IsLegacyLunar()) return compat::lunar::HotbarSlotPlaceable(slot);
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject inventory = CurrentInventory(env);
    if (inventory == nullptr)
        return false;

    jobject stack = env->CallObjectMethod(inventory, g_scaffold.invGetItem,
                                          static_cast<jint>(slot));
    env->DeleteLocalRef(inventory);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (stack == nullptr)
        return false;

    // The name test runs first so the item handle it needs is still alive, and it
    // is the one that carries the per-item memory.
    bool placeable = ItemIsPlaceable(env, stack);
    if (placeable) {
        jobject item = env->CallObjectMethod(stack, g_place.getItem);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            item = nullptr;
        }
        if (item == nullptr) {
            placeable = false;
        } else {
            placeable = !ItemNameIsBlocked(env, stack, item);
            env->DeleteLocalRef(item);
        }
    }

    env->DeleteLocalRef(stack);
    return placeable;
}

int HotbarSlotCount(int slot) {
    if (compat::UsesModernRuntime()) return compat::mc263::HotbarSlotCount(slot);
    if (compat::IsLegacyLunar()) return compat::lunar::HotbarSlotCount(slot);
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return 0;
    JNIEnv* env = attach.env;

    jobject inventory = CurrentInventory(env);
    if (inventory == nullptr)
        return 0;

    jobject stack = env->CallObjectMethod(inventory, g_scaffold.invGetItem,
                                          static_cast<jint>(slot));
    env->DeleteLocalRef(inventory);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return 0;
    }
    if (stack == nullptr)
        return 0;

    const jint count = env->CallIntMethod(stack, g_scaffold.stackCount);
    env->DeleteLocalRef(stack);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return 0;
    }
    return static_cast<int>(count);
}

int FindPlaceableBlockSlot(int reserve) {
    for (int slot = 0; slot < 9; ++slot) {
        if (HotbarSlotPlaceable(slot) && HotbarSlotCount(slot) > reserve)
            return slot;
    }
    return -1;
}

int CountPlaceableBlocks() {
    int total = 0;
    for (int slot = 0; slot < 9; ++slot) {
        if (HotbarSlotPlaceable(slot))
            total += HotbarSlotCount(slot);
    }
    return total;
}

int FacingStepX(int facing) {
    if (compat::UsesModernRuntime()) return compat::mc263::FacingStep(facing, 0);
    if (compat::IsLegacyLunar()) return compat::lunar::FacingStep(facing, 0);
    return facing >= 0 && facing < 6 ? g_scaffold.facingStepX[facing] : 0;
}

int FacingStepY(int facing) {
    if (compat::UsesModernRuntime()) return compat::mc263::FacingStep(facing, 1);
    if (compat::IsLegacyLunar()) return compat::lunar::FacingStep(facing, 1);
    return facing >= 0 && facing < 6 ? g_scaffold.facingStepY[facing] : 0;
}

int FacingStepZ(int facing) {
    if (compat::UsesModernRuntime()) return compat::mc263::FacingStep(facing, 2);
    if (compat::IsLegacyLunar()) return compat::lunar::FacingStep(facing, 2);
    return facing >= 0 && facing < 6 ? g_scaffold.facingStepZ[facing] : 0;
}

int FacingOpposite(int facing) {
    if (compat::UsesModernRuntime()) return compat::mc263::FacingOpposite(facing);
    if (compat::IsLegacyLunar()) return compat::lunar::FacingOpposite(facing);
    return facing >= 0 && facing < 6 ? g_scaffold.facingOpposite[facing] : facing;
}

bool UseItemOnBlock(const BlockPos& pos, int facing, const rotation::Vec3& hitVector) {
    if (FreeCamBlocks(FreeCamAction::Place)) return false;
    if (compat::UsesModernRuntime()) return compat::mc263::UseItemOnBlock(pos, facing, hitVector);
    if (compat::IsLegacyLunar()) return compat::lunar::UseItemOnBlock(pos, facing, hitVector);
    if (facing < 0 || facing > 5)
        return false;

    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject client = ScaffoldClient(env);
    if (client == nullptr)
        return false;
    jobject player = env->GetObjectField(client, g_place.player);
    jobject gameMode = env->GetObjectField(client, g_scaffold.gameModeField);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        if (player) env->DeleteLocalRef(player);
        if (gameMode) env->DeleteLocalRef(gameMode);
        return false;
    }
    if (player == nullptr || gameMode == nullptr) {
        if (player) env->DeleteLocalRef(player);
        if (gameMode) env->DeleteLocalRef(gameMode);
        return false;
    }

    jobject vec = MakeVec3(env, hitVector);
    jobject blockPos = MakeBlockPos(env, pos);
    if (vec == nullptr || blockPos == nullptr) {
        if (vec) env->DeleteLocalRef(vec);
        if (blockPos) env->DeleteLocalRef(blockPos);
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(gameMode);
        return false;
    }

    jobject hit = env->NewObject(g_scaffold.blockHit, g_scaffold.blockHitCtor, vec,
                                 g_scaffold.facing[facing], blockPos, JNI_FALSE);
    env->DeleteLocalRef(vec);
    env->DeleteLocalRef(blockPos);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(gameMode);
        return false;
    }
    if (hit == nullptr) {
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(gameMode);
        return false;
    }

    jobject result = env->CallObjectMethod(gameMode, g_scaffold.useItemOn, player,
                                           g_scaffold.mainHand, hit);
    env->DeleteLocalRef(hit);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(gameMode);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (result == nullptr)
        return false;

    const jboolean consumed = env->CallBooleanMethod(result, g_scaffold.consumesAction);
    env->DeleteLocalRef(result);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return consumed == JNI_TRUE;
}

bool SendMoveRotation(float yaw, float pitch, bool onGround) {
    if (FreeCamActive()) return false;
    if (compat::UsesModernRuntime()) return compat::mc263::SendMoveRotation(yaw, pitch, onGround);
    if (compat::IsLegacyLunar()) return compat::lunar::SendMoveRotation(yaw, pitch, onGround);
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    // NewObject on a null class is a hard crash inside the VM rather than a
    // failed call, so every one of these is checked before it is reached.
    if (g_scaffold.rotPacket == nullptr || g_scaffold.rotCtor == nullptr)
        return false;

    jobject packet = env->NewObject(g_scaffold.rotPacket, g_scaffold.rotCtor,
                                    static_cast<jfloat>(yaw), static_cast<jfloat>(pitch),
                                    onGround ? JNI_TRUE : JNI_FALSE);
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

bool SendSwingPacket() {
    if (compat::UsesModernRuntime()) return compat::mc263::SendSwingPacket();
    if (compat::IsLegacyLunar()) return compat::lunar::SendSwingPacket();
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    if (g_scaffold.swingPacket == nullptr || g_scaffold.swingCtor == nullptr ||
        g_scaffold.mainHand == nullptr)
        return false;

    jobject packet = env->NewObject(g_scaffold.swingPacket, g_scaffold.swingCtor,
                                    g_scaffold.mainHand);
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

bool SwingMainHand() {
    if (compat::UsesModernRuntime()) { return compat::mc263::SwingHand(); }
    if (compat::IsLegacyLunar()) { return compat::lunar::SwingHand(); }
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    if (g_scaffold.swingHand == nullptr || g_scaffold.mainHand == nullptr)
        return false;

    jobject player = ScaffoldPlayer(env);
    if (player == nullptr)
        return false;

    env->CallVoidMethod(player, g_scaffold.swingHand, g_scaffold.mainHand);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

bool KeyJumpDown() {
    if (compat::UsesModernRuntime()) { return compat::mc263::KeyDown(compat::mc263::InputKey::Jump); }
    if (compat::IsLegacyLunar()) { return compat::lunar::KeyDown(compat::lunar::InputKey::Jump); }
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    if (g_scaffold.keyJump == nullptr)
        return false;
    const jboolean down = env->CallBooleanMethod(g_scaffold.keyJump, g_place.isDown);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return down == JNI_TRUE;
}

bool KeyShiftDown() {
    if (compat::UsesModernRuntime()) { return compat::mc263::KeyDown(compat::mc263::InputKey::Sneak); }
    if (compat::IsLegacyLunar()) { return compat::lunar::KeyDown(compat::lunar::InputKey::Sneak); }
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    if (g_scaffold.keyShift == nullptr)
        return false;
    const jboolean down = env->CallBooleanMethod(g_scaffold.keyShift, g_place.isDown);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return down == JNI_TRUE;
}

bool SetKeyShiftDown(bool down) {
    if (compat::UsesModernRuntime()) { return compat::mc263::SetKeyShiftDown(down); }
    if (compat::IsLegacyLunar()) { return compat::lunar::SetKeyShiftDown(down); }
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    if (g_scaffold.keyShift == nullptr)
        return false;
    env->CallVoidMethod(g_scaffold.keyShift, g_scaffold.keySetDown,
                        down ? JNI_TRUE : JNI_FALSE);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

bool PlayerDeltaMovement(rotation::Vec3& out) {
    if (compat::UsesModernRuntime()) { return compat::mc263::PlayerDeltaMovement(out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::PlayerDeltaMovement(out); }
    out = rotation::Vec3{};
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject player = ScaffoldPlayer(env);
    if (player == nullptr)
        return false;

    jobject motion = env->CallObjectMethod(player, g_scaffold.deltaMovement);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (motion == nullptr)
        return false;

    out.x = static_cast<double>(env->GetDoubleField(motion, g_scaffold.vecX));
    out.y = static_cast<double>(env->GetDoubleField(motion, g_scaffold.vecY));
    out.z = static_cast<double>(env->GetDoubleField(motion, g_scaffold.vecZ));
    env->DeleteLocalRef(motion);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

bool SetPlayerDeltaMovement(double x, double y, double z) {
    if (FreeCamActive()) return false;
    if (compat::UsesModernRuntime()) { return compat::mc263::SetPlayerDeltaMovement(x, y, z); }
    if (compat::IsLegacyLunar()) { return compat::lunar::SetPlayerDeltaMovement(x, y, z); }
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject player = ScaffoldPlayer(env);
    if (player == nullptr)
        return false;

    env->CallVoidMethod(player, g_scaffold.setDeltaMovement, static_cast<jdouble>(x),
                        static_cast<jdouble>(y), static_cast<jdouble>(z));
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

bool SetPlayerSprinting(bool sprinting) {
    if (FreeCamActive()) return false;
    if (compat::UsesModernRuntime()) { return compat::mc263::SetPlayerSprinting(sprinting); }
    if (compat::IsLegacyLunar()) { return compat::lunar::SetPlayerSprinting(sprinting); }
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject player = ScaffoldPlayer(env);
    if (player == nullptr)
        return false;

    env->CallVoidMethod(player, g_scaffold.setSprinting, sprinting ? JNI_TRUE : JNI_FALSE);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

bool PlayerSpeed(float& out) {
    if (compat::UsesModernRuntime()) { return compat::mc263::PlayerSpeed(out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::PlayerSpeed(out); }
    out = 0.0f;
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject player = ScaffoldPlayer(env);
    if (player == nullptr)
        return false;

    const jfloat speed = env->CallFloatMethod(player, g_scaffold.getSpeed);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    out = static_cast<float>(speed);
    return true;
}

bool PlayerSprinting(bool& out) {
    if (compat::UsesModernRuntime()) { return compat::mc263::PlayerSprinting(out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::PlayerSprinting(out); }
    out = false;
    JniAttach attach;
    if (!attach || !ScaffoldReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject player = ScaffoldPlayer(env);
    if (player == nullptr)
        return false;

    const jboolean sprinting = env->CallBooleanMethod(player, g_scaffold.isSprinting);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    out = sprinting == JNI_TRUE;
    return true;
}

void RequestPlayerTickSkip() {
    g_tickSkip.fetch_add(1, std::memory_order_acq_rel);
}

bool ShouldSkipPlayerTick(jobject entity) {
    // The counter is zero except for the handful of ticks the scaffold asks for,
    // and this runs once per entity per tick, so the cheap test comes first and
    // nothing below it touches the JVM in the common case.
    if (entity == nullptr || g_tickSkip.load(std::memory_order_acquire) <= 0)
        return false;

    EntityHandle player = PlayerEntity();
    if (!player.Valid())
        return false;

    JniAttach attach;
    if (!attach)
        return false;
    if (attach.env->IsSameObject(entity, static_cast<jobject>(player.raw())) != JNI_TRUE)
        return false;

    g_tickSkip.fetch_sub(1, std::memory_order_acq_rel);
    return true;
}

}
