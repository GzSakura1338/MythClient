
#include "minecraft/java/api/game_player.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"

#include "minecraft/java/api/base.h"
#include "minecraft/java/api/jni_lookup.h"

#include <cmath>

namespace nova::java {
namespace {

#define NOVA_FORGE1201_PLAYER
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_PLAYER

constexpr jint kEffectDuration = 1337;

#define NOVA_JUMP_DELAY_MAPPING
constexpr compat::mc263::access::Member kLunarJumpDelay
#include "minecraft/compat/mappings/Lunar189.inc"
;
constexpr compat::mc263::access::Member kForgeJumpDelay
#include "minecraft/compat/mappings/forge1201.inc"
;
constexpr compat::mc263::access::Member kModernJumpDelay
#include "minecraft/compat/mappings/minecraft263.inc"
;
#undef NOVA_JUMP_DELAY_MAPPING

struct PlayerHandles {
    jclass    minecraft = nullptr;
    jmethodID getInstance = nullptr;
    jfieldID  player = nullptr;
    jmethodID addEffect = nullptr;
    jmethodID removeEffect = nullptr;
    jmethodID onGround = nullptr;
    jclass    instanceClass = nullptr;
    jmethodID instanceCtor = nullptr;
    jobject   nightVision = nullptr;
};

PlayerHandles g_player;
bool g_playerLogged = false;

bool Resolve(JNIEnv* env) {
    jobject loader = GetMinecraftClassLoader(env);
    jclass minecraft = nova_base::loadOrFind(env, loader, kMinecraftDot, kMinecraftSig);
    jclass living = nova_base::loadOrFind(env, loader, kLivingDot, kLivingSig);
    jclass effects = nova_base::loadOrFind(env, loader, kEffectsDot, kEffectsSig);
    jclass instance = nova_base::loadOrFind(env, loader, kInstanceDot, kInstanceSig);
    if (loader)
        env->DeleteGlobalRef(loader);

    if (!minecraft || !living || !effects || !instance) {
        if (minecraft) env->DeleteLocalRef(minecraft);
        if (living) env->DeleteLocalRef(living);
        if (effects) env->DeleteLocalRef(effects);
        if (instance) env->DeleteLocalRef(instance);
        return false;
    }

    jmethodID getInstance = env->GetStaticMethodID(minecraft, kGetInstance, kGetInstanceDesc);
    jfieldID playerField = env->GetFieldID(minecraft, kPlayerField, kPlayerSig);
    jmethodID addEffect = env->GetMethodID(living, kAddEffect, kAddEffectDesc);
    jmethodID removeEffect = env->GetMethodID(living, kRemoveEffect, kRemoveEffectDesc);
    jmethodID onGround = env->GetMethodID(living, kOnGround, kOnGroundDesc);
    jmethodID ctor = env->GetMethodID(instance, kInstanceCtor, kInstanceCtorDesc);
    jfieldID nightVision = env->GetStaticFieldID(effects, kNightVision, kEffectSig);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    jobject effect = nightVision ? env->GetStaticObjectField(effects, nightVision) : nullptr;
    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (!getInstance || !playerField || !addEffect || !removeEffect || !onGround ||
        !ctor || !effect) {
        if (!g_playerLogged) {
            g_playerLogged = true;
            LogTo("Base: player effects unavailable (getInstance=%d, player=%d, add=%d, "
                  "remove=%d, onGround=%d, ctor=%d, nightVision=%d)",
                  getInstance != nullptr, playerField != nullptr, addEffect != nullptr,
                  removeEffect != nullptr, onGround != nullptr, ctor != nullptr,
                  effect != nullptr);
        }
        if (effect) env->DeleteLocalRef(effect);
        env->DeleteLocalRef(minecraft);
        env->DeleteLocalRef(living);
        env->DeleteLocalRef(effects);
        env->DeleteLocalRef(instance);
        return false;
    }

    g_player.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));
    g_player.instanceClass = static_cast<jclass>(env->NewGlobalRef(instance));
    g_player.nightVision = env->NewGlobalRef(effect);
    g_player.getInstance = getInstance;
    g_player.player = playerField;
    g_player.addEffect = addEffect;
    g_player.removeEffect = removeEffect;
    g_player.onGround = onGround;
    g_player.instanceCtor = ctor;

    env->DeleteLocalRef(effect);
    env->DeleteLocalRef(minecraft);
    env->DeleteLocalRef(living);
    env->DeleteLocalRef(effects);
    env->DeleteLocalRef(instance);
    return g_player.minecraft && g_player.instanceClass && g_player.nightVision;
}

struct PredicateHandles {
    jclass    minecraft = nullptr;
    jmethodID getInstance = nullptr;
    jfieldID  player = nullptr;
    jmethodID isUsingItem = nullptr;
    jmethodID getAbilities = nullptr;
    jmethodID getMainHandItem = nullptr;
    jmethodID getItem = nullptr;
    jfieldID  instabuild = nullptr;
    jfieldID  mayBuild = nullptr;
    jclass    sword = nullptr;
    jclass    axe = nullptr;
    jclass    trident = nullptr;
};

PredicateHandles g_predicates;
bool g_predicatesLogged = false;

bool ResolvePredicates(JNIEnv* env) {
    jobject loader = GetMinecraftClassLoader(env);
    jclass minecraft = nova_base::loadOrFind(env, loader, kMinecraftDot, kMinecraftSig);
    jclass living = nova_base::loadOrFind(env, loader, kLivingDot, kLivingSig);
    jclass player = nova_base::loadOrFind(env, loader, kPlayerDot, kPlayerTypeSig);
    jclass abilities = nova_base::loadOrFind(env, loader, kAbilitiesDot, kAbilitiesSig);
    jclass itemStack = nova_base::loadOrFind(env, loader, kItemStackDot, kItemStackSig);
    jclass sword = nova_base::loadOrFind(env, loader, kSwordDot, kSwordSig);
    jclass axe = nova_base::loadOrFind(env, loader, kAxeDot, kAxeSig);
    jclass trident = nova_base::loadOrFind(env, loader, kTridentDot, kTridentSig);
    if (loader)
        env->DeleteGlobalRef(loader);

    if (!minecraft || !living || !player || !abilities || !itemStack || !sword || !axe ||
        !trident) {
        if (minecraft) env->DeleteLocalRef(minecraft);
        if (living) env->DeleteLocalRef(living);
        if (player) env->DeleteLocalRef(player);
        if (abilities) env->DeleteLocalRef(abilities);
        if (itemStack) env->DeleteLocalRef(itemStack);
        if (sword) env->DeleteLocalRef(sword);
        if (axe) env->DeleteLocalRef(axe);
        if (trident) env->DeleteLocalRef(trident);
        return false;
    }

    jmethodID getInstance = env->GetStaticMethodID(minecraft, kGetInstance, kGetInstanceDesc);
    jfieldID playerField = env->GetFieldID(minecraft, kPlayerField, kPlayerSig);
    jmethodID isUsingItem = env->GetMethodID(living, kIsUsingItem, kOnGroundDesc);
    jmethodID getAbilities = env->GetMethodID(player, kGetAbilities, kGetAbilitiesDesc);
    jmethodID getMainHandItem = env->GetMethodID(living, kGetMainHandItem, kGetMainHandDesc);
    jmethodID getItem = env->GetMethodID(itemStack, kGetItem, kGetItemDesc);
    jfieldID instabuild = env->GetFieldID(abilities, kInstabuild, kMappedPrimitiveBooleanSig);
    jfieldID mayBuild = env->GetFieldID(abilities, kMayBuild, kMappedPrimitiveBooleanSig);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (!getInstance || !playerField || !isUsingItem || !getAbilities || !getMainHandItem ||
        !getItem || !instabuild || !mayBuild) {
        if (!g_predicatesLogged) {
            g_predicatesLogged = true;
            LogTo("Base: player predicates unavailable (getInstance=%d, player=%d, usingItem=%d, "
                  "abilities=%d, mainHand=%d, item=%d, instabuild=%d, mayBuild=%d)",
                  getInstance != nullptr, playerField != nullptr, isUsingItem != nullptr,
                  getAbilities != nullptr, getMainHandItem != nullptr, getItem != nullptr,
                  instabuild != nullptr, mayBuild != nullptr);
        }
        env->DeleteLocalRef(minecraft);
        env->DeleteLocalRef(living);
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(abilities);
        env->DeleteLocalRef(itemStack);
        env->DeleteLocalRef(sword);
        env->DeleteLocalRef(axe);
        env->DeleteLocalRef(trident);
        return false;
    }

    g_predicates.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));
    g_predicates.sword = static_cast<jclass>(env->NewGlobalRef(sword));
    g_predicates.axe = static_cast<jclass>(env->NewGlobalRef(axe));
    g_predicates.trident = static_cast<jclass>(env->NewGlobalRef(trident));
    g_predicates.getInstance = getInstance;
    g_predicates.player = playerField;
    g_predicates.isUsingItem = isUsingItem;
    g_predicates.getAbilities = getAbilities;
    g_predicates.getMainHandItem = getMainHandItem;
    g_predicates.getItem = getItem;
    g_predicates.instabuild = instabuild;
    g_predicates.mayBuild = mayBuild;

    env->DeleteLocalRef(minecraft);
    env->DeleteLocalRef(living);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(abilities);
    env->DeleteLocalRef(itemStack);
    env->DeleteLocalRef(sword);
    env->DeleteLocalRef(axe);
    env->DeleteLocalRef(trident);
    return g_predicates.minecraft && g_predicates.sword && g_predicates.axe &&
           g_predicates.trident;
}

jobject PredicatePlayer(JNIEnv* env) {
    if (g_predicates.minecraft == nullptr)
        return nullptr;
    jobject instance = env->CallStaticObjectMethod(g_predicates.minecraft,
                                                   g_predicates.getInstance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    if (instance == nullptr)
        return nullptr;

    jobject player = env->GetObjectField(instance, g_predicates.player);
    env->DeleteLocalRef(instance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return player;
}

bool AbilityFlag(bool creative) {
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_predicates.minecraft && !ResolvePredicates(env))
        return false;

    jobject player = PredicatePlayer(env);
    if (player == nullptr)
        return false;

    jobject abilities = env->CallObjectMethod(player, g_predicates.getAbilities);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck())
        env->ExceptionClear();
    if (abilities == nullptr)
        return false;

    const jfieldID field = creative ? g_predicates.instabuild : g_predicates.mayBuild;
    const jboolean value = env->GetBooleanField(abilities, field);
    env->DeleteLocalRef(abilities);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return value == JNI_TRUE;
}

jobject CurrentPlayer(JNIEnv* env) {
    jobject instance = env->CallStaticObjectMethod(g_player.minecraft, g_player.getInstance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    if (instance == nullptr)
        return nullptr;

    jobject player = env->GetObjectField(instance, g_player.player);
    env->DeleteLocalRef(instance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return player;
}

}

bool PlayerOnGround() {
    if (compat::UsesModernRuntime()) return compat::mc263::PlayerOnGround();
    if (compat::IsLegacyLunar()) return compat::lunar::PlayerOnGround();
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_player.nightVision && !Resolve(env))
        return false;

    jobject player = CurrentPlayer(env);
    if (player == nullptr)
        return false;

    const jboolean onGround = env->CallBooleanMethod(player, g_player.onGround);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return onGround == JNI_TRUE;
}

bool AddNightVision() {
    if (compat::UsesModernRuntime()) return compat::mc263::AddNightVision();
    if (compat::IsLegacyLunar()) return compat::lunar::AddNightVision();
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_player.nightVision && !Resolve(env))
        return false;

    jobject player = CurrentPlayer(env);
    if (player == nullptr)
        return false;

    jobject effect = env->NewObject(g_player.instanceClass, g_player.instanceCtor,
                                    g_player.nightVision, kEffectDuration, static_cast<jint>(0),
                                    static_cast<jint>(0), static_cast<jint>(0),
                                    static_cast<jint>(0));
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(player);
        return false;
    }

    const jboolean added = effect
        ? env->CallBooleanMethod(player, g_player.addEffect, effect) : JNI_FALSE;
    if (effect)
        env->DeleteLocalRef(effect);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return added == JNI_TRUE;
}

bool RemoveNightVision() {
    if (compat::UsesModernRuntime()) return compat::mc263::RemoveNightVision();
    if (compat::IsLegacyLunar()) return compat::lunar::RemoveNightVision();
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_player.nightVision && !Resolve(env))
        return false;

    jobject player = CurrentPlayer(env);
    if (player == nullptr)
        return false;

    const jboolean removed =
        env->CallBooleanMethod(player, g_player.removeEffect, g_player.nightVision);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return removed == JNI_TRUE;
}

bool PlayerUsingItem() {
    if (compat::UsesModernRuntime()) return compat::mc263::PlayerUsingItem();
    if (compat::IsLegacyLunar()) return compat::lunar::PlayerUsingItem();
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_predicates.minecraft && !ResolvePredicates(env))
        return false;

    jobject player = PredicatePlayer(env);
    if (player == nullptr)
        return false;

    const jboolean result = env->CallBooleanMethod(player, g_predicates.isUsingItem);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return result == JNI_TRUE;
}

bool PlayerInCreative() { return compat::UsesModernRuntime() ? compat::mc263::PlayerInCreative() : compat::IsLegacyLunar() ? compat::lunar::PlayerInCreative() : AbilityFlag(true); }

bool PlayerAttackCooldownReady() {
    if (compat::UsesModernRuntime()) return compat::mc263::PlayerAttackCooldownReady();
    if (compat::IsLegacyLunar()) return true;
    JniAttach attach;
    if (!attach) return false;
    JNIEnv* env = attach.env;
    jobject player = compat::LocalPlayer(env);
    if (!player) return false;
    jclass type = env->GetObjectClass(player);
    jmethodID method = type ? env->GetMethodID(type, kAttackStrength, kAttackStrengthDesc) : nullptr;
    bool clean = !env->ExceptionCheck();
    if (!clean) env->ExceptionClear();
    float strength = 0.0f;
    if (clean && method) {
        strength = env->CallFloatMethod(player, method, 0.0f);
        clean = !env->ExceptionCheck();
        if (!clean) env->ExceptionClear();
    }
    if (type) env->DeleteLocalRef(type);
    env->DeleteLocalRef(player);
    return clean && method && std::isfinite(strength) && strength >= 1.0f;
}

bool PlayerCanEdit() { return compat::UsesModernRuntime() ? compat::mc263::PlayerCanEdit() : compat::IsLegacyLunar() ? compat::lunar::PlayerCanEdit() : AbilityFlag(false); }

bool ResetPlayerJumpDelay() {
    compat::mc263::access::Frame frame;
    if (!frame) return false;
    auto player = frame.player();
    if (!player) return false;
    const auto& field = compat::IsLegacyLunar() ? kLunarJumpDelay :
        compat::UsesModernRuntime() ? kModernJumpDelay : kForgeJumpDelay;
    return frame.set(player, field, 0);
}

bool HoldingWeapon() {
    if (compat::UsesModernRuntime()) return compat::mc263::HoldingWeapon();
    if (compat::IsLegacyLunar()) return compat::lunar::HoldingWeapon();
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_predicates.minecraft && !ResolvePredicates(env))
        return false;

    jobject player = PredicatePlayer(env);
    if (player == nullptr)
        return false;

    jobject stack = env->CallObjectMethod(player, g_predicates.getMainHandItem);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (stack == nullptr)
        return false;

    jobject item = env->CallObjectMethod(stack, g_predicates.getItem);
    env->DeleteLocalRef(stack);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (item == nullptr)
        return false;

    const bool weapon = env->IsInstanceOf(item, g_predicates.sword) ||
                        env->IsInstanceOf(item, g_predicates.axe) ||
                        env->IsInstanceOf(item, g_predicates.trident);
    env->DeleteLocalRef(item);
    return weapon;
}

}
