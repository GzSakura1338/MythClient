#include "minecraft/java/api/game_click.h"
#include "minecraft/utils/rotation/RotationManager.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"

#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/jni_lookup.h"

namespace nova::java {
namespace {

#define NOVA_FORGE1201_CLICK
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_CLICK

constexpr jint kMouseButtonPickup = 0;
constexpr int  kLocalCount = 16;

struct TargetHandles {
    jclass    minecraft = nullptr;
    jmethodID getInstance = nullptr;
    jfieldID  level = nullptr;
    jfieldID  gameMode = nullptr;
    jfieldID  player = nullptr;
    jfieldID  screen = nullptr;
    jfieldID  hitResult = nullptr;
    jfieldID  missTime = nullptr;
    jmethodID getType = nullptr;
    jobject   blockType = nullptr;
    jmethodID getBlockPos = nullptr;
    jmethodID getBlockState = nullptr;
    jmethodID isAir = nullptr;
    jmethodID getBlock = nullptr;
    jclass    liquidBlock = nullptr;
    jclass    entityHit = nullptr;
    jclass    living = nullptr;
    jmethodID getEntity = nullptr;
    jmethodID attack = nullptr;
    jmethodID swing = nullptr;
    jobject   mainHand = nullptr;
    jclass    containerScreen = nullptr;
    jfieldID  hoveredSlot = nullptr;
    jfieldID  menu = nullptr;
    jfieldID  containerId = nullptr;
    jfieldID  slots = nullptr;
    jmethodID indexOf = nullptr;
    jobject   pickup = nullptr;
    jobject   quickMove = nullptr;
    jmethodID handleClick = nullptr;
};

TargetHandles g_target;
bool g_targetLogged = false;

void ReleaseAll(JNIEnv* env, jclass (&locals)[kLocalCount]) {
    for (int i = 0; i < kLocalCount; ++i) {
        if (locals[i])
            env->DeleteLocalRef(locals[i]);
    }
}

bool Resolve(JNIEnv* env) {
    jobject loader = GetMinecraftClassLoader(env);
    jclass locals[kLocalCount] = {
        nova_base::loadOrFind(env, loader, kMinecraftDot, kMinecraftSig),
        nova_base::loadOrFind(env, loader, kLevelDot, kLevelSig),
        nova_base::loadOrFind(env, loader, kGameModeDot, kGameModeSig),
        nova_base::loadOrFind(env, loader, kScreenDot, kScreenSig),
        nova_base::loadOrFind(env, loader, kContainerDot, kContainerSig),
        nova_base::loadOrFind(env, loader, kMenuDot, kMenuSig),
        nova_base::loadOrFind(env, loader, kClickTypeDot, kClickTypeSig),
        nova_base::loadOrFind(env, loader, kHitResultDot, kHitResultSig),
        nova_base::loadOrFind(env, loader, kHitTypeDot, kHitTypeSig),
        nova_base::loadOrFind(env, loader, kBlockHitDot, kBlockHitSig),
        nova_base::loadOrFind(env, loader, kStateDot, kStateSig),
        nova_base::loadOrFind(env, loader, kLiquidDot, kLiquidSig),
        nullptr,
        nova_base::loadOrFind(env, loader, kEntityHitDot, kEntityHitSig),
        nova_base::loadOrFind(env, loader, kLivingDot, kLivingSig),
        nova_base::loadOrFind(env, loader, kHandDot, kHandSig),
    };
    if (loader)
        env->DeleteGlobalRef(loader);

    locals[12] = env->FindClass(kListDot);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    for (int i = 0; i < kLocalCount; ++i) {
        if (!locals[i]) {
            ReleaseAll(env, locals);
            return false;
        }
    }

    jclass minecraft = locals[0];
    jclass level = locals[1];
    jclass gameMode = locals[2];
    jclass container = locals[4];
    jclass menu = locals[5];
    jclass clickType = locals[6];
    jclass hitResult = locals[7];
    jclass hitType = locals[8];
    jclass blockHit = locals[9];
    jclass state = locals[10];
    jclass liquid = locals[11];
    jclass list = locals[12];
    jclass entityHit = locals[13];
    jclass living = locals[14];
    jclass hand = locals[15];

    jmethodID getInstance = env->GetStaticMethodID(minecraft, kGetInstance, kGetInstanceDesc);
    jfieldID levelField = env->GetFieldID(minecraft, kLevelField, kLevelFieldSig);
    jfieldID gameModeField = env->GetFieldID(minecraft, kGameModeField, kGameModeSig);
    jfieldID playerField = env->GetFieldID(minecraft, kPlayerField, kLocalPlayerSig);
    jfieldID screenField = env->GetFieldID(minecraft, kScreenField, kScreenSig);
    jfieldID hitResultField = env->GetFieldID(minecraft, kHitResultField, kHitResultSig);
    jfieldID missTimeField = env->GetFieldID(minecraft, kMissTimeField, kMappedPrimitiveIntSig);
    jmethodID getType = env->GetMethodID(hitResult, kGetType, kGetTypeDesc);
    jmethodID getBlockPos = env->GetMethodID(blockHit, kGetBlockPos, kGetBlockPosDesc);
    jmethodID getBlockState = env->GetMethodID(level, kGetBlockState, kGetBlockStateDesc);
    jmethodID isAir = env->GetMethodID(state, kIsAir, kIsAirDesc);
    jmethodID getBlock = env->GetMethodID(state, kGetBlock, kGetBlockDesc);
    jfieldID hoveredSlot = env->GetFieldID(container, kHoveredSlot, kSlotSig);
    jfieldID menuField = env->GetFieldID(container, kMenuField, kMenuSig);
    jfieldID containerId = env->GetFieldID(menu, kContainerId, kMappedPrimitiveIntSig);
    jfieldID slots = env->GetFieldID(menu, kSlotsField, kSlotListSig);
    jmethodID indexOf = env->GetMethodID(list, kIndexOf, kIndexOfDesc);
    jfieldID pickupField = env->GetStaticFieldID(clickType, kPickup, kClickTypeSig);
    jfieldID quickMoveField = env->GetStaticFieldID(clickType, kQuickMove, kClickTypeSig);
    jmethodID handleClick = env->GetMethodID(gameMode, kHandleClick, kHandleClickDesc);
    jfieldID blockTypeField = env->GetStaticFieldID(hitType, kBlockType, kHitTypeSig);
    jmethodID getEntity = env->GetMethodID(entityHit, kGetEntity, kGetEntityDesc);
    jmethodID attack = env->GetMethodID(gameMode, kAttack, kAttackDesc);
    jmethodID swing = env->GetMethodID(living, kSwing, kSwingDesc);
    jfieldID mainHandField = env->GetStaticFieldID(hand, kMainHand, kHandSig);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    jobject blockType = blockTypeField
        ? env->GetStaticObjectField(hitType, blockTypeField) : nullptr;
    jobject pickup = pickupField ? env->GetStaticObjectField(clickType, pickupField) : nullptr;
    jobject quickMove = quickMoveField
        ? env->GetStaticObjectField(clickType, quickMoveField) : nullptr;
    jobject mainHand = mainHandField
        ? env->GetStaticObjectField(hand, mainHandField) : nullptr;
    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (!getInstance || !levelField || !gameModeField || !playerField || !screenField ||
        !hitResultField || !missTimeField || !getType || !getBlockPos || !getBlockState ||
        !isAir || !getBlock || !hoveredSlot || !menuField || !containerId || !slots ||
        !indexOf || !handleClick || !blockType || !pickup || !quickMove || !getEntity ||
        !attack || !swing || !mainHand) {
        if (!g_targetLogged) {
            g_targetLogged = true;
            LogTo("Base: click targets unavailable (getInstance=%d level=%d gameMode=%d "
                  "player=%d screen=%d hit=%d missTime=%d type=%d blockPos=%d state=%d air=%d "
                  "block=%d hovered=%d menu=%d container=%d slots=%d index=%d click=%d "
                  "blockType=%d pickup=%d quickMove=%d getEntity=%d attack=%d swing=%d "
                  "mainHand=%d)",
                  getInstance != nullptr, levelField != nullptr, gameModeField != nullptr,
                  playerField != nullptr, screenField != nullptr, hitResultField != nullptr,
                  missTimeField != nullptr, getType != nullptr, getBlockPos != nullptr,
                  getBlockState != nullptr, isAir != nullptr, getBlock != nullptr,
                  hoveredSlot != nullptr, menuField != nullptr, containerId != nullptr,
                  slots != nullptr, indexOf != nullptr, handleClick != nullptr,
                  blockType != nullptr, pickup != nullptr, quickMove != nullptr,
                  getEntity != nullptr, attack != nullptr, swing != nullptr,
                  mainHand != nullptr);
        }
        if (blockType) env->DeleteLocalRef(blockType);
        if (pickup) env->DeleteLocalRef(pickup);
        if (quickMove) env->DeleteLocalRef(quickMove);
        if (mainHand) env->DeleteLocalRef(mainHand);
        ReleaseAll(env, locals);
        return false;
    }

    g_target.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));
    g_target.liquidBlock = static_cast<jclass>(env->NewGlobalRef(liquid));
    g_target.containerScreen = static_cast<jclass>(env->NewGlobalRef(container));
    g_target.entityHit = static_cast<jclass>(env->NewGlobalRef(entityHit));
    g_target.living = static_cast<jclass>(env->NewGlobalRef(living));
    g_target.blockType = env->NewGlobalRef(blockType);
    g_target.pickup = env->NewGlobalRef(pickup);
    g_target.quickMove = env->NewGlobalRef(quickMove);
    g_target.mainHand = env->NewGlobalRef(mainHand);
    g_target.getEntity = getEntity;
    g_target.attack = attack;
    g_target.swing = swing;
    g_target.getInstance = getInstance;
    g_target.level = levelField;
    g_target.gameMode = gameModeField;
    g_target.player = playerField;
    g_target.screen = screenField;
    g_target.hitResult = hitResultField;
    g_target.missTime = missTimeField;
    g_target.getType = getType;
    g_target.getBlockPos = getBlockPos;
    g_target.getBlockState = getBlockState;
    g_target.isAir = isAir;
    g_target.getBlock = getBlock;
    g_target.hoveredSlot = hoveredSlot;
    g_target.menu = menuField;
    g_target.containerId = containerId;
    g_target.slots = slots;
    g_target.indexOf = indexOf;
    g_target.handleClick = handleClick;

    if (blockType) env->DeleteLocalRef(blockType);
    if (pickup) env->DeleteLocalRef(pickup);
    if (quickMove) env->DeleteLocalRef(quickMove);
    if (mainHand) env->DeleteLocalRef(mainHand);
    ReleaseAll(env, locals);

    return g_target.minecraft && g_target.liquidBlock && g_target.containerScreen &&
           g_target.entityHit && g_target.living && g_target.blockType && g_target.pickup &&
           g_target.quickMove && g_target.mainHand;
}

jobject Client(JNIEnv* env) {
    if (g_target.minecraft == nullptr)
        return nullptr;
    return env->CallStaticObjectMethod(g_target.minecraft, g_target.getInstance);
}

bool TargetTypeIs(JNIEnv* env, jobject client, jobject expected) {
    jobject hit = env->GetObjectField(client, g_target.hitResult);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (hit == nullptr)
        return false;

    jobject type = env->CallObjectMethod(hit, g_target.getType);
    env->DeleteLocalRef(hit);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (type == nullptr)
        return false;

    const bool same = env->IsSameObject(type, expected) == JNI_TRUE;
    env->DeleteLocalRef(type);
    return same;
}

bool BlockTarget(JNIEnv* env, jobject client) {
    return TargetTypeIs(env, client, g_target.blockType);
}

}

bool ScreenOpen() {
    if (!compat::Has(compat::Capability::Forge1201Events)) return compat::ScreenOpen();
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_target.minecraft && !Resolve(env))
        return false;

    jobject client = Client(env);
    if (env->ExceptionCheck() || client == nullptr) {
        env->ExceptionClear();
        return false;
    }

    jobject screen = env->GetObjectField(client, g_target.screen);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (screen == nullptr)
        return false;

    env->DeleteLocalRef(screen);
    return true;
}

bool AttackTargetIsBlock() {
    if (compat::UsesModernRuntime()) return compat::mc263::AttackTargetIsBlock();
    if (compat::IsLegacyLunar()) return compat::lunar::AttackTargetIsBlock();
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_target.minecraft && !Resolve(env))
        return false;

    jobject client = Client(env);
    if (env->ExceptionCheck() || client == nullptr) {
        env->ExceptionClear();
        return false;
    }

    const bool block = BlockTarget(env, client);
    env->DeleteLocalRef(client);
    return block;
}

bool ClearMissTime() {
    if (compat::UsesModernRuntime()) return compat::mc263::ClearMissTime();
    if (compat::IsLegacyLunar()) return compat::lunar::ClearMissTime();
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_target.minecraft && !Resolve(env))
        return false;

    jobject client = Client(env);
    if (env->ExceptionCheck() || client == nullptr) {
        env->ExceptionClear();
        return false;
    }

    env->SetIntField(client, g_target.missTime, 0);
    const bool clean = !env->ExceptionCheck();
    if (env->ExceptionCheck())
        env->ExceptionClear();
    env->DeleteLocalRef(client);
    return clean;
}

bool TargetedBlockBreakable() {
    if (compat::UsesModernRuntime()) return compat::mc263::TargetedBlockBreakable();
    if (compat::IsLegacyLunar()) return compat::lunar::TargetedBlockBreakable();
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_target.minecraft && !Resolve(env))
        return false;

    jobject client = Client(env);
    if (env->ExceptionCheck() || client == nullptr) {
        env->ExceptionClear();
        return false;
    }

    bool breakable = false;
    if (BlockTarget(env, client)) {
        jobject hit = env->GetObjectField(client, g_target.hitResult);
        jobject level = env->GetObjectField(client, g_target.level);
        jobject pos = hit ? env->CallObjectMethod(hit, g_target.getBlockPos) : nullptr;
        jobject state = (level && pos)
            ? env->CallObjectMethod(level, g_target.getBlockState, pos) : nullptr;
        if (env->ExceptionCheck())
            env->ExceptionClear();

        if (state) {
            const jboolean air = env->CallBooleanMethod(state, g_target.isAir);
            if (env->ExceptionCheck())
                env->ExceptionClear();
            if (air == JNI_FALSE) {
                jobject block = env->CallObjectMethod(state, g_target.getBlock);
                if (env->ExceptionCheck())
                    env->ExceptionClear();
                if (block) {
                    breakable = env->IsInstanceOf(block, g_target.liquidBlock) == JNI_FALSE;
                    env->DeleteLocalRef(block);
                }
            }
        }

        if (state) env->DeleteLocalRef(state);
        if (pos) env->DeleteLocalRef(pos);
        if (level) env->DeleteLocalRef(level);
        if (hit) env->DeleteLocalRef(hit);
    }

    env->DeleteLocalRef(client);
    return breakable;
}

bool InventoryScreenOpen() {
    if (compat::UsesModernRuntime()) return compat::mc263::InventoryScreenOpen();
    if (compat::IsLegacyLunar()) return compat::lunar::InventoryScreenOpen();
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_target.minecraft && !Resolve(env))
        return false;

    jobject client = Client(env);
    if (env->ExceptionCheck() || client == nullptr) {
        env->ExceptionClear();
        return false;
    }

    jobject screen = env->GetObjectField(client, g_target.screen);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (screen == nullptr)
        return false;

    const bool open = env->IsInstanceOf(screen, g_target.containerScreen) == JNI_TRUE;
    env->DeleteLocalRef(screen);
    return open;
}

bool ClickHoveredSlot(bool quickMove) {
    if (compat::UsesModernRuntime()) return compat::mc263::ClickHoveredSlot(quickMove);
    if (compat::IsLegacyLunar()) return compat::lunar::ClickHoveredSlot(quickMove);
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_target.minecraft && !Resolve(env))
        return false;

    jobject client = Client(env);
    if (env->ExceptionCheck() || client == nullptr) {
        env->ExceptionClear();
        return false;
    }

    jobject gameMode = env->GetObjectField(client, g_target.gameMode);
    jobject player = env->GetObjectField(client, g_target.player);
    jobject screen = env->GetObjectField(client, g_target.screen);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    jobject slot = screen ? env->GetObjectField(screen, g_target.hoveredSlot) : nullptr;
    jobject menu = screen ? env->GetObjectField(screen, g_target.menu) : nullptr;
    jobject slots = menu ? env->GetObjectField(menu, g_target.slots) : nullptr;
    if (env->ExceptionCheck())
        env->ExceptionClear();

    bool clicked = false;
    if (gameMode && player && slot && menu && slots) {
        const jint containerId = env->GetIntField(menu, g_target.containerId);
        const jint slotId = env->CallIntMethod(slots, g_target.indexOf, slot);
        if (env->ExceptionCheck())
            env->ExceptionClear();

        if (slotId >= 0) {
            env->CallVoidMethod(gameMode, g_target.handleClick, containerId, slotId,
                                kMouseButtonPickup,
                                quickMove ? g_target.quickMove : g_target.pickup, player);
            clicked = !env->ExceptionCheck();
            if (env->ExceptionCheck())
                env->ExceptionClear();
        }
    }

    if (slots) env->DeleteLocalRef(slots);
    if (menu) env->DeleteLocalRef(menu);
    if (slot) env->DeleteLocalRef(slot);
    if (screen) env->DeleteLocalRef(screen);
    if (player) env->DeleteLocalRef(player);
    if (gameMode) env->DeleteLocalRef(gameMode);
    return clicked;
}

bool AttackTargetEntity(EntityHandle& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::AttackTargetEntity(out);
    if (compat::IsLegacyLunar()) return compat::lunar::AttackTargetEntity(out);
    out.Reset();

    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_target.minecraft && !Resolve(env))
        return false;

    jobject client = Client(env);
    if (env->ExceptionCheck() || client == nullptr) {
        env->ExceptionClear();
        return false;
    }

    jobject hit = env->GetObjectField(client, g_target.hitResult);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (hit == nullptr)
        return false;

    if (env->IsInstanceOf(hit, g_target.entityHit) != JNI_TRUE) {
        if (env->ExceptionCheck())
            env->ExceptionClear();
        env->DeleteLocalRef(hit);
        return false;
    }

    jobject entity = env->CallObjectMethod(hit, g_target.getEntity);
    env->DeleteLocalRef(hit);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (entity == nullptr)
        return false;

    out = AdoptEntity(entity);
    env->DeleteLocalRef(entity);
    return out.Valid();
}

bool AttackEntity(const EntityHandle& target) {
    if (!rotation::CanCallNativeAttack()) return false;
    if (FreeCamBlocks(FreeCamAction::Interact)) return false;
    if (compat::UsesModernRuntime()) return compat::mc263::AttackEntity(target);
    if (compat::IsLegacyLunar()) return compat::lunar::AttackEntity(target);
    if (!target.Valid())
        return false;

    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_target.minecraft && !Resolve(env))
        return false;

    jobject client = Client(env);
    if (env->ExceptionCheck() || client == nullptr) {
        env->ExceptionClear();
        return false;
    }

    jobject gameMode = env->GetObjectField(client, g_target.gameMode);
    jobject player = env->GetObjectField(client, g_target.player);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    bool attacked = false;
    if (gameMode && player) {
        env->CallVoidMethod(gameMode, g_target.attack, player,
                            static_cast<jobject>(target.raw()));
        attacked = !env->ExceptionCheck();
        if (env->ExceptionCheck())
            env->ExceptionClear();
    }

    if (player) env->DeleteLocalRef(player);
    if (gameMode) env->DeleteLocalRef(gameMode);
    return attacked;
}

bool AttackAndSwing(const EntityHandle& target) {
    if (FreeCamBlocks(FreeCamAction::Interact)) return false;
    if (!target.Valid()) return false;
    if (compat::IsLegacyLunar()) {
        if (!SwingHand()) return false;
        return AttackEntity(target);
    }
    if (!AttackEntity(target)) return false;
    return SwingHand();
}

bool SwingHand() {
    if (compat::UsesModernRuntime()) return compat::mc263::SwingHand();
    if (compat::IsLegacyLunar()) return compat::lunar::SwingHand();
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_target.minecraft && !Resolve(env))
        return false;

    jobject client = Client(env);
    if (env->ExceptionCheck() || client == nullptr) {
        env->ExceptionClear();
        return false;
    }

    jobject player = env->GetObjectField(client, g_target.player);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (player == nullptr)
        return false;

    env->CallVoidMethod(player, g_target.swing, g_target.mainHand);
    const bool clean = !env->ExceptionCheck();
    if (env->ExceptionCheck())
        env->ExceptionClear();
    env->DeleteLocalRef(player);
    return clean;
}

}
