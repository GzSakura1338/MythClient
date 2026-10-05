#include "minecraft/java/api/game_tool.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"

#include <chrono>

#include "minecraft/java/api/jni_lookup.h"
#include "minecraft/utils/inventory/InventoryUtils.h"

// Screen, game mode, and hotbar scoring for the auto tool.
//
// A third self-contained handle set, for the same reason the scaffold has its own:
// a missing member here must not be able to take the placement or the inventory
// path down with it. What this one reads is small -- the crosshair, the break
// state, and one hotbar -- so it resolves only what those need.

namespace nova::java {
namespace {

#define NOVA_FORGE1201_TOOL
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_TOOL

// HitResult.Type reads MISS, BLOCK, ENTITY. The layer trusts the ordinal rather
// than holding the enum constant, which is how WorldClip already tells a block hit
// from an entity hit.
constexpr jint kHitTypeBlock = 1;

struct ToolHandles {
    jclass    minecraft = nullptr;
    jclass    pos = nullptr;
    jclass    blockState = nullptr;
    jclass    web = nullptr;
    jclass    dropXp = nullptr;
    jclass    ore = nullptr;

    jmethodID getInstance = nullptr;
    jmethodID posCtor = nullptr;
    jmethodID levelGetState = nullptr;
    jmethodID stateIsAir = nullptr;
    jmethodID stateGetBlock = nullptr;
    jmethodID stackDestroySpeed = nullptr;
    jmethodID getInventory = nullptr;
    jmethodID invGetItem = nullptr;
    jmethodID isDestroying = nullptr;
    jmethodID hitGetType = nullptr;
    jmethodID hitGetBlockPos = nullptr;
    jmethodID ordinal = nullptr;
    jmethodID vec3iGetX = nullptr;
    jmethodID vec3iGetY = nullptr;
    jmethodID vec3iGetZ = nullptr;

    jfieldID level = nullptr;
    jfieldID gameMode = nullptr;
    jfieldID player = nullptr;
    jfieldID hitResult = nullptr;
    jfieldID inventorySelected = nullptr;

    bool ok = false;
};

ToolHandles g_tool;
bool g_toolLogged = false;
bool g_toolResolveFailed = false;
std::chrono::steady_clock::time_point g_toolRetryAt;

constexpr int kResolveRetrySeconds = 10;

bool ResolveTool(JNIEnv* env) {
    if (!env)
        return false;
    if (g_tool.ok)
        return true;

    // A missing member is re-checked on a timer rather than on every call: the
    // layer is asked several times a tick, and a resolve that keeps failing would
    // otherwise walk the class loader just as often.
    if (g_toolResolveFailed && std::chrono::steady_clock::now() < g_toolRetryAt)
        return false;

    jobject loader = GetMinecraftClassLoader(env);
    jclass minecraft = nova_base::loadOrFind(env, loader, kMinecraftDot, kMinecraftSig);
    jclass level = nova_base::loadOrFind(env, loader, kLevelDot, kLevelSig);
    jclass gameMode = nova_base::loadOrFind(env, loader, kGameModeDot, kGameModeSig);
    jclass player = nova_base::loadOrFind(env, loader, kPlayerDot, kPlayerEntitySig);
    jclass inventory = nova_base::loadOrFind(env, loader, kInventoryDot, kInventorySig);
    jclass stack = nova_base::loadOrFind(env, loader, kStackDot, kStackSig);
    jclass blockState = nova_base::loadOrFind(env, loader, kBlockStateDot, kBlockStateSig);
    jclass pos = nova_base::loadOrFind(env, loader, kPosDot, kPosSig);
    jclass hit = nova_base::loadOrFind(env, loader, kHitDot, kHitSig);
    jclass blockHit = nova_base::loadOrFind(env, loader, kBlockHitDot, kBlockHitSig);
    jclass web = nova_base::loadOrFind(env, loader, kWebDot, kWebSig);
    jclass dropXp = nova_base::loadOrFind(env, loader, kDropXpDot, kDropXpSig);
    jclass ore = nova_base::loadOrFind(env, loader, kOreDot, kOreSig);
    // Enum is what ordinal is reached through, so that the hit type does not need
    // its own class object held.
    jclass enumClass = nova_base::loadOrFind(env, loader, kMappedEnumDot, kMappedEnumSig);
    if (loader)
        env->DeleteGlobalRef(loader);

    jmethodID getInstance =
        minecraft ? env->GetStaticMethodID(minecraft, kGetInstance, kGetInstanceDesc) : nullptr;
    jmethodID isDestroying =
        gameMode ? env->GetMethodID(gameMode, kIsDestroying, kBoolDesc) : nullptr;
    jmethodID levelGetState =
        level ? env->GetMethodID(level, kLevelGetState, kLevelGetStateDesc) : nullptr;
    jmethodID stateIsAir =
        blockState ? env->GetMethodID(blockState, kStateIsAir, kBoolDesc) : nullptr;
    jmethodID stateGetBlock =
        blockState ? env->GetMethodID(blockState, kStateGetBlock, kStateGetBlockDesc) : nullptr;
    jmethodID stackDestroySpeed =
        stack ? env->GetMethodID(stack, kStackDestroySpeed, kStackDestroySpeedDesc) : nullptr;
    jmethodID getInventory =
        player ? env->GetMethodID(player, kGetInventory, kGetInventoryDesc) : nullptr;
    jmethodID invGetItem =
        inventory ? env->GetMethodID(inventory, kInvGetItem, kInvGetItemDesc) : nullptr;
    jmethodID hitGetType = hit ? env->GetMethodID(hit, kHitGetType, kHitGetTypeDesc) : nullptr;
    jmethodID hitGetBlockPos =
        blockHit ? env->GetMethodID(blockHit, kHitGetBlockPos, kHitGetBlockPosDesc) : nullptr;
    jmethodID ordinal = enumClass ? env->GetMethodID(enumClass, kOrdinal, kOrdinalDesc) : nullptr;
    jmethodID vec3iGetX = pos ? env->GetMethodID(pos, kVec3iGetX, kIntDesc) : nullptr;
    jmethodID vec3iGetY = pos ? env->GetMethodID(pos, kVec3iGetY, kIntDesc) : nullptr;
    jmethodID vec3iGetZ = pos ? env->GetMethodID(pos, kVec3iGetZ, kIntDesc) : nullptr;
    jmethodID posCtor = pos ? env->GetMethodID(pos, kPosCtor, kPosCtorDesc) : nullptr;

    jfieldID levelField =
        minecraft ? env->GetFieldID(minecraft, kLevelField, kClientLevelSig) : nullptr;
    jfieldID gameModeField =
        minecraft ? env->GetFieldID(minecraft, kGameModeField, kGameModeSig) : nullptr;
    jfieldID playerField =
        minecraft ? env->GetFieldID(minecraft, kPlayerField, kLocalPlayerSig) : nullptr;
    jfieldID hitResultField =
        minecraft ? env->GetFieldID(minecraft, kHitResultField, kHitSig) : nullptr;
    jfieldID inventorySelected =
        inventory ? env->GetFieldID(inventory, kInvSelected, kInvSelectedDesc) : nullptr;
    if (env->ExceptionCheck())
        env->ExceptionClear();

    // Every handle that is later handed to NewObject or IsInstanceOf gates the
    // layer: those two crash on a null class rather than failing the call, and the
    // web and ore classes are only ever used for instance tests.
    const bool ok = minecraft && level && gameMode && player && inventory && stack &&
                    blockState && pos && hit && blockHit && web && dropXp && ore && getInstance &&
                    isDestroying && levelGetState && stateIsAir && stateGetBlock &&
                    stackDestroySpeed && getInventory && invGetItem && hitGetType &&
                    hitGetBlockPos && ordinal && vec3iGetX && vec3iGetY && vec3iGetZ && posCtor &&
                    levelField && gameModeField && playerField && hitResultField &&
                    inventorySelected;

    if (!ok) {
        g_toolResolveFailed = true;
        g_toolRetryAt =
            std::chrono::steady_clock::now() + std::chrono::seconds(kResolveRetrySeconds);
        if (!g_toolLogged) {
            g_toolLogged = true;
            LogTo("Base: tool layer incomplete -> mc=%d level=%d mode=%d player=%d inv=%d "
                  "stack=%d state=%d pos=%d hit=%d blockHit=%d web=%d dropXp=%d ore=%d "
                  "enum=%d getInstance=%d destroying=%d getState=%d isAir=%d getBlock=%d "
                  "speed=%d getInv=%d invItem=%d hitType=%d hitPos=%d ordinal=%d vec3i=%d/%d/%d "
                  "posCtor=%d fields=%d/%d/%d/%d/%d",
                  minecraft != nullptr, level != nullptr, gameMode != nullptr, player != nullptr,
                  inventory != nullptr, stack != nullptr, blockState != nullptr, pos != nullptr,
                  hit != nullptr, blockHit != nullptr, web != nullptr, dropXp != nullptr,
                  ore != nullptr, enumClass != nullptr, getInstance != nullptr,
                  isDestroying != nullptr, levelGetState != nullptr, stateIsAir != nullptr,
                  stateGetBlock != nullptr, stackDestroySpeed != nullptr, getInventory != nullptr,
                  invGetItem != nullptr, hitGetType != nullptr, hitGetBlockPos != nullptr,
                  ordinal != nullptr, vec3iGetX != nullptr, vec3iGetY != nullptr,
                  vec3iGetZ != nullptr, posCtor != nullptr, levelField != nullptr,
                  gameModeField != nullptr, playerField != nullptr, hitResultField != nullptr,
                  inventorySelected != nullptr);
        }
    } else {
        g_tool.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));
        g_tool.pos = static_cast<jclass>(env->NewGlobalRef(pos));
        g_tool.blockState = static_cast<jclass>(env->NewGlobalRef(blockState));
        g_tool.web = static_cast<jclass>(env->NewGlobalRef(web));
        g_tool.dropXp = static_cast<jclass>(env->NewGlobalRef(dropXp));
        g_tool.ore = static_cast<jclass>(env->NewGlobalRef(ore));

        g_tool.getInstance = getInstance;
        g_tool.posCtor = posCtor;
        g_tool.levelGetState = levelGetState;
        g_tool.stateIsAir = stateIsAir;
        g_tool.stateGetBlock = stateGetBlock;
        g_tool.stackDestroySpeed = stackDestroySpeed;
        g_tool.getInventory = getInventory;
        g_tool.invGetItem = invGetItem;
        g_tool.isDestroying = isDestroying;
        g_tool.hitGetType = hitGetType;
        g_tool.hitGetBlockPos = hitGetBlockPos;
        g_tool.ordinal = ordinal;
        g_tool.vec3iGetX = vec3iGetX;
        g_tool.vec3iGetY = vec3iGetY;
        g_tool.vec3iGetZ = vec3iGetZ;

        g_tool.level = levelField;
        g_tool.gameMode = gameModeField;
        g_tool.player = playerField;
        g_tool.hitResult = hitResultField;
        g_tool.inventorySelected = inventorySelected;

        // The classes are what can crash, so the layer is not considered up unless
        // the global references actually took.
        g_tool.ok = g_tool.minecraft && g_tool.pos && g_tool.blockState && g_tool.web &&
                    g_tool.dropXp && g_tool.ore;
    }

    if (minecraft) env->DeleteLocalRef(minecraft);
    if (level) env->DeleteLocalRef(level);
    if (gameMode) env->DeleteLocalRef(gameMode);
    if (player) env->DeleteLocalRef(player);
    if (inventory) env->DeleteLocalRef(inventory);
    if (stack) env->DeleteLocalRef(stack);
    if (blockState) env->DeleteLocalRef(blockState);
    if (pos) env->DeleteLocalRef(pos);
    if (hit) env->DeleteLocalRef(hit);
    if (blockHit) env->DeleteLocalRef(blockHit);
    if (web) env->DeleteLocalRef(web);
    if (dropXp) env->DeleteLocalRef(dropXp);
    if (ore) env->DeleteLocalRef(ore);
    if (enumClass) env->DeleteLocalRef(enumClass);
    return g_tool.ok;
}

bool ToolReady(JNIEnv* env) { return ResolveTool(env); }

jobject ToolClient(JNIEnv* env) {
    jobject instance = env->CallStaticObjectMethod(g_tool.minecraft, g_tool.getInstance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return instance;
}

// The player's inventory, as a local reference the caller releases. Kept here
// rather than borrowed from the scaffold layer so that a missing scaffold member
// cannot stop the hotbar being read.
jobject ToolInventory(JNIEnv* env) {
    jobject client = ToolClient(env);
    if (client == nullptr)
        return nullptr;

    jobject player = env->GetObjectField(client, g_tool.player);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    if (player == nullptr)
        return nullptr;

    jobject inventory = env->CallObjectMethod(player, g_tool.getInventory);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return inventory;
}

}  // namespace

bool GameModeDestroying() {
    if (compat::UsesModernRuntime()) return compat::mc263::GameModeDestroying();
    if (compat::IsLegacyLunar()) return compat::lunar::GameModeDestroying();
    JniAttach attach;
    if (!attach || !ToolReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject client = ToolClient(env);
    if (client == nullptr)
        return false;

    jobject gameMode = env->GetObjectField(client, g_tool.gameMode);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (gameMode == nullptr)
        return false;

    const jboolean destroying = env->CallBooleanMethod(gameMode, g_tool.isDestroying);
    env->DeleteLocalRef(gameMode);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return destroying == JNI_TRUE;
}

bool HitBlockPos(BlockPos& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::HitBlockPos(out);
    if (compat::IsLegacyLunar()) return compat::lunar::HitBlockPos(out);
    JniAttach attach;
    if (!attach || !ToolReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject client = ToolClient(env);
    if (client == nullptr)
        return false;

    jobject hit = env->GetObjectField(client, g_tool.hitResult);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (hit == nullptr)
        return false;

    jobject type = env->CallObjectMethod(hit, g_tool.hitGetType);
    if (env->ExceptionCheck() || type == nullptr) {
        env->ExceptionClear();
        if (type) env->DeleteLocalRef(type);
        env->DeleteLocalRef(hit);
        return false;
    }
    const jint kind = env->CallIntMethod(type, g_tool.ordinal);
    env->DeleteLocalRef(type);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(hit);
        return false;
    }
    if (kind != kHitTypeBlock) {
        env->DeleteLocalRef(hit);
        return false;
    }

    jobject blockPos = env->CallObjectMethod(hit, g_tool.hitGetBlockPos);
    env->DeleteLocalRef(hit);
    if (env->ExceptionCheck() || blockPos == nullptr) {
        env->ExceptionClear();
        if (blockPos) env->DeleteLocalRef(blockPos);
        return false;
    }

    const jint x = env->CallIntMethod(blockPos, g_tool.vec3iGetX);
    const jint y = env->CallIntMethod(blockPos, g_tool.vec3iGetY);
    const jint z = env->CallIntMethod(blockPos, g_tool.vec3iGetZ);
    env->DeleteLocalRef(blockPos);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }

    out = BlockPos{static_cast<int>(x), static_cast<int>(y), static_cast<int>(z)};
    return true;
}

bool SelectedHotbarSlot(int& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::SelectedSlot(out);
    if (compat::IsLegacyLunar()) return compat::lunar::SelectedSlot(out);
    out = -1;
    JniAttach attach;
    if (!attach || !ToolReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject inventory = ToolInventory(env);
    if (inventory == nullptr)
        return false;

    const jint slot = env->GetIntField(inventory, g_tool.inventorySelected);
    env->DeleteLocalRef(inventory);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    out = static_cast<int>(slot);
    return true;
}

bool SetSelectedHotbarSlot(int value) {
    if (compat::UsesModernRuntime()) return compat::mc263::SetSelectedSlot(value);
    if (compat::IsLegacyLunar()) return compat::lunar::SetSelectedSlot(value);
    JniAttach attach;
    if (!attach || !ToolReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject inventory = ToolInventory(env);
    if (inventory == nullptr)
        return false;

    env->SetIntField(inventory, g_tool.inventorySelected, static_cast<jint>(value));
    env->DeleteLocalRef(inventory);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

bool BestToolSlotFor(const BlockPos& pos, int& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::BestToolSlotFor(pos, out);
    if (compat::IsLegacyLunar()) return compat::lunar::BestToolSlotFor(pos, out);
    out = -1;
    JniAttach attach;
    if (!attach || !ToolReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject client = ToolClient(env);
    if (client == nullptr)
        return false;

    jobject level = env->GetObjectField(client, g_tool.level);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (level == nullptr)
        return false;

    jobject inventory = ToolInventory(env);
    if (inventory == nullptr) {
        env->DeleteLocalRef(level);
        return false;
    }

    jobject blockPos = env->NewObject(g_tool.pos, g_tool.posCtor, static_cast<jint>(pos.x),
                                      static_cast<jint>(pos.y), static_cast<jint>(pos.z));
    if (env->ExceptionCheck() || blockPos == nullptr) {
        env->ExceptionClear();
        if (blockPos) env->DeleteLocalRef(blockPos);
        env->DeleteLocalRef(level);
        env->DeleteLocalRef(inventory);
        return false;
    }

    jobject state = env->CallObjectMethod(level, g_tool.levelGetState, blockPos);
    env->DeleteLocalRef(blockPos);
    env->DeleteLocalRef(level);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        state = nullptr;
    }
    if (state == nullptr) {
        env->DeleteLocalRef(inventory);
        return false;
    }

    const jboolean air = env->CallBooleanMethod(state, g_tool.stateIsAir);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(state);
        env->DeleteLocalRef(inventory);
        return false;
    }
    if (air == JNI_TRUE) {
        env->DeleteLocalRef(state);
        env->DeleteLocalRef(inventory);
        return true;
    }

    jobject block = env->CallObjectMethod(state, g_tool.stateGetBlock);
    if (env->ExceptionCheck() || block == nullptr) {
        env->ExceptionClear();
        if (block) env->DeleteLocalRef(block);
        env->DeleteLocalRef(state);
        env->DeleteLocalRef(inventory);
        return false;
    }

    // Two class tests rather than held block constants: the original asks
    // `instanceof`, and a redstone ore is not the same block object a player would
    // have placed.
    const bool isWeb = env->IsInstanceOf(block, g_tool.web) == JNI_TRUE;
    const bool skipEfficiency = env->IsInstanceOf(block, g_tool.dropXp) == JNI_TRUE ||
                                env->IsInstanceOf(block, g_tool.ore) == JNI_TRUE;
    env->DeleteLocalRef(block);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    int bestSlot = -1;
    float best = 1.0f;

    for (int index = 0; index < 9; ++index) {
        jobject stack =
            env->CallObjectMethod(inventory, g_tool.invGetItem, static_cast<jint>(index));
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            stack = nullptr;
        }
        if (stack == nullptr)
            continue;

        // The original's own order: the god-item test comes first and runs for
        // every stack, so the batch read is what pays for it here. That same read
        // also carries the efficiency level and the sword test, which the original
        // would have to ask for separately.
        ItemFacts facts;
        ReadItemFacts(stack, facts);

        const bool considered = !nova::inventory::IsGodItem(facts) && !facts.empty &&
                                ((facts.kinds & kItemSword) == 0 || isWeb);
        if (considered) {
            const jfloat speed = env->CallFloatMethod(stack, g_tool.stackDestroySpeed, state);
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
            } else {
                float score = static_cast<float>(speed);
                if (score > 1.0f && !skipEfficiency && facts.efficiency > 0) {
                    score += static_cast<float>(facts.efficiency * facts.efficiency + 1);
                }
                if (score > best) {
                    best = score;
                    bestSlot = index;
                }
            }
        }

        env->DeleteLocalRef(stack);
    }

    env->DeleteLocalRef(state);
    env->DeleteLocalRef(inventory);

    out = best > 1.0f ? bestSlot : -1;
    return true;
}

}
