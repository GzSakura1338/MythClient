#include "minecraft/java/api/game_entity.h"

#include <chrono>
#include <cmath>
#include <algorithm>
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"

#include "minecraft/java/api/base.h"
#include "minecraft/java/api/jni_lookup.h"

namespace nova::java {
namespace {

#define NOVA_FORGE1201_ENTITY
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_ENTITY

// Entity's own previous-tick position, which is where the frame interpolates from.

struct Handles {
    jclass    minecraft = nullptr;
    jmethodID getInstance = nullptr;
    jfieldID  level = nullptr;
    jfieldID  player = nullptr;
    jmethodID frameTime = nullptr;

    jclass    playerClass = nullptr;
    jclass    levelClass = nullptr;
    jclass    entityClass = nullptr;
    jclass    livingClass = nullptr;
    jclass    componentClass = nullptr;
    jclass    aabbClass = nullptr;
    jclass    vecClass = nullptr;
    jclass    hitClass = nullptr;
    jclass    hitTypeClass = nullptr;
    jclass    entityHitClass = nullptr;
    jclass    clipCtxClass = nullptr;
    jclass    clipBlockClass = nullptr;
    jclass    clipFluidClass = nullptr;

    jmethodID getEntities = nullptr;
    jmethodID players = nullptr;
    jmethodID clip = nullptr;

    jmethodID getX = nullptr;
    jmethodID getY = nullptr;
    jmethodID getZ = nullptr;
    jmethodID position = nullptr;
    jmethodID eyePosition = nullptr;
    jmethodID eyeHeight = nullptr;
    jmethodID boundingBox = nullptr;
    jmethodID distanceTo = nullptr;
    jmethodID pickRadius = nullptr;
    jmethodID isSpectator = nullptr;
    jmethodID isPickable = nullptr;
    jmethodID isInvisible = nullptr;
    jmethodID isAlliedTo = nullptr;
    jmethodID displayName = nullptr;
    jmethodID componentString = nullptr;
    jmethodID getHealth = nullptr;
    jmethodID getAbsorption = nullptr;
    jmethodID getMaxHealth = nullptr;
    jmethodID setHealth = nullptr;
    jmethodID scoreboardName = nullptr;
    jfieldID  xOld = nullptr;
    jfieldID  yOld = nullptr;
    jfieldID  zOld = nullptr;
    jmethodID levelGetEntity = nullptr;
    jmethodID distanceSqr = nullptr;
    jmethodID rootVehicle = nullptr;
    jfieldID  tickCount = nullptr;
    jfieldID  onGround = nullptr;
    jfieldID  hurtTime = nullptr;
    jfieldID  deathTime = nullptr;

    jmethodID aabbCtor = nullptr;
    jmethodID aabbClip = nullptr;
    jmethodID aabbContains = nullptr;
    jmethodID aabbInflate = nullptr;
    jmethodID aabbExpand = nullptr;
    jfieldID  aabbMinX = nullptr;
    jfieldID  aabbMinY = nullptr;
    jfieldID  aabbMinZ = nullptr;
    jfieldID  aabbMaxX = nullptr;
    jfieldID  aabbMaxY = nullptr;
    jfieldID  aabbMaxZ = nullptr;

    jmethodID vecCtor = nullptr;
    jmethodID vecDirFromRot = nullptr;
    jfieldID  vecX = nullptr;
    jfieldID  vecY = nullptr;
    jfieldID  vecZ = nullptr;

    jmethodID hitGetType = nullptr;
    jmethodID hitGetLocation = nullptr;
    jmethodID hitGetBlockPos = nullptr;
    jmethodID hitGetDirection = nullptr;
    jmethodID vec3iGetX = nullptr;
    jmethodID vec3iGetY = nullptr;
    jmethodID vec3iGetZ = nullptr;
    jmethodID ordinal = nullptr;

    jmethodID clipCtxCtor = nullptr;
    jobject   clipOutline = nullptr;
    jobject   clipNone = nullptr;
    jmethodID entityHitCtor = nullptr;

    jmethodID listSize = nullptr;
    jmethodID listGet = nullptr;
    jmethodID optionalIsPresent = nullptr;
    jmethodID optionalGet = nullptr;
};

struct KindHandles {
    jclass    monster = nullptr;
    jclass    slime = nullptr;
    jclass    dragon = nullptr;
    jclass    dragonPart = nullptr;
    jclass    animal = nullptr;
    jclass    ambient = nullptr;
    jclass    waterAnimal = nullptr;
    jclass    horse = nullptr;
    jclass    villager = nullptr;
    jmethodID isAlive = nullptr;
};

Handles g_entity;
bool g_entityLogged = false;
bool g_entityOptionalLogged = false;
bool g_entityResolveFailed = false;
std::chrono::steady_clock::time_point g_entityResolveRetryAt;

KindHandles g_kind;
int g_kindState = 0;

constexpr int kEntityResolveRetrySeconds = 10;

bool Resolve(JNIEnv* env) {
    jobject loader = GetMinecraftClassLoader(env);
    jclass minecraft = nova_base::loadOrFind(env, loader, kMinecraftDot, kMinecraftSig);
    jclass level = nova_base::loadOrFind(env, loader, kLevelDot, kLevelSig);
    jclass entity = nova_base::loadOrFind(env, loader, kEntityDot, kEntitySig);
    jclass player = nova_base::loadOrFind(env, loader, kPlayerDot, kPlayerSig);
    jclass aabb = nova_base::loadOrFind(env, loader, kAabbDot, kAabbSig);
    jclass vec = nova_base::loadOrFind(env, loader, kVec3Dot, kVec3Sig);
    jclass hit = nova_base::loadOrFind(env, loader, kHitDot, kHitSig);
    jclass blockHit = nova_base::loadOrFind(env, loader, kBlockHitDot, kBlockHitSig);
    jclass hitType = nova_base::loadOrFind(env, loader, kMappedHitResultTypeDot,
                                           kHitTypeSig);
    jclass entityHit = nova_base::loadOrFind(env, loader, kEntityHitDot, kEntityHitSig);
    jclass clipCtx = nova_base::loadOrFind(env, loader, kClipCtxDot, kClipCtxSig);
    jclass clipBlock = nova_base::loadOrFind(env, loader, kMappedClipContextBlockDot,
                                             kClipBlockSig);
    jclass clipFluid = nova_base::loadOrFind(env, loader, kMappedClipContextFluidDot,
                                             kClipFluidSig);
    jclass list = nova_base::loadOrFind(env, loader, kListDot, kListSig);
    jclass living = nova_base::loadOrFind(env, loader, kMappedLivingEntityDot,
                                          kMappedLivingEntitySig);
    jclass component = nova_base::loadOrFind(env, loader, kMappedComponentDot,
                                             kMappedComponentSig);
    jclass enumClass = nova_base::loadOrFind(env, loader, kMappedEnumDot, kMappedEnumSig);
    jclass optional = nova_base::loadOrFind(env, loader, kOptionalDot, kOptionalSig);
    jclass blockPos = nova_base::loadOrFind(env, loader, kBlockPosDot, kBlockPosSig);
    if (loader)
        env->DeleteGlobalRef(loader);

    if (!minecraft || !level || !entity || !player || !aabb || !vec || !hit || !blockHit ||
        !hitType || !entityHit || !clipCtx || !clipBlock || !clipFluid || !list || !living ||
        !enumClass || !optional || !blockPos || !component) {
        if (minecraft) env->DeleteLocalRef(minecraft);
        if (level) env->DeleteLocalRef(level);
        if (entity) env->DeleteLocalRef(entity);
        if (player) env->DeleteLocalRef(player);
        if (aabb) env->DeleteLocalRef(aabb);
        if (vec) env->DeleteLocalRef(vec);
        if (hit) env->DeleteLocalRef(hit);
        if (blockHit) env->DeleteLocalRef(blockHit);
        if (hitType) env->DeleteLocalRef(hitType);
        if (entityHit) env->DeleteLocalRef(entityHit);
        if (clipCtx) env->DeleteLocalRef(clipCtx);
        if (clipBlock) env->DeleteLocalRef(clipBlock);
        if (clipFluid) env->DeleteLocalRef(clipFluid);
        if (list) env->DeleteLocalRef(list);
        if (living) env->DeleteLocalRef(living);
        if (enumClass) env->DeleteLocalRef(enumClass);
        if (optional) env->DeleteLocalRef(optional);
        if (blockPos) env->DeleteLocalRef(blockPos);
        if (component) env->DeleteLocalRef(component);
        if (!g_entityLogged) {
            g_entityLogged = true;
            LogTo("Base: entity layer unavailable (mc=%d, level=%d, entity=%d, player=%d, "
                  "aabb=%d, vec=%d, hit=%d, hitType=%d, entityHit=%d, clipCtx=%d, "
                  "clipBlock=%d, clipFluid=%d, list=%d, living=%d, enum=%d, optional=%d, "
                  "blockPos=%d)",
                  minecraft != nullptr, level != nullptr, entity != nullptr, player != nullptr,
                  aabb != nullptr, vec != nullptr, hit != nullptr, hitType != nullptr,
                  entityHit != nullptr, clipCtx != nullptr, clipBlock != nullptr,
                  clipFluid != nullptr, list != nullptr, living != nullptr,
                  enumClass != nullptr, optional != nullptr, blockPos != nullptr);
        }
        return false;
    }

    jmethodID getInstance = env->GetStaticMethodID(minecraft, kGetInstance, kGetInstanceDesc);
    jfieldID levelField = env->GetFieldID(minecraft, kLevelField, kLevelSig);
    jfieldID playerField = env->GetFieldID(minecraft, kPlayerField, kMappedLocalPlayerSig);
    jmethodID frameTime = env->GetMethodID(minecraft, kFrameTime, kFrameTimeDesc);

    jmethodID getEntities = env->GetMethodID(level, kGetEntities, kGetEntitiesDesc);
    jmethodID players = env->GetMethodID(level, kPlayers, kPlayersDesc);
    jmethodID clip = env->GetMethodID(level, kClip, kClipDesc);

    jmethodID getX = env->GetMethodID(entity, kGetX, kGetDouble);
    jmethodID getY = env->GetMethodID(entity, kGetY, kGetDouble);
    jmethodID getZ = env->GetMethodID(entity, kGetZ, kGetDouble);
    jmethodID position = env->GetMethodID(entity, kPosition, kPositionDesc);
    jmethodID eyePosition = env->GetMethodID(entity, kGetEyePos, kGetEyePosDesc);
    jmethodID eyeHeight = env->GetMethodID(entity, kGetEyeHeight, kGetEyeHeightDesc);
    jmethodID boundingBox = env->GetMethodID(entity, kBoundingBox, kBoundingBoxDesc);
    jmethodID distanceTo = env->GetMethodID(entity, kDistanceTo, kDistanceToDesc);
    jmethodID pickRadius = env->GetMethodID(entity, kPickRadius, kPickRadiusDesc);
    jmethodID isSpectator = env->GetMethodID(entity, kIsSpectator, kBoolDesc);
    jmethodID isPickable = env->GetMethodID(entity, kIsPickable, kBoolDesc);
    jmethodID rootVehicle = env->GetMethodID(entity, kRootVehicle, kRootVehicleDesc);
    jmethodID isInvisible = env->GetMethodID(entity, kIsInvisible, kBoolDesc);
    jmethodID isAlliedTo = env->GetMethodID(entity, kIsAlliedTo,
                                            kMappedEntityToBooleanDesc);
    jmethodID displayName = env->GetMethodID(entity, kGetDisplayName,
                                             kMappedNoArgsToComponentDesc);
    jmethodID componentString = env->GetMethodID(component, kMappedGetString, kStringDesc);
    jmethodID getHealth = env->GetMethodID(living, kGetHealth, kFloatDesc);
    jmethodID getAbsorption = env->GetMethodID(living, kGetAbsorption, kFloatDesc);
    jmethodID getMaxHealth = env->GetMethodID(living, kGetMaxHealth, kFloatDesc);
    jmethodID setHealth = env->GetMethodID(living, kSetHealth, kSetHealthDesc);
    // Entity declares getScoreboardName and Player overrides it, so the call below
    // dispatches to the player's own answer.
    jmethodID scoreboardName = env->GetMethodID(entity, kScoreboardName, kStringDesc);
    jfieldID xOld = env->GetFieldID(entity, kXOld, kDoubleField);
    jfieldID yOld = env->GetFieldID(entity, kYOld, kDoubleField);
    jfieldID zOld = env->GetFieldID(entity, kZOld, kDoubleField);
    // getEntity is declared on Level and found by walking up from ClientLevel.
    jmethodID levelGetEntity = env->GetMethodID(level, kLevelGetEntity, kLevelGetEntityDesc);
    jmethodID distanceSqr = env->GetMethodID(entity, kDistanceSqr,
                                             kMappedEntityToDoubleDesc);
    jfieldID tickCount = env->GetFieldID(entity, kTickCount, kIntDesc);
    jfieldID onGround = env->GetFieldID(entity, kOnGround, kMappedPrimitiveBooleanSig);

    jmethodID aabbCtor = env->GetMethodID(aabb, kAabbCtor, kAabbCtorDesc);
    jmethodID aabbClip = env->GetMethodID(aabb, kAabbClip, kAabbClipDesc);
    jmethodID aabbContains = env->GetMethodID(aabb, kAabbContains, kAabbContainsDesc);
    jmethodID aabbInflate = env->GetMethodID(aabb, kAabbInflate, kAabbInflateDesc);
    jmethodID aabbExpand = env->GetMethodID(aabb, kAabbExpand, kAabbExpandDesc);
    jfieldID aabbMinX = env->GetFieldID(aabb, kAabbMinX, kDoubleField);
    jfieldID aabbMinY = env->GetFieldID(aabb, kAabbMinY, kDoubleField);
    jfieldID aabbMinZ = env->GetFieldID(aabb, kAabbMinZ, kDoubleField);
    jfieldID aabbMaxX = env->GetFieldID(aabb, kAabbMaxX, kDoubleField);
    jfieldID aabbMaxY = env->GetFieldID(aabb, kAabbMaxY, kDoubleField);
    jfieldID aabbMaxZ = env->GetFieldID(aabb, kAabbMaxZ, kDoubleField);

    jmethodID vecCtor = env->GetMethodID(vec, kVecCtor, kVecCtorDesc);
    jmethodID vecDirFromRot =
        env->GetStaticMethodID(vec, kVecDirFromRot, kVecDirFromRotDesc);
    jfieldID vecX = env->GetFieldID(vec, kVecX, kDoubleField);
    jfieldID vecY = env->GetFieldID(vec, kVecY, kDoubleField);
    jfieldID vecZ = env->GetFieldID(vec, kVecZ, kDoubleField);

    jmethodID hitGetType = env->GetMethodID(hit, kHitGetType, kHitGetTypeDesc);
    jmethodID hitGetLocation = env->GetMethodID(hit, kHitGetLocation, kHitGetLocationDesc);
    jmethodID hitGetBlockPos = env->GetMethodID(blockHit, kHitGetBlockPos, kHitGetBlockPosDesc);
    jmethodID hitGetDirection =
        env->GetMethodID(blockHit, kHitGetDirection, kHitGetDirectionDesc);
    jmethodID vec3iGetX = env->GetMethodID(blockPos, kVec3iGetX, kListSizeDesc);
    jmethodID vec3iGetY = env->GetMethodID(blockPos, kVec3iGetY, kListSizeDesc);
    jmethodID vec3iGetZ = env->GetMethodID(blockPos, kVec3iGetZ, kListSizeDesc);
    jmethodID ordinal = env->GetMethodID(enumClass, kMappedOrdinal, kListSizeDesc);

    jmethodID clipCtxCtor = env->GetMethodID(clipCtx, kClipCtxCtor, kClipCtxCtorDesc);
    jfieldID outlineField = env->GetStaticFieldID(clipBlock, kMappedOUTLINE, kClipBlockSig);
    jfieldID noneField = env->GetStaticFieldID(clipFluid, kMappedNONE, kClipFluidSig);
    jobject clipOutline = outlineField ? env->GetStaticObjectField(clipBlock, outlineField) : nullptr;
    jobject clipNone = noneField ? env->GetStaticObjectField(clipFluid, noneField) : nullptr;

    jmethodID entityHitCtor = env->GetMethodID(entityHit, kEntityHitCtor, kEntityHitCtorDesc);
    jmethodID listSize = env->GetMethodID(list, kListSize, kListSizeDesc);
    jmethodID listGet = env->GetMethodID(list, kListGet, kListGetDesc);
    jfieldID hurtTime = env->GetFieldID(living, kHurtTime, kIntDesc);
    jfieldID deathTime = env->GetFieldID(living, kDeathTime, kIntDesc);
    jmethodID optionalIsPresent = env->GetMethodID(optional, kMappedIsPresent, kBoolDesc);
    jmethodID optionalGet = env->GetMethodID(optional, kListGet, kMappedNoArgsToObjectDesc);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    if ((!isInvisible || !isAlliedTo || !displayName || !componentString || !getHealth ||
         !getAbsorption || !getMaxHealth || !setHealth || !scoreboardName || !distanceSqr ||
         !hitGetBlockPos || !hitGetDirection || !xOld || !yOld || !zOld ||
         !vec3iGetX || !vec3iGetY || !vec3iGetZ || !levelGetEntity) &&
        !g_entityOptionalLogged) {
        g_entityOptionalLogged = true;
        LogTo("Base: entity optional missing -> invis=%d allied=%d name=%d component=%d "
              "health=%d absorb=%d maxHealth=%d setHealth=%d boardName=%d distSq=%d "
              "blockPos=%d direction=%d vec3iX=%d vec3iY=%d vec3iZ=%d levelGetEntity=%d "
              "oldPos=%d/%d/%d",
              isInvisible != nullptr, isAlliedTo != nullptr, displayName != nullptr,
              componentString != nullptr, getHealth != nullptr, getAbsorption != nullptr,
              getMaxHealth != nullptr, setHealth != nullptr, scoreboardName != nullptr,
              distanceSqr != nullptr, hitGetBlockPos != nullptr, hitGetDirection != nullptr,
              vec3iGetX != nullptr, vec3iGetY != nullptr, vec3iGetZ != nullptr,
              levelGetEntity != nullptr, xOld != nullptr, yOld != nullptr, zOld != nullptr);
    }

    const bool ok = getInstance && levelField && playerField && frameTime && getEntities &&
                    players && clip && getX && getY && getZ && position && eyePosition &&
                    eyeHeight && boundingBox && distanceTo && pickRadius && isSpectator &&
                    isPickable &&
                    rootVehicle && tickCount && onGround && aabbCtor &&
                    aabbClip && aabbContains && aabbInflate && aabbExpand && vecCtor &&
                    vecDirFromRot && vecX && vecY && vecZ && hitGetType && hitGetLocation &&
                    hitGetBlockPos && hitGetDirection && vec3iGetX && vec3iGetY && vec3iGetZ &&
                    ordinal && clipCtxCtor &&
                    clipOutline && clipNone && entityHitCtor && listSize && listGet && hurtTime &&
                    optionalIsPresent && optionalGet;
    if (!ok) {
        if (!g_entityLogged) {
            g_entityLogged = true;
            LogTo("Base: entity layer incomplete -> instance=%d level=%d player=%d "
                  "frameTime=%d getEntities=%d players=%d clip=%d getX=%d getY=%d getZ=%d "
                  "pos=%d eye=%d eyeHeight=%d box=%d distanceTo=%d pickRadius=%d "
                  "spectator=%d pickable=%d rootVehicle=%d tickCount=%d onGround=%d "
                  "aabbCtor=%d aabbClip=%d aabbContains=%d aabbInflate=%d aabbExpand=%d "
                  "vecCtor=%d vecDirFromRot=%d vecX=%d hitGetType=%d hitGetLocation=%d "
                  "hitGetBlockPos=%d hitGetDirection=%d vec3iGetX=%d vec3iGetY=%d "
                  "vec3iGetZ=%d ordinal=%d clipCtxCtor=%d clipOutline=%d clipNone=%d "
                  "entityHitCtor=%d listSize=%d listGet=%d hurtTime=%d optionalIsPresent=%d "
                  "optionalGet=%d",
                  getInstance != nullptr, levelField != nullptr, playerField != nullptr,
                  frameTime != nullptr, getEntities != nullptr, players != nullptr,
                  clip != nullptr, getX != nullptr, getY != nullptr, getZ != nullptr,
                  position != nullptr, eyePosition != nullptr, eyeHeight != nullptr,
                  boundingBox != nullptr, distanceTo != nullptr, pickRadius != nullptr,
                  isSpectator != nullptr, isPickable != nullptr, rootVehicle != nullptr,
                  tickCount != nullptr, onGround != nullptr, aabbCtor != nullptr,
                  aabbClip != nullptr, aabbContains != nullptr, aabbInflate != nullptr,
                  aabbExpand != nullptr, vecCtor != nullptr, vecDirFromRot != nullptr,
                  vecX != nullptr, hitGetType != nullptr, hitGetLocation != nullptr,
                  hitGetBlockPos != nullptr, hitGetDirection != nullptr, vec3iGetX != nullptr,
                  vec3iGetY != nullptr, vec3iGetZ != nullptr, ordinal != nullptr,
                  clipCtxCtor != nullptr, clipOutline != nullptr, clipNone != nullptr,
                  entityHitCtor != nullptr, listSize != nullptr, listGet != nullptr,
                  hurtTime != nullptr, optionalIsPresent != nullptr, optionalGet != nullptr);
        }
        env->DeleteLocalRef(minecraft);
        env->DeleteLocalRef(level);
        env->DeleteLocalRef(entity);
        env->DeleteLocalRef(player);
        env->DeleteLocalRef(aabb);
        env->DeleteLocalRef(vec);
        env->DeleteLocalRef(hit);
        env->DeleteLocalRef(hitType);
        env->DeleteLocalRef(entityHit);
        env->DeleteLocalRef(clipCtx);
        env->DeleteLocalRef(clipBlock);
        env->DeleteLocalRef(clipFluid);
        env->DeleteLocalRef(list);
        env->DeleteLocalRef(living);
        env->DeleteLocalRef(enumClass);
        env->DeleteLocalRef(optional);
        return false;
    }

    g_entity.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));
    g_entity.getInstance = getInstance;
    g_entity.level = levelField;
    g_entity.player = playerField;
    g_entity.frameTime = frameTime;

    g_entity.playerClass = static_cast<jclass>(env->NewGlobalRef(player));
    g_entity.levelClass = static_cast<jclass>(env->NewGlobalRef(level));
    g_entity.entityClass = static_cast<jclass>(env->NewGlobalRef(entity));
    g_entity.livingClass = static_cast<jclass>(env->NewGlobalRef(living));
    g_entity.componentClass = static_cast<jclass>(env->NewGlobalRef(component));
    g_entity.aabbClass = static_cast<jclass>(env->NewGlobalRef(aabb));
    g_entity.vecClass = static_cast<jclass>(env->NewGlobalRef(vec));
    g_entity.hitClass = static_cast<jclass>(env->NewGlobalRef(hit));
    g_entity.hitTypeClass = static_cast<jclass>(env->NewGlobalRef(hitType));
    g_entity.entityHitClass = static_cast<jclass>(env->NewGlobalRef(entityHit));
    g_entity.clipCtxClass = static_cast<jclass>(env->NewGlobalRef(clipCtx));
    g_entity.clipBlockClass = static_cast<jclass>(env->NewGlobalRef(clipBlock));
    g_entity.clipFluidClass = static_cast<jclass>(env->NewGlobalRef(clipFluid));

    g_entity.getEntities = getEntities;
    g_entity.players = players;
    g_entity.clip = clip;

    g_entity.getX = getX;
    g_entity.getY = getY;
    g_entity.getZ = getZ;
    g_entity.position = position;
    g_entity.eyePosition = eyePosition;
    g_entity.eyeHeight = eyeHeight;
    g_entity.boundingBox = boundingBox;
    g_entity.distanceTo = distanceTo;
    g_entity.pickRadius = pickRadius;
    g_entity.isSpectator = isSpectator;
    g_entity.isPickable = isPickable;
    g_entity.isInvisible = isInvisible;
    g_entity.isAlliedTo = isAlliedTo;
    g_entity.displayName = displayName;
    g_entity.componentString = componentString;
    g_entity.getHealth = getHealth;
    g_entity.getAbsorption = getAbsorption;
    g_entity.getMaxHealth = getMaxHealth;
    g_entity.setHealth = setHealth;
    g_entity.scoreboardName = scoreboardName;
    g_entity.xOld = xOld;
    g_entity.yOld = yOld;
    g_entity.zOld = zOld;
    g_entity.levelGetEntity = levelGetEntity;
    g_entity.distanceSqr = distanceSqr;
    g_entity.rootVehicle = rootVehicle;
    g_entity.tickCount = tickCount;
    g_entity.onGround = onGround;
    g_entity.hurtTime = hurtTime;
    g_entity.deathTime = deathTime;

    g_entity.aabbCtor = aabbCtor;
    g_entity.aabbClip = aabbClip;
    g_entity.aabbContains = aabbContains;
    g_entity.aabbInflate = aabbInflate;
    g_entity.aabbExpand = aabbExpand;
    g_entity.aabbMinX = aabbMinX;
    g_entity.aabbMinY = aabbMinY;
    g_entity.aabbMinZ = aabbMinZ;
    g_entity.aabbMaxX = aabbMaxX;
    g_entity.aabbMaxY = aabbMaxY;
    g_entity.aabbMaxZ = aabbMaxZ;

    g_entity.vecCtor = vecCtor;
    g_entity.vecDirFromRot = vecDirFromRot;
    g_entity.vecX = vecX;
    g_entity.vecY = vecY;
    g_entity.vecZ = vecZ;

    g_entity.hitGetType = hitGetType;
    g_entity.hitGetLocation = hitGetLocation;
    g_entity.hitGetBlockPos = hitGetBlockPos;
    g_entity.hitGetDirection = hitGetDirection;
    g_entity.vec3iGetX = vec3iGetX;
    g_entity.vec3iGetY = vec3iGetY;
    g_entity.vec3iGetZ = vec3iGetZ;
    g_entity.ordinal = ordinal;

    g_entity.clipCtxCtor = clipCtxCtor;
    g_entity.clipOutline = env->NewGlobalRef(clipOutline);
    g_entity.clipNone = env->NewGlobalRef(clipNone);
    g_entity.entityHitCtor = entityHitCtor;

    g_entity.listSize = listSize;
    g_entity.listGet = listGet;
    g_entity.optionalIsPresent = optionalIsPresent;
    g_entity.optionalGet = optionalGet;

    env->DeleteLocalRef(clipOutline);
    env->DeleteLocalRef(clipNone);
    env->DeleteLocalRef(minecraft);
    env->DeleteLocalRef(level);
    env->DeleteLocalRef(entity);
    env->DeleteLocalRef(player);
    env->DeleteLocalRef(aabb);
    env->DeleteLocalRef(vec);
    env->DeleteLocalRef(hit);
    env->DeleteLocalRef(hitType);
    env->DeleteLocalRef(entityHit);
    env->DeleteLocalRef(clipCtx);
    env->DeleteLocalRef(clipBlock);
    env->DeleteLocalRef(clipFluid);
    env->DeleteLocalRef(list);
    env->DeleteLocalRef(living);
    env->DeleteLocalRef(enumClass);
    env->DeleteLocalRef(optional);
    env->DeleteLocalRef(blockPos);
    env->DeleteLocalRef(component);
    return true;
}

bool EnsureResolved(JNIEnv* env) {
    if (g_entity.minecraft != nullptr)
        return true;

    const auto now = std::chrono::steady_clock::now();
    if (g_entityResolveFailed &&
        now - g_entityResolveRetryAt < std::chrono::seconds(kEntityResolveRetrySeconds)) {
        return false;
    }

    if (Resolve(env))
        return true;

    g_entityResolveFailed = true;
    g_entityResolveRetryAt = now;
    return false;
}

bool ResolveKind(JNIEnv* env) {
    jobject loader = GetMinecraftClassLoader(env);
    jclass monster = nova_base::loadOrFind(env, loader, kMonsterDot,
                                           kMappedMonsterSig);
    jclass slime = nova_base::loadOrFind(env, loader, kSlimeDot,
                                         kMappedSlimeSig);
    jclass dragon = nova_base::loadOrFind(env, loader, kDragonDot,
                                          kMappedEnderDragonSig);
    jclass dragonPart = nova_base::loadOrFind(env, loader, kDragonPartDot,
                                              kMappedEnderDragonPartSig);
    jclass animal = nova_base::loadOrFind(env, loader, kAnimalDot,
                                          kMappedAnimalSig);
    jclass ambient = nova_base::loadOrFind(env, loader, kAmbientDot,
                                           kMappedAmbientCreatureSig);
    jclass water = nova_base::loadOrFind(env, loader, kWaterDot,
                                         kMappedWaterAnimalSig);
    jclass horse = nova_base::loadOrFind(
        env, loader, kHorseDot, kMappedAbstractHorseSig);
    jclass villager = nova_base::loadOrFind(
        env, loader, kVillagerDot, kMappedAbstractVillagerSig);
    if (loader)
        env->DeleteGlobalRef(loader);

    jmethodID isAlive = env->GetMethodID(g_entity.entityClass, kIsAlive, kBoolDesc);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (!monster || !slime || !dragon || !dragonPart || !animal || !ambient || !water ||
        !horse || !villager || !isAlive) {
        LogTo("Base: entity kinds unavailable -> monster=%d slime=%d dragon=%d part=%d "
              "animal=%d ambient=%d water=%d horse=%d villager=%d alive=%d",
              monster != nullptr, slime != nullptr, dragon != nullptr,
              dragonPart != nullptr, animal != nullptr, ambient != nullptr, water != nullptr,
              horse != nullptr, villager != nullptr, isAlive != nullptr);
        if (monster) env->DeleteLocalRef(monster);
        if (slime) env->DeleteLocalRef(slime);
        if (dragon) env->DeleteLocalRef(dragon);
        if (dragonPart) env->DeleteLocalRef(dragonPart);
        if (animal) env->DeleteLocalRef(animal);
        if (ambient) env->DeleteLocalRef(ambient);
        if (water) env->DeleteLocalRef(water);
        if (horse) env->DeleteLocalRef(horse);
        if (villager) env->DeleteLocalRef(villager);
        return false;
    }

    g_kind.monster = static_cast<jclass>(env->NewGlobalRef(monster));
    g_kind.slime = static_cast<jclass>(env->NewGlobalRef(slime));
    g_kind.dragon = static_cast<jclass>(env->NewGlobalRef(dragon));
    g_kind.dragonPart = static_cast<jclass>(env->NewGlobalRef(dragonPart));
    g_kind.animal = static_cast<jclass>(env->NewGlobalRef(animal));
    g_kind.ambient = static_cast<jclass>(env->NewGlobalRef(ambient));
    g_kind.waterAnimal = static_cast<jclass>(env->NewGlobalRef(water));
    g_kind.horse = static_cast<jclass>(env->NewGlobalRef(horse));
    g_kind.villager = static_cast<jclass>(env->NewGlobalRef(villager));
    g_kind.isAlive = isAlive;

    env->DeleteLocalRef(monster);
    env->DeleteLocalRef(slime);
    env->DeleteLocalRef(dragon);
    env->DeleteLocalRef(dragonPart);
    env->DeleteLocalRef(animal);
    env->DeleteLocalRef(ambient);
    env->DeleteLocalRef(water);
    env->DeleteLocalRef(horse);
    env->DeleteLocalRef(villager);
    return true;
}

bool EnsureKindResolved(JNIEnv* env) {
    if (g_kindState != 0 || g_entity.minecraft == nullptr)
        return g_kindState > 0;

    g_kindState = ResolveKind(env) ? 1 : -1;
    return g_kindState > 0;
}

void DeleteGlobal(JNIEnv* env, void* obj) {
    if (obj)
        env->DeleteGlobalRef(static_cast<jobject>(obj));
}

jobject Client(JNIEnv* env) {
    jobject instance = env->CallStaticObjectMethod(g_entity.minecraft, g_entity.getInstance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return instance;
}

jobject LevelLocal(JNIEnv* env) {
    jobject client = Client(env);
    if (client == nullptr)
        return nullptr;
    jobject level = env->GetObjectField(client, g_entity.level);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return level;
}

jobject MakeVec3(JNIEnv* env, const rotation::Vec3& v) {
    jobject out = env->NewObject(g_entity.vecClass, g_entity.vecCtor, v.x, v.y, v.z);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return out;
}

jobject MakeBox(JNIEnv* env, const Box& box) {
    jobject out = env->NewObject(g_entity.aabbClass, g_entity.aabbCtor, box.minX, box.minY,
                                 box.minZ, box.maxX, box.maxY, box.maxZ);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return out;
}

bool ReadVec3(JNIEnv* env, jobject vec, rotation::Vec3& out) {
    if (vec == nullptr)
        return false;
    out.x = env->GetDoubleField(vec, g_entity.vecX);
    out.y = env->GetDoubleField(vec, g_entity.vecY);
    out.z = env->GetDoubleField(vec, g_entity.vecZ);
    const bool clean = !env->ExceptionCheck();
    if (env->ExceptionCheck())
        env->ExceptionClear();
    return clean;
}

bool ReadBox(JNIEnv* env, jobject aabb, Box& out) {
    if (aabb == nullptr)
        return false;
    out.minX = env->GetDoubleField(aabb, g_entity.aabbMinX);
    out.minY = env->GetDoubleField(aabb, g_entity.aabbMinY);
    out.minZ = env->GetDoubleField(aabb, g_entity.aabbMinZ);
    out.maxX = env->GetDoubleField(aabb, g_entity.aabbMaxX);
    out.maxY = env->GetDoubleField(aabb, g_entity.aabbMaxY);
    out.maxZ = env->GetDoubleField(aabb, g_entity.aabbMaxZ);
    const bool clean = !env->ExceptionCheck();
    if (env->ExceptionCheck())
        env->ExceptionClear();
    return clean;
}

void ReadList(JNIEnv* env, jobject list, std::vector<EntityHandle>& out) {
    const jint size = env->CallIntMethod(list, g_entity.listSize);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return;
    }
    for (jint i = 0; i < size; ++i) {
        jobject element = env->CallObjectMethod(list, g_entity.listGet, i);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            continue;
        }
        if (element) {
            out.push_back(AdoptEntity(element));
            env->DeleteLocalRef(element);
        }
    }
}

}

EntityHandle::~EntityHandle() {
    Reset();
}

EntityHandle::EntityHandle(EntityHandle&& other) noexcept : obj(other.obj) {
    other.obj = nullptr;
}

EntityHandle& EntityHandle::operator=(EntityHandle&& other) noexcept {
    if (this != &other) {
        Reset();
        obj = other.obj;
        other.obj = nullptr;
    }
    return *this;
}

void EntityHandle::Reset() {
    if (obj == nullptr)
        return;
    void* target = obj;
    obj = nullptr;
    JniAttach attach;
    if (!attach)
        return;
    DeleteGlobal(attach.env, target);
}

bool EntityHandle::IsPlayer() const {
    if (compat::UsesModernRuntime()) { bool value = false; return compat::mc263::ReadEntityBool(obj, compat::mc263::EntityBool::Player, value) && value; }
    if (compat::IsLegacyLunar()) { bool value = false; return compat::lunar::ReadEntityBool(obj, compat::lunar::EntityBool::Player, value) && value; }
    if (obj == nullptr)
        return false;
    JniAttach attach;
    if (!attach)
        return false;
    if (!EnsureResolved(attach.env))
        return false;
    const bool result = attach.env->IsInstanceOf(static_cast<jobject>(obj), g_entity.playerClass);
    if (attach.env->ExceptionCheck())
        attach.env->ExceptionClear();
    return result;
}

bool EntityHandle::IsSpectator() const {
    if (compat::UsesModernRuntime()) { bool value = false; return compat::mc263::ReadEntityBool(obj, compat::mc263::EntityBool::Spectator, value) && value; }
    if (compat::IsLegacyLunar()) { bool value = false; return compat::lunar::ReadEntityBool(obj, compat::lunar::EntityBool::Spectator, value) && value; }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env))
        return false;
    const jboolean result = attach.env->CallBooleanMethod(static_cast<jobject>(obj), g_entity.isSpectator);
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    return result;
}

bool EntityHandle::IsPickable() const {
    if (compat::UsesModernRuntime()) { bool value = false; return compat::mc263::ReadEntityBool(obj, compat::mc263::EntityBool::Pickable, value) && value; }
    if (compat::IsLegacyLunar()) { bool value = false; return compat::lunar::ReadEntityBool(obj, compat::lunar::EntityBool::Pickable, value) && value; }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env))
        return false;
    const jboolean result = attach.env->CallBooleanMethod(static_cast<jobject>(obj), g_entity.isPickable);
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    return result;
}

bool EntityHandle::IsAlive() const {
    if (compat::UsesModernRuntime()) { bool value = false; return compat::mc263::ReadEntityBool(obj, compat::mc263::EntityBool::Alive, value) && value; }
    if (compat::IsLegacyLunar()) { bool value = false; return compat::lunar::ReadEntityBool(obj, compat::lunar::EntityBool::Alive, value) && value; }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env) ||
        !EnsureKindResolved(attach.env))
        return false;
    const jboolean result =
        attach.env->CallBooleanMethod(static_cast<jobject>(obj), g_kind.isAlive);
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    return result;
}

bool EntityHandle::IsSame(const EntityHandle& other) const {
    JniAttach attach;
    if (!attach || obj == nullptr || other.obj == nullptr)
        return false;
    return attach.env->IsSameObject(static_cast<jobject>(obj), static_cast<jobject>(other.obj));
}

bool EntityHandle::IsSameRootVehicle(const EntityHandle& other) const {
    if (compat::UsesModernRuntime()) { return compat::mc263::EntitySameVehicle(obj, other.raw()); }
    if (compat::IsLegacyLunar()) { return compat::lunar::EntitySameVehicle(obj, other.raw()); }
    JniAttach attach;
    if (!attach || obj == nullptr || other.obj == nullptr || !EnsureResolved(attach.env))
        return false;
    JNIEnv* env = attach.env;
    jobject a = env->CallObjectMethod(static_cast<jobject>(obj), g_entity.rootVehicle);
    jobject b = env->CallObjectMethod(static_cast<jobject>(other.obj), g_entity.rootVehicle);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        if (a) env->DeleteLocalRef(a);
        if (b) env->DeleteLocalRef(b);
        return false;
    }
    const jboolean result = env->IsSameObject(a, b);
    if (a) env->DeleteLocalRef(a);
    if (b) env->DeleteLocalRef(b);
    return result;
}

bool EntityHandle::Position(rotation::Vec3& out) const {
    if (compat::UsesModernRuntime()) { return compat::mc263::EntityPosition(obj, out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::EntityPosition(obj, out); }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env))
        return false;
    JNIEnv* env = attach.env;
    jobject vec = env->CallObjectMethod(static_cast<jobject>(obj), g_entity.position);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    const bool clean = ReadVec3(env, vec, out);
    if (vec)
        env->DeleteLocalRef(vec);
    return clean;
}

bool EntityHandle::PositionPair(rotation::Vec3& now, rotation::Vec3& before) const {
    if (compat::UsesModernRuntime()) { return compat::mc263::EntityPosition(obj, now) && compat::mc263::EntityPosition(obj, before, true); }
    if (compat::IsLegacyLunar()) { return compat::lunar::EntityPosition(obj, now) && compat::lunar::EntityPosition(obj, before, true); }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env))
        return false;
    if (g_entity.xOld == nullptr || g_entity.yOld == nullptr || g_entity.zOld == nullptr)
        return false;

    JNIEnv* env = attach.env;
    jobject self = static_cast<jobject>(obj);

    // The current position through its accessors, since the field it lives in is the
    // entity's private Vec3 and the previous tick is three plain doubles; six
    // crossings either way, for every entity on screen, every tick.
    now.x = static_cast<double>(env->CallDoubleMethod(self, g_entity.getX));
    now.y = static_cast<double>(env->CallDoubleMethod(self, g_entity.getY));
    now.z = static_cast<double>(env->CallDoubleMethod(self, g_entity.getZ));
    before.x = static_cast<double>(env->GetDoubleField(self, g_entity.xOld));
    before.y = static_cast<double>(env->GetDoubleField(self, g_entity.yOld));
    before.z = static_cast<double>(env->GetDoubleField(self, g_entity.zOld));

    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

bool EntityHandle::RenderPositionPair(rotation::Vec3& now, rotation::Vec3& before) const {
    if (compat::UsesModernRuntime())
        return compat::mc263::EntityRenderPositionPair(obj, now, before);
    if (compat::IsLegacyLunar())
        return compat::lunar::EntityRenderPositionPair(obj, now, before);
    return PositionPair(now, before);
}

bool EntityHandle::EyePosition(float partialTicks, rotation::Vec3& out) const {
    if (compat::UsesModernRuntime()) { return compat::mc263::EntityEyePosition(obj, partialTicks, out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::EntityEyePosition(obj, partialTicks, out); }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env))
        return false;
    JNIEnv* env = attach.env;
    jobject vec = env->CallObjectMethod(static_cast<jobject>(obj), g_entity.eyePosition,
                                        static_cast<jfloat>(partialTicks));
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    const bool clean = ReadVec3(env, vec, out);
    if (vec)
        env->DeleteLocalRef(vec);
    return clean;
}

bool EntityHandle::EyeHeight(double& out) const {
    if (compat::UsesModernRuntime()) { float value = 0; if (!compat::mc263::ReadEntityFloat(obj, compat::mc263::EntityFloat::EyeHeight, value)) return false; out = value; return true; }
    if (compat::IsLegacyLunar()) { float value = 0; if (!compat::lunar::ReadEntityFloat(obj, compat::lunar::EntityFloat::EyeHeight, value)) return false; out = value; return true; }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env))
        return false;
    const jfloat value = attach.env->CallFloatMethod(static_cast<jobject>(obj), g_entity.eyeHeight);
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    out = static_cast<double>(value);
    return true;
}

bool EntityHandle::GetY(double& out) const {
    if (compat::UsesModernRuntime()) { rotation::Vec3 value; if (!Position(value)) return false; out = value.y; return true; }
    if (compat::IsLegacyLunar()) { rotation::Vec3 value; if (!Position(value)) return false; out = value.y; return true; }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env))
        return false;
    const jdouble value = attach.env->CallDoubleMethod(static_cast<jobject>(obj), g_entity.getY);
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    out = static_cast<double>(value);
    return true;
}

bool EntityHandle::BoundingBox(Box& out) const {
    if (compat::UsesModernRuntime()) { return compat::mc263::EntityBox(obj, out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::EntityBox(obj, out); }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env))
        return false;
    JNIEnv* env = attach.env;
    jobject aabb = env->CallObjectMethod(static_cast<jobject>(obj), g_entity.boundingBox);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    const bool clean = ReadBox(env, aabb, out);
    if (aabb)
        env->DeleteLocalRef(aabb);
    return clean;
}

bool EntityHandle::DistanceTo(const EntityHandle& other, double& out) const {
    if (compat::UsesModernRuntime()) { double value = 0; if (!DistanceSqTo(other, value)) return false; out = std::sqrt(value); return true; }
    if (compat::IsLegacyLunar()) { double value = 0; if (!DistanceSqTo(other, value)) return false; out = std::sqrt(value); return true; }
    JniAttach attach;
    if (!attach || obj == nullptr || other.obj == nullptr || !EnsureResolved(attach.env))
        return false;
    const jfloat value = attach.env->CallFloatMethod(static_cast<jobject>(obj), g_entity.distanceTo,
                                                     static_cast<jobject>(other.obj));
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    out = static_cast<double>(value);
    return true;
}

bool EntityHandle::PickRadius(double& out) const {
    if (compat::UsesModernRuntime()) { float value = 0; if (!compat::mc263::ReadEntityFloat(obj, compat::mc263::EntityFloat::PickRadius, value)) return false; out = value; return true; }
    if (compat::IsLegacyLunar()) { float value = 0; if (!compat::lunar::ReadEntityFloat(obj, compat::lunar::EntityFloat::PickRadius, value)) return false; out = value; return true; }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env))
        return false;
    const jfloat value = attach.env->CallFloatMethod(static_cast<jobject>(obj), g_entity.pickRadius);
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    out = static_cast<double>(value);
    return true;
}

bool EntityHandle::IsInvisible(bool& out) const {
    if (compat::UsesModernRuntime()) { return compat::mc263::ReadEntityBool(obj, compat::mc263::EntityBool::Invisible, out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::ReadEntityBool(obj, compat::lunar::EntityBool::Invisible, out); }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env) || g_entity.isInvisible == nullptr)
        return false;
    const jboolean value = attach.env->CallBooleanMethod(static_cast<jobject>(obj), g_entity.isInvisible);
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    out = value != JNI_FALSE;
    return true;
}

bool EntityHandle::IsAlliedTo(const EntityHandle& other, bool& out) const {
    if (compat::UsesModernRuntime()) { return compat::mc263::EntitySameTeam(obj, other.raw(), out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::EntitySameTeam(obj, other.raw(), out); }
    JniAttach attach;
    if (!attach || obj == nullptr || other.obj == nullptr || !EnsureResolved(attach.env) ||
        g_entity.isAlliedTo == nullptr)
        return false;
    const jboolean value = attach.env->CallBooleanMethod(static_cast<jobject>(obj), g_entity.isAlliedTo,
                                                         static_cast<jobject>(other.obj));
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    out = value != JNI_FALSE;
    return true;
}

bool EntityHandle::DisplayName(std::string& out) const {
    if (compat::UsesModernRuntime()) { return compat::mc263::EntityName(obj, out, true); }
    if (compat::IsLegacyLunar()) { return compat::lunar::EntityName(obj, out, true); }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env) || g_entity.displayName == nullptr ||
        g_entity.componentString == nullptr)
        return false;
    JNIEnv* env = attach.env;
    jobject name = env->CallObjectMethod(static_cast<jobject>(obj), g_entity.displayName);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (name == nullptr)
        return false;
    jobject text = env->CallObjectMethod(name, g_entity.componentString);
    env->DeleteLocalRef(name);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (text == nullptr)
        return false;
    const char* utf = env->GetStringUTFChars(static_cast<jstring>(text), nullptr);
    if (utf != nullptr) {
        out = utf;
        env->ReleaseStringUTFChars(static_cast<jstring>(text), utf);
    }
    env->DeleteLocalRef(text);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

// LivingEntity.setHealth. The client's own copy of the value and nothing else: no
// packet goes out and the entity's tracked data is untouched, so the server never
// learns that the number on screen is not the one it sent. It clamps to
// getMaxHealth, which is why a bypassed value above the maximum lands on the
// maximum rather than past the end of the bar.
bool EntityHandle::SetHealth(float health) const {
    if (compat::UsesModernRuntime()) { return compat::mc263::EntitySetHealth(obj, health); }
    if (compat::IsLegacyLunar()) { return compat::lunar::EntitySetHealth(obj, health); }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env) || g_entity.setHealth == nullptr)
        return false;
    if (!attach.env->IsInstanceOf(static_cast<jobject>(obj), g_entity.livingClass)) {
        if (attach.env->ExceptionCheck())
            attach.env->ExceptionClear();
        return false;
    }
    attach.env->CallVoidMethod(static_cast<jobject>(obj), g_entity.setHealth,
                               static_cast<jfloat>(health));
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    return true;
}

bool EntityHandle::ScoreboardName(std::string& out) const {
    if (compat::UsesModernRuntime()) { return compat::mc263::EntityName(obj, out, false); }
    if (compat::IsLegacyLunar()) { return compat::lunar::EntityName(obj, out, false); }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env) ||
        g_entity.scoreboardName == nullptr)
        return false;
    jobject text =
        attach.env->CallObjectMethod(static_cast<jobject>(obj), g_entity.scoreboardName);
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    if (text == nullptr)
        return false;
    const char* utf = attach.env->GetStringUTFChars(static_cast<jstring>(text), nullptr);
    if (utf != nullptr) {
        out = utf;
        attach.env->ReleaseStringUTFChars(static_cast<jstring>(text), utf);
    }
    attach.env->DeleteLocalRef(text);
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    return true;
}

bool EntityHandle::Health(float& health, float& absorption) const {
    if (compat::UsesModernRuntime()) { return compat::mc263::ReadEntityFloat(obj, compat::mc263::EntityFloat::Health, health) && compat::mc263::ReadEntityFloat(obj, compat::mc263::EntityFloat::Absorption, absorption); }
    if (compat::IsLegacyLunar()) { return compat::lunar::ReadEntityFloat(obj, compat::lunar::EntityFloat::Health, health) && compat::lunar::ReadEntityFloat(obj, compat::lunar::EntityFloat::Absorption, absorption); }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env) || g_entity.getHealth == nullptr ||
        g_entity.getAbsorption == nullptr)
        return false;
    if (!attach.env->IsInstanceOf(static_cast<jobject>(obj), g_entity.livingClass)) {
        if (attach.env->ExceptionCheck())
            attach.env->ExceptionClear();
        health = 0.0f;
        absorption = 0.0f;
        return true;
    }
    const jfloat hp = attach.env->CallFloatMethod(static_cast<jobject>(obj), g_entity.getHealth);
    const jfloat extra = attach.env->CallFloatMethod(static_cast<jobject>(obj), g_entity.getAbsorption);
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    health = static_cast<float>(hp);
    absorption = static_cast<float>(extra);
    return true;
}

// LivingEntity.getMaxHealth: what the bar's full width stands for. Servers move it
// -- a mini-game can hand out 40 or 6 -- so it is read rather than assumed, which
// is the difference between a bar that reads and one that lies.
bool EntityHandle::MaxHealth(float& out) const {
    if (compat::UsesModernRuntime()) { return compat::mc263::ReadEntityFloat(obj, compat::mc263::EntityFloat::MaxHealth, out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::ReadEntityFloat(obj, compat::lunar::EntityFloat::MaxHealth, out); }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env) || g_entity.getMaxHealth == nullptr)
        return false;
    if (!attach.env->IsInstanceOf(static_cast<jobject>(obj), g_entity.livingClass)) {
        if (attach.env->ExceptionCheck())
            attach.env->ExceptionClear();
        return false;
    }
    const jfloat value = attach.env->CallFloatMethod(static_cast<jobject>(obj), g_entity.getMaxHealth);
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    out = static_cast<float>(value);
    return true;
}

bool EntityHandle::HurtTime(int& out) const {
    if (compat::UsesModernRuntime()) { return compat::mc263::ReadEntityInt(obj, compat::mc263::EntityInt::Hurt, out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::ReadEntityInt(obj, compat::lunar::EntityInt::Hurt, out); }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env) || g_entity.hurtTime == nullptr)
        return false;
    out = attach.env->GetIntField(static_cast<jobject>(obj), g_entity.hurtTime);
    const bool clean = !attach.env->ExceptionCheck();
    if (attach.env->ExceptionCheck())
        attach.env->ExceptionClear();
    return clean;
}

bool EntityHandle::DeathTime(int& out) const {
    if (compat::UsesModernRuntime()) { return compat::mc263::ReadEntityInt(obj, compat::mc263::EntityInt::Death, out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::ReadEntityInt(obj, compat::lunar::EntityInt::Death, out); }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env) || g_entity.deathTime == nullptr)
        return false;
    out = attach.env->GetIntField(static_cast<jobject>(obj), g_entity.deathTime);
    const bool clean = !attach.env->ExceptionCheck();
    if (attach.env->ExceptionCheck())
        attach.env->ExceptionClear();
    return clean;
}

bool EntityHandle::DistanceSqTo(const EntityHandle& other, double& out) const {
    if (compat::UsesModernRuntime()) { rotation::Vec3 a, b; if (!Position(a) || !other.Position(b)) return false; const double x = a.x-b.x, y = a.y-b.y, z = a.z-b.z; out = x*x+y*y+z*z; return true; }
    if (compat::IsLegacyLunar()) { rotation::Vec3 a, b; if (!Position(a) || !other.Position(b)) return false; const double x = a.x-b.x, y = a.y-b.y, z = a.z-b.z; out = x*x+y*y+z*z; return true; }
    JniAttach attach;
    if (!attach || obj == nullptr || other.obj == nullptr || !EnsureResolved(attach.env) ||
        g_entity.distanceSqr == nullptr)
        return false;
    const jdouble value = attach.env->CallDoubleMethod(static_cast<jobject>(obj), g_entity.distanceSqr,
                                                       static_cast<jobject>(other.obj));
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        return false;
    }
    out = static_cast<double>(value);
    return true;
}

bool EntityHandle::TickCount(int& out) const {
    if (compat::UsesModernRuntime()) { return compat::mc263::ReadEntityInt(obj, compat::mc263::EntityInt::Ticks, out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::ReadEntityInt(obj, compat::lunar::EntityInt::Ticks, out); }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env))
        return false;
    out = attach.env->GetIntField(static_cast<jobject>(obj), g_entity.tickCount);
    const bool clean = !attach.env->ExceptionCheck();
    if (attach.env->ExceptionCheck())
        attach.env->ExceptionClear();
    return clean;
}

bool EntityHandle::OnGround(bool& out) const {
    if (compat::UsesModernRuntime()) { return compat::mc263::ReadEntityBool(obj, compat::mc263::EntityBool::OnGround, out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::ReadEntityBool(obj, compat::lunar::EntityBool::OnGround, out); }
    JniAttach attach;
    if (!attach || obj == nullptr || !EnsureResolved(attach.env))
        return false;
    out = attach.env->GetBooleanField(static_cast<jobject>(obj), g_entity.onGround) != JNI_FALSE;
    const bool clean = !attach.env->ExceptionCheck();
    if (attach.env->ExceptionCheck())
        attach.env->ExceptionClear();
    return clean;
}

EntityHandle AdoptEntity(void* localRef) {
    if (localRef == nullptr)
        return EntityHandle();
    JniAttach attach;
    if (!attach)
        return EntityHandle();
    EntityHandle handle;
    handle.obj = attach.env->NewGlobalRef(static_cast<jobject>(localRef));
    if (attach.env->ExceptionCheck()) {
        attach.env->ExceptionClear();
        handle.obj = nullptr;
    }
    return handle;
}

EntityHandle PlayerEntity() {
    if (compat::UsesModernRuntime()) { return compat::mc263::PlayerEntity(); }
    if (compat::IsLegacyLunar()) { return compat::lunar::PlayerEntity(); }
    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env))
        return EntityHandle();
    JNIEnv* env = attach.env;
    jobject client = Client(env);
    if (client == nullptr)
        return EntityHandle();
    jobject player = env->GetObjectField(client, g_entity.player);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return EntityHandle();
    }
    if (player == nullptr)
        return EntityHandle();
    EntityHandle handle = AdoptEntity(player);
    env->DeleteLocalRef(player);
    return handle;
}

EntityHandle LevelEntity() {
    if (compat::UsesModernRuntime()) { return compat::mc263::LevelEntity(); }
    if (compat::IsLegacyLunar()) { return compat::lunar::LevelEntity(); }
    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env))
        return EntityHandle();
    JNIEnv* env = attach.env;
    jobject level = LevelLocal(env);
    if (level == nullptr)
        return EntityHandle();
    EntityHandle handle = AdoptEntity(level);
    env->DeleteLocalRef(level);
    return handle;
}

EntityHandle EntityById(int id) {
    if (compat::UsesModernRuntime()) { return compat::mc263::EntityById(id); }
    if (compat::IsLegacyLunar()) { return compat::lunar::EntityById(id); }
    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env) || g_entity.levelGetEntity == nullptr)
        return EntityHandle();
    JNIEnv* env = attach.env;

    jobject level = LevelLocal(env);
    if (level == nullptr)
        return EntityHandle();

    jobject entity = env->CallObjectMethod(level, g_entity.levelGetEntity, static_cast<jint>(id));
    env->DeleteLocalRef(level);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return EntityHandle();
    }
    if (entity == nullptr)
        return EntityHandle();

    EntityHandle handle = AdoptEntity(entity);
    env->DeleteLocalRef(entity);
    return handle;
}

bool WorldPlayers(std::vector<EntityHandle>& out) {
    if (compat::UsesModernRuntime()) { return compat::mc263::WorldPlayers(out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::WorldPlayers(out); }
    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env))
        return false;
    JNIEnv* env = attach.env;
    jobject level = LevelLocal(env);
    if (level == nullptr)
        return false;
    jobject list = env->CallObjectMethod(level, g_entity.players);
    env->DeleteLocalRef(level);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (list == nullptr)
        return false;
    ReadList(env, list, out);
    env->DeleteLocalRef(list);
    return true;
}

bool EntityTypes(const EntityHandle& entity, unsigned& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::EntityTypes(entity, out);
    if (compat::IsLegacyLunar()) return compat::lunar::EntityTypes(entity, out);
    out = 0;
    if (!entity.Valid())
        return false;

    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env) || !EnsureKindResolved(attach.env))
        return false;

    JNIEnv* env = attach.env;
    jobject obj = static_cast<jobject>(entity.raw());

    if (env->IsInstanceOf(obj, g_entity.livingClass) == JNI_TRUE)
        out |= kTypeLiving;
    if (env->IsInstanceOf(obj, g_entity.playerClass) == JNI_TRUE)
        out |= kTypePlayer;
    if (env->IsInstanceOf(obj, g_kind.villager) == JNI_TRUE)
        out |= kTypeVillager;
    if (env->IsInstanceOf(obj, g_kind.monster) == JNI_TRUE ||
        env->IsInstanceOf(obj, g_kind.slime) == JNI_TRUE ||
        env->IsInstanceOf(obj, g_kind.dragon) == JNI_TRUE ||
        env->IsInstanceOf(obj, g_kind.dragonPart) == JNI_TRUE)
        out |= kTypeMonster;
    if (env->IsInstanceOf(obj, g_kind.animal) == JNI_TRUE ||
        env->IsInstanceOf(obj, g_kind.ambient) == JNI_TRUE ||
        env->IsInstanceOf(obj, g_kind.waterAnimal) == JNI_TRUE ||
        env->IsInstanceOf(obj, g_kind.horse) == JNI_TRUE)
        out |= kTypeAnimal;

    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

bool WorldEntities(const EntityHandle& source, const Box& area, std::vector<EntityHandle>& out) {
    if (compat::UsesModernRuntime()) { return compat::mc263::WorldEntities(source, area, out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::WorldEntities(source, area, out); }
    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env) || !source.Valid())
        return false;
    JNIEnv* env = attach.env;
    jobject level = LevelLocal(env);
    if (level == nullptr)
        return false;
    jobject box = MakeBox(env, area);
    if (box == nullptr) {
        env->DeleteLocalRef(level);
        return false;
    }
    jobject list = env->CallObjectMethod(level, g_entity.getEntities,
                                         static_cast<jobject>(source.raw()), box);
    env->DeleteLocalRef(box);
    env->DeleteLocalRef(level);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (list == nullptr)
        return false;
    ReadList(env, list, out);
    env->DeleteLocalRef(list);
    return true;
}

bool WorldClip(const rotation::Vec3& from, const rotation::Vec3& to, const EntityHandle& source,
               BlockHitResultInfo& out, ClipMode mode) {
    if (compat::UsesModernRuntime()) return compat::mc263::WorldClip(from, to, source, out, mode);
    if (compat::IsLegacyLunar()) return compat::lunar::WorldClip(from, to, source, out, mode);
    out.kind = HitKind::Miss;
    out.location = rotation::Vec3{0.0, 0.0, 0.0};
    out.blockPos = BlockPos{};
    out.direction = -1;
    out.hasBlockPos = false;

    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env) || !source.Valid())
        return false;
    JNIEnv* env = attach.env;

    jobject level = LevelLocal(env);
    if (level == nullptr)
        return false;

    jobject start = MakeVec3(env, from);
    jobject end = MakeVec3(env, to);
    if (start == nullptr || end == nullptr) {
        if (start) env->DeleteLocalRef(start);
        if (end) env->DeleteLocalRef(end);
        env->DeleteLocalRef(level);
        return false;
    }

    jobject blockMode=g_entity.clipOutline,fluidMode=g_entity.clipNone;
    if(mode!=ClipMode::Outline) {
        const char* clsName=mode==ClipMode::Collision?kMappedClipContextBlockDot:kMappedClipContextFluidDot;
        const char* sig=mode==ClipMode::Collision?kClipBlockSig:kClipFluidSig;
        auto loader=GetMinecraftClassLoader(env);
        auto cls=nova_base::loadOrFind(env,loader,clsName,sig);
        if(loader)env->DeleteGlobalRef(loader);
        auto id=cls?env->GetStaticFieldID(cls,mode==ClipMode::Collision?"COLLIDER":"SOURCE_ONLY",sig):nullptr;
        auto value=id?env->GetStaticObjectField(cls,id):nullptr;
        if(cls)env->DeleteLocalRef(cls);
        if(env->ExceptionCheck() || !value) {
            env->ExceptionClear();env->DeleteLocalRef(start);env->DeleteLocalRef(end);env->DeleteLocalRef(level);return false;
        }
        if(mode==ClipMode::Collision)blockMode=value;else fluidMode=value;
    }
    jobject context = env->NewObject(g_entity.clipCtxClass, g_entity.clipCtxCtor, start, end,
                                     blockMode, fluidMode,
                                     static_cast<jobject>(source.raw()));
    if(blockMode!=g_entity.clipOutline)env->DeleteLocalRef(blockMode);
    if(fluidMode!=g_entity.clipNone)env->DeleteLocalRef(fluidMode);
    env->DeleteLocalRef(start);
    env->DeleteLocalRef(end);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        if (context) env->DeleteLocalRef(context);
        env->DeleteLocalRef(level);
        return false;
    }

    jobject result = env->CallObjectMethod(level, g_entity.clip, context);
    if (context)
        env->DeleteLocalRef(context);
    env->DeleteLocalRef(level);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        if (result) env->DeleteLocalRef(result);
        return false;
    }
    if (result == nullptr)
        return false;

    jobject type = env->CallObjectMethod(result, g_entity.hitGetType);
    jobject location = env->CallObjectMethod(result, g_entity.hitGetLocation);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        if (type) env->DeleteLocalRef(type);
        if (location) env->DeleteLocalRef(location);
        env->DeleteLocalRef(result);
        return false;
    }

    const jint kind = type ? env->CallIntMethod(type, g_entity.ordinal) : 0;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        out.kind = HitKind::Miss;
    } else {
        out.kind = kind == 1 ? HitKind::Block : (kind == 2 ? HitKind::Entity : HitKind::Miss);
    }

    ReadVec3(env, location, out.location);

    if (out.kind == HitKind::Block && g_entity.hitGetBlockPos != nullptr &&
        g_entity.vec3iGetX != nullptr && g_entity.vec3iGetY != nullptr &&
        g_entity.vec3iGetZ != nullptr && g_entity.hitGetDirection != nullptr &&
        g_entity.ordinal != nullptr) {
        jobject blockPos = env->CallObjectMethod(result, g_entity.hitGetBlockPos);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        } else if (blockPos != nullptr) {
            const jint bx = env->CallIntMethod(blockPos, g_entity.vec3iGetX);
            const jint by = env->CallIntMethod(blockPos, g_entity.vec3iGetY);
            const jint bz = env->CallIntMethod(blockPos, g_entity.vec3iGetZ);
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
            } else {
                out.blockPos = BlockPos{static_cast<int>(bx), static_cast<int>(by),
                                        static_cast<int>(bz)};
                out.hasBlockPos = true;
            }
            env->DeleteLocalRef(blockPos);
        }

        jobject direction = env->CallObjectMethod(result, g_entity.hitGetDirection);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        } else if (direction != nullptr) {
            const jint face = env->CallIntMethod(direction, g_entity.ordinal);
            if (env->ExceptionCheck())
                env->ExceptionClear();
            else
                out.direction = static_cast<int>(face);
            env->DeleteLocalRef(direction);
        }
    }

    if (type) env->DeleteLocalRef(type);
    if (location) env->DeleteLocalRef(location);
    env->DeleteLocalRef(result);
    return true;
}

EntityHandle MakeEntityHit(const EntityHandle& entity, const rotation::Vec3& location) {
    if (compat::UsesModernRuntime()) return compat::mc263::MakeEntityHit(entity, location);
    if (compat::IsLegacyLunar()) return compat::lunar::MakeEntityHit(entity, location);
    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env) || !entity.Valid())
        return EntityHandle();
    JNIEnv* env = attach.env;
    jobject vec = MakeVec3(env, location);
    if (vec == nullptr)
        return EntityHandle();
    jobject hit = env->NewObject(g_entity.entityHitClass, g_entity.entityHitCtor,
                                 static_cast<jobject>(entity.raw()), vec);
    env->DeleteLocalRef(vec);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        if (hit) env->DeleteLocalRef(hit);
        return EntityHandle();
    }
    if (hit == nullptr)
        return EntityHandle();
    EntityHandle handle = AdoptEntity(hit);
    env->DeleteLocalRef(hit);
    return handle;
}

bool BoxClip(const Box& box, const rotation::Vec3& from, const rotation::Vec3& to, bool& hit,
             rotation::Vec3& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::BoxClip(box, from, to, hit, out);
    if (compat::IsLegacyLunar()) return compat::lunar::BoxClip(box, from, to, hit, out);
    hit = false;
    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject aabb = MakeBox(env, box);
    jobject start = MakeVec3(env, from);
    jobject end = MakeVec3(env, to);
    if (aabb == nullptr || start == nullptr || end == nullptr) {
        if (aabb) env->DeleteLocalRef(aabb);
        if (start) env->DeleteLocalRef(start);
        if (end) env->DeleteLocalRef(end);
        return false;
    }

    jobject optional = env->CallObjectMethod(aabb, g_entity.aabbClip, start, end);
    env->DeleteLocalRef(aabb);
    env->DeleteLocalRef(start);
    env->DeleteLocalRef(end);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        if (optional) env->DeleteLocalRef(optional);
        return false;
    }
    if (optional == nullptr)
        return false;

    if (env->CallBooleanMethod(optional, g_entity.optionalIsPresent)) {
        jobject value = env->CallObjectMethod(optional, g_entity.optionalGet);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        } else {
            hit = ReadVec3(env, value, out);
        }
        if (value)
            env->DeleteLocalRef(value);
    }
    if (env->ExceptionCheck())
        env->ExceptionClear();

    env->DeleteLocalRef(optional);
    return true;
}

bool BoxContains(const Box& box, const rotation::Vec3& point) {
    if (compat::UsesModernRuntime())
        return point.x >= box.minX && point.x < box.maxX &&
               point.y >= box.minY && point.y < box.maxY &&
               point.z >= box.minZ && point.z < box.maxZ;
    if (compat::IsLegacyLunar())
        return point.x >= box.minX && point.x < box.maxX &&
               point.y >= box.minY && point.y < box.maxY &&
               point.z >= box.minZ && point.z < box.maxZ;
    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env))
        return false;
    JNIEnv* env = attach.env;
    jobject aabb = MakeBox(env, box);
    jobject vec = MakeVec3(env, point);
    if (aabb == nullptr || vec == nullptr) {
        if (aabb) env->DeleteLocalRef(aabb);
        if (vec) env->DeleteLocalRef(vec);
        return false;
    }
    const jboolean result = env->CallBooleanMethod(aabb, g_entity.aabbContains, vec);
    env->DeleteLocalRef(aabb);
    env->DeleteLocalRef(vec);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return result != JNI_FALSE;
}

bool Vec3DirectionFromRotation(float pitch, float yaw, rotation::Vec3& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::DirectionFromRotation(pitch, yaw, out);
    if (compat::IsLegacyLunar()) return compat::lunar::DirectionFromRotation(pitch, yaw, out);
    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env))
        return false;
    JNIEnv* env = attach.env;
    jobject vec = env->CallStaticObjectMethod(g_entity.vecClass, g_entity.vecDirFromRot,
                                              static_cast<jfloat>(pitch), static_cast<jfloat>(yaw));
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        if (vec) env->DeleteLocalRef(vec);
        return false;
    }
    const bool clean = ReadVec3(env, vec, out);
    if (vec)
        env->DeleteLocalRef(vec);
    return clean;
}

Box BoxInflate(const Box& box, double amount) {
    if (compat::UsesModernRuntime())
        return {box.minX - amount, box.minY - amount, box.minZ - amount,
                box.maxX + amount, box.maxY + amount, box.maxZ + amount};
    if (compat::IsLegacyLunar())
        return {box.minX - amount, box.minY - amount, box.minZ - amount,
                box.maxX + amount, box.maxY + amount, box.maxZ + amount};
    Box out = box;
    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env))
        return out;
    JNIEnv* env = attach.env;
    jobject aabb = MakeBox(env, box);
    if (aabb == nullptr)
        return out;
    jobject grown = env->CallObjectMethod(aabb, g_entity.aabbInflate, static_cast<jdouble>(amount));
    env->DeleteLocalRef(aabb);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        if (grown) env->DeleteLocalRef(grown);
        return out;
    }
    ReadBox(env, grown, out);
    if (grown)
        env->DeleteLocalRef(grown);
    return out;
}

Box BoxExpandTowards(const Box& box, const rotation::Vec3& delta) {
    if (compat::UsesModernRuntime())
        return {box.minX + std::min(0.0, delta.x), box.minY + std::min(0.0, delta.y),
                box.minZ + std::min(0.0, delta.z), box.maxX + std::max(0.0, delta.x),
                box.maxY + std::max(0.0, delta.y), box.maxZ + std::max(0.0, delta.z)};
    if (compat::IsLegacyLunar())
        return {box.minX + std::min(0.0, delta.x), box.minY + std::min(0.0, delta.y),
                box.minZ + std::min(0.0, delta.z), box.maxX + std::max(0.0, delta.x),
                box.maxY + std::max(0.0, delta.y), box.maxZ + std::max(0.0, delta.z)};
    Box out = box;
    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env))
        return out;
    JNIEnv* env = attach.env;
    jobject aabb = MakeBox(env, box);
    if (aabb == nullptr)
        return out;
    jobject grown = env->CallObjectMethod(aabb, g_entity.aabbExpand, delta.x, delta.y, delta.z);
    env->DeleteLocalRef(aabb);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        if (grown) env->DeleteLocalRef(grown);
        return out;
    }
    ReadBox(env, grown, out);
    if (grown)
        env->DeleteLocalRef(grown);
    return out;
}

// getId and fallDistance are not part of the main handle set -- only the velocity
// module reads them -- so they resolve lazily and leave the big Resolve above
// alone.
struct VelocityEntityHandles {
    jmethodID getId = nullptr;
    jfieldID  fallDistance = nullptr;
};

VelocityEntityHandles g_velocityEntity;

bool EnsureVelocityResolved(JNIEnv* env) {
    if (!env)
        return false;
    if (g_velocityEntity.getId && g_velocityEntity.fallDistance)
        return true;

    jobject loader = GetMinecraftClassLoader(env);
    jclass entity = nova_base::loadOrFind(env, loader, kEntityDot, kEntitySig);
    if (loader)
        env->DeleteGlobalRef(loader);
    if (entity == nullptr)
        return false;

    jmethodID getId = env->GetMethodID(entity, kMappedEntityId, kListSizeDesc);
    jfieldID fallDistance = env->GetFieldID(entity, kMappedFallDistance, kMappedPrimitiveFloatSig);
    if (env->ExceptionCheck())
        env->ExceptionClear();
    env->DeleteLocalRef(entity);

    if (!getId || !fallDistance) {
        LogTo("Base: entity velocity members unavailable (id=%d fall=%d)", getId != nullptr,
              fallDistance != nullptr);
        return false;
    }

    g_velocityEntity.getId = getId;
    g_velocityEntity.fallDistance = fallDistance;
    return true;
}

bool EntityId(const EntityHandle& entity, int& out) {
    if (compat::UsesModernRuntime()) { return compat::mc263::ReadEntityInt(entity.raw(), compat::mc263::EntityInt::Id, out); }
    if (compat::IsLegacyLunar()) { return compat::lunar::ReadEntityInt(entity.raw(), compat::lunar::EntityInt::Id, out); }
    out = 0;
    if (!entity.Valid())
        return false;

    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env) || !EnsureVelocityResolved(attach.env))
        return false;

    JNIEnv* env = attach.env;
    const jint id = env->CallIntMethod(static_cast<jobject>(entity.raw()), g_velocityEntity.getId);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    out = static_cast<int>(id);
    return true;
}

bool PlayerFallDistance(float& out) {
    if (compat::UsesModernRuntime()) { auto player = PlayerEntity(); return compat::mc263::ReadEntityFloat(player.raw(), compat::mc263::EntityFloat::FallDistance, out); }
    if (compat::IsLegacyLunar()) { auto player = PlayerEntity(); return compat::lunar::ReadEntityFloat(player.raw(), compat::lunar::EntityFloat::FallDistance, out); }
    out = 0.0f;

    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env) || !EnsureVelocityResolved(attach.env))
        return false;

    JNIEnv* env = attach.env;
    jobject client = Client(env);
    if (client == nullptr)
        return false;

    jobject player = env->GetObjectField(client, g_entity.player);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (player == nullptr)
        return false;

    const jfloat value = env->GetFloatField(player, g_velocityEntity.fallDistance);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    out = static_cast<float>(value);
    return true;
}

float FrameTime() {
    if (compat::UsesModernRuntime()) { return compat::mc263::FrameTime(); }
    if (compat::IsLegacyLunar()) { return compat::lunar::FrameTime(); }
    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env))
        return 0.0f;
    JNIEnv* env = attach.env;
    jobject client = Client(env);
    if (client == nullptr)
        return 0.0f;
    const jfloat result = env->CallFloatMethod(client, g_entity.frameTime);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return 0.0f;
    }
    return static_cast<float>(result);
}

int PlayerTickCount() {
    if (compat::UsesModernRuntime()) { int value = 0; auto player = PlayerEntity(); player.TickCount(value); return value; }
    if (compat::IsLegacyLunar()) { int value = 0; auto player = PlayerEntity(); player.TickCount(value); return value; }
    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env))
        return 0;
    JNIEnv* env = attach.env;
    jobject client = Client(env);
    if (client == nullptr)
        return 0;
    jobject player = env->GetObjectField(client, g_entity.player);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return 0;
    }
    if (player == nullptr)
        return 0;
    const jint result = env->GetIntField(player, g_entity.tickCount);
    const bool clean = !env->ExceptionCheck();
    if (env->ExceptionCheck())
        env->ExceptionClear();
    env->DeleteLocalRef(player);
    return clean ? static_cast<int>(result) : 0;
}

int PlayerHurtTime() {
    if (compat::UsesModernRuntime()) { int value = 0; auto player = PlayerEntity(); player.HurtTime(value); return value; }
    if (compat::IsLegacyLunar()) { int value = 0; auto player = PlayerEntity(); player.HurtTime(value); return value; }
    JniAttach attach;
    if (!attach || !EnsureResolved(attach.env) || g_entity.hurtTime == nullptr)
        return 0;
    JNIEnv* env = attach.env;
    jobject client = Client(env);
    if (client == nullptr)
        return 0;
    jobject player = env->GetObjectField(client, g_entity.player);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return 0;
    }
    if (player == nullptr)
        return 0;
    const jint result = env->GetIntField(player, g_entity.hurtTime);
    const bool clean = !env->ExceptionCheck();
    if (env->ExceptionCheck())
        env->ExceptionClear();
    env->DeleteLocalRef(player);
    return clean ? static_cast<int>(result) : 0;
}

}
