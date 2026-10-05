#include "minecraft/java/api/game_chest.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/jni_lookup.h"

// The chest ESP's slice of the world: which chests are loaded, where the camera
// is, and the one packet that says a chest has been opened.
//
// A fifth self-contained handle set, for the reason the earlier ones are. The
// chunk walk is the part worth explaining: Naven streams every block entity in
// every loaded chunk each tick, and from here that is Level.getChunk ->
// LevelChunk.getBlockEntities -> Map.values -> iterate. A lot of handles for one
// feature, so all of it is resolved together and a missing piece stands the layer
// down rather than letting it half-work.
//
// One deliberate difference from the original: Naven filters chunk positions with
// Level.hasChunk before asking for them. On a client level an unloaded position
// comes back as the shared empty chunk rather than null, whose block entity map is
// empty, so the filter buys nothing here and is left out.

namespace nova::java {
namespace {

#define NOVA_FORGE1201_CHEST
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_CHEST

// getValue and hasProperty live on StateHolder, which BlockState extends; JNI
// walks up the chain to find them.

// ChestType declares SINGLE, LEFT, RIGHT, so the ordinals tell the two halves of a
// double chest apart without holding the enum constants. The left half is skipped
// because the right half reports the pair.
constexpr jint kChestTypeSingle = 0;
constexpr jint kChestTypeLeft   = 1;

struct ChestHandles {
    jclass    minecraft = nullptr;
    jclass    chestEntity = nullptr;
    jclass    chestBlock = nullptr;
    jclass    blockEvent = nullptr;
    jclass    pos = nullptr;

    jmethodID getInstance = nullptr;
    jmethodID getMainCamera = nullptr;
    jmethodID getFov = nullptr;
    jmethodID getProjection = nullptr;
    jmethodID optionGet = nullptr;
    jmethodID getWindow = nullptr;
    jmethodID windowWidth = nullptr;
    jmethodID windowHeight = nullptr;
    jmethodID booleanValue = nullptr;
    jmethodID cameraPosition = nullptr;
    jmethodID cameraYRot = nullptr;
    jmethodID cameraXRot = nullptr;
    jmethodID renderDistance = nullptr;
    jmethodID getChunk = nullptr;
    jmethodID getBlockEntities = nullptr;
    jmethodID mapValues = nullptr;
    jmethodID collectionIterator = nullptr;
    jmethodID iteratorHasNext = nullptr;
    jmethodID iteratorNext = nullptr;
    jmethodID getBlockState = nullptr;
    jmethodID getBlockPos = nullptr;
    jmethodID getValue = nullptr;
    jmethodID hasProperty = nullptr;
    jmethodID connectedDirection = nullptr;
    jmethodID getShape = nullptr;
    jmethodID shapeBounds = nullptr;
    jmethodID shapeEmpty = nullptr;
    jmethodID levelGetState = nullptr;
    jmethodID posCtor = nullptr;
    jmethodID posRelative = nullptr;
    jmethodID posX = nullptr;
    jmethodID posY = nullptr;
    jmethodID posZ = nullptr;
    jmethodID ordinal = nullptr;
    jmethodID eventGetBlock = nullptr;
    jmethodID eventGetB0 = nullptr;
    jmethodID eventGetB1 = nullptr;
    jmethodID eventGetPos = nullptr;

    jfieldID  gameRenderer = nullptr;
    jfieldID  options = nullptr;
    jfieldID  level = nullptr;
    jfieldID  chestType = nullptr;
    jfieldID  bobView = nullptr;
    jfieldID  player = nullptr;
    jfieldID  walkDist = nullptr;
    jfieldID  walkDistO = nullptr;
    jfieldID  bob = nullptr;
    jfieldID  oBob = nullptr;
    jfieldID  matrix[16] = {};
    jfieldID  vecX = nullptr;
    jfieldID  vecY = nullptr;
    jfieldID  vecZ = nullptr;
    jfieldID  aabbMinX = nullptr;
    jfieldID  aabbMinY = nullptr;
    jfieldID  aabbMinZ = nullptr;
    jfieldID  aabbMaxX = nullptr;
    jfieldID  aabbMaxY = nullptr;
    jfieldID  aabbMaxZ = nullptr;

    jobject   chestBlockObj = nullptr;
    jobject   trappedChestObj = nullptr;

    // The ChestBlock.TYPE property itself, which is the key hasProperty and
    // getValue are asked about. Held as a global reference because it is passed to
    // the game on every chest of every tick.
    jobject   chestTypeProperty = nullptr;

    bool ok = false;
};

ChestHandles g_chest;
bool g_chestLogged = false;
bool g_chestResolveFailed = false;
std::chrono::steady_clock::time_point g_chestRetryAt;

constexpr int kResolveRetrySeconds = 10;

// A failed GetMethodID or GetFieldID leaves a NoSuchMethodError pending, and the
// JVM then fails every lookup after it -- which turns one absent member into a
// cascade and hides which one was really missing. Every lookup below goes through
// one of these, so each answer is its own.
void Settle(JNIEnv* env) {
    if (env->ExceptionCheck())
        env->ExceptionClear();
}

jmethodID Method(JNIEnv* env, jclass cls, const char* name, const char* desc) {
    if (cls == nullptr)
        return nullptr;
    jmethodID id = env->GetMethodID(cls, name, desc);
    Settle(env);
    return id;
}

jmethodID StaticMethod(JNIEnv* env, jclass cls, const char* name, const char* desc) {
    if (cls == nullptr)
        return nullptr;
    jmethodID id = env->GetStaticMethodID(cls, name, desc);
    Settle(env);
    return id;
}

jfieldID Field(JNIEnv* env, jclass cls, const char* name, const char* desc) {
    if (cls == nullptr)
        return nullptr;
    jfieldID id = env->GetFieldID(cls, name, desc);
    Settle(env);
    return id;
}

jfieldID StaticField(JNIEnv* env, jclass cls, const char* name, const char* desc) {
    if (cls == nullptr)
        return nullptr;
    jfieldID id = env->GetStaticFieldID(cls, name, desc);
    Settle(env);
    return id;
}

bool ResolveChest(JNIEnv* env) {
    if (!env)
        return false;
    if (g_chest.ok)
        return true;
    if (g_chestResolveFailed && std::chrono::steady_clock::now() < g_chestRetryAt)
        return false;

    jobject loader = GetMinecraftClassLoader(env);
    jclass minecraft = nova_base::loadOrFind(env, loader, kMinecraftDot, kMinecraftSig);
    jclass gameRenderer = nova_base::loadOrFind(env, loader, kGameRendererDot, kGameRendererSig);
    jclass camera = nova_base::loadOrFind(env, loader, kCameraDot, kCameraSig);
    jclass level = nova_base::loadOrFind(env, loader, kLevelDot, kLevelSig);
    jclass chunk = nova_base::loadOrFind(env, loader, kChunkDot, kChunkSig);
    jclass blockEntity = nova_base::loadOrFind(env, loader, kBlockEntityDot, kBlockEntitySig);
    jclass chestEntity = nova_base::loadOrFind(env, loader, kChestEntityDot, kChestEntitySig);
    jclass blockState = nova_base::loadOrFind(env, loader, kBlockStateDot, kBlockStateSig);
    jclass shape = nova_base::loadOrFind(env, loader, kShapeDot, kShapeSig);
    jclass chestBlock = nova_base::loadOrFind(env, loader, kChestBlockDot, kChestBlockSig);
    jclass blocks = nova_base::loadOrFind(env, loader, kBlocksDot, kBlocksSig);
    jclass options = nova_base::loadOrFind(env, loader, kMappedOptionsDot,
                                           kOptionsSig);
    jclass pos = nova_base::loadOrFind(env, loader, kPosDot, kPosSig);
    jclass vec = nova_base::loadOrFind(env, loader, kVecDot, kVecSig);
    jclass aabb = nova_base::loadOrFind(env, loader, kAabbDot, kAabbSig);
    jclass map = nova_base::loadOrFind(env, loader, kMappedMapDot, kMapSig);
    jclass collection = nova_base::loadOrFind(env, loader, kMappedCollectionDot, kCollectionSig);
    jclass iterator = nova_base::loadOrFind(env, loader, kMappedIteratorDot, kIteratorSig);
    jclass enumClass = nova_base::loadOrFind(env, loader, kMappedEnumDot, kMappedEnumSig);
    jclass blockEvent = nova_base::loadOrFind(env, loader, kBlockEventDot, kBlockEventSig);
    jclass matrix = nova_base::loadOrFind(env, loader, kMatrixDot, kMatrixSig);
    jclass optionInstance = nova_base::loadOrFind(env, loader, kMappedOptionInstanceDot,
                                                  kOptionInstanceSig);
    jclass window = nova_base::loadOrFind(env, loader, kMappedWindowDot,
                                          kMappedWindowSig);
    jclass booleanClass = nova_base::loadOrFind(env, loader, kMappedBooleanDot,
                                                kMappedBooleanSig);
    jclass localPlayer = nova_base::loadOrFind(env, loader,
                                               kMappedLocalPlayerDot,
                                               kLocalPlayerSig);
    if (loader)
        env->DeleteGlobalRef(loader);

    jmethodID getInstance = StaticMethod(env, minecraft, kGetInstance, kGetInstanceDesc);
    jmethodID getMainCamera = Method(env, gameRenderer, kGetMainCamera, kGetMainCameraDesc);
    jmethodID getFov = Method(env, gameRenderer, kGetFov, kGetFovDesc);
    jmethodID getProjection = Method(env, gameRenderer, kGetProjection, kGetProjectionDesc);
    jmethodID optionGet = Method(env, optionInstance, kOptionGet, kOptionGetDesc);
    jmethodID getWindow = Method(env, minecraft, kGetWindow, kGetWindowDesc);
    jmethodID windowWidth = Method(env, window, kWindowWidth, kIntDesc);
    jmethodID windowHeight = Method(env, window, kWindowHeight, kIntDesc);
    jmethodID booleanValue =
        Method(env, booleanClass, kBooleanValue, kBoolDesc);
    jmethodID cameraPosition = Method(env, camera, kCameraPosition, kCameraPositionDesc);
    jmethodID cameraYRot = Method(env, camera, kCameraYRot, kFloatDesc);
    jmethodID cameraXRot = Method(env, camera, kCameraXRot, kFloatDesc);
    jmethodID renderDistance = Method(env, options, kRenderDistance, kIntDesc);
    jmethodID getChunk = Method(env, level, kGetChunk, kGetChunkDesc);
    jmethodID getBlockEntities = Method(env, chunk, kGetBlockEntities, kGetBlockEntitiesDesc);
    jmethodID mapValues = Method(env, map, kMapValues, kMapValuesDesc);
    jmethodID collectionIterator = Method(env, collection, kIterator, kIteratorDesc);
    jmethodID iteratorHasNext = Method(env, iterator, kHasNext, kHasNextDesc);
    jmethodID iteratorNext = Method(env, iterator, kNext, kNextDesc);
    jmethodID getBlockState = Method(env, blockEntity, kGetBlockState, kGetBlockStateDesc);
    jmethodID getBlockPos = Method(env, blockEntity, kGetBlockPos, kGetBlockPosDesc);
    jmethodID getValue = Method(env, blockState, kGetValue, kGetValueDesc);
    jmethodID hasProperty = Method(env, blockState, kHasProperty, kHasPropertyDesc);
    jmethodID connectedDirection =
        StaticMethod(env, chestBlock, kConnectedDir, kConnectedDirDesc);
    jmethodID getShape = Method(env, blockState, kGetShape, kGetShapeDesc);
    jmethodID shapeBounds = Method(env, shape, kShapeBounds, kShapeBoundsDesc);
    jmethodID shapeEmpty = Method(env, shape, kShapeEmpty, kBoolDesc);
    jmethodID levelGetState = Method(env, level, kLevelGetState, kLevelGetStateDesc);
    jmethodID posCtor = Method(env, pos, kPosCtor, kPosCtorDesc);
    jmethodID posRelative = Method(env, pos, kRelative, kRelativeDesc);
    jmethodID posX = Method(env, pos, kPosX, kIntDesc);
    jmethodID posY = Method(env, pos, kPosY, kIntDesc);
    jmethodID posZ = Method(env, pos, kPosZ, kIntDesc);
    jmethodID ordinal = Method(env, enumClass, kOrdinal, kOrdinalDesc);
    jmethodID eventGetBlock = Method(env, blockEvent, kEventGetBlock, kEventGetBlockDesc);
    jmethodID eventGetB0 = Method(env, blockEvent, kEventGetB0, kIntDesc);
    jmethodID eventGetB1 = Method(env, blockEvent, kEventGetB1, kIntDesc);
    jmethodID eventGetPos = Method(env, blockEvent, kEventGetPos, kEventGetPosDesc);

    jfieldID gameRendererField = Field(env, minecraft, kGameRendererField, kGameRendererSig);
    jfieldID optionsField = Field(env, minecraft, kOptionsField, kOptionsSig);
    jfieldID levelField = Field(env, minecraft, kLevelField, kClientLevelSig);
    jfieldID chestTypeField = StaticField(env, chestBlock, kChestType, kChestTypeSig);
    jfieldID bobViewField = Field(env, options, kBobViewField, kOptionInstanceSig);
    // walkDist and walkDistO are declared on Entity and bob and oBob on Player;
    // JNI walks up from LocalPlayer to find all four.
    jfieldID playerField = Field(env, minecraft, kPlayerField, kLocalPlayerSig);
    jfieldID walkDistField = Field(env, localPlayer, kWalkDist, kFloatField);
    jfieldID walkDistOField = Field(env, localPlayer, kWalkDistO, kFloatField);
    jfieldID bobField = Field(env, localPlayer, kBob, kFloatField);
    jfieldID oBobField = Field(env, localPlayer, kOBob, kFloatField);
    jfieldID matrixFields[16] = {};
    {
        static const char* const kNames[16] = {
            "m00", "m01", "m02", "m03", "m10", "m11", "m12", "m13",
            "m20", "m21", "m22", "m23", "m30", "m31", "m32", "m33",
        };
        for (int i = 0; i < 16; ++i)
            matrixFields[i] = Field(env, matrix, kNames[i], kFloatField);
    }
    jfieldID vecX = Field(env, vec, kVecX, kMappedPrimitiveDoubleSig);
    jfieldID vecY = Field(env, vec, kVecY, kMappedPrimitiveDoubleSig);
    jfieldID vecZ = Field(env, vec, kVecZ, kMappedPrimitiveDoubleSig);
    jfieldID aabbMinX = Field(env, aabb, kAabbMinX, kMappedPrimitiveDoubleSig);
    jfieldID aabbMinY = Field(env, aabb, kAabbMinY, kMappedPrimitiveDoubleSig);
    jfieldID aabbMinZ = Field(env, aabb, kAabbMinZ, kMappedPrimitiveDoubleSig);
    jfieldID aabbMaxX = Field(env, aabb, kAabbMaxX, kMappedPrimitiveDoubleSig);
    jfieldID aabbMaxY = Field(env, aabb, kAabbMaxY, kMappedPrimitiveDoubleSig);
    jfieldID aabbMaxZ = Field(env, aabb, kAabbMaxZ, kMappedPrimitiveDoubleSig);

    jfieldID chestField = StaticField(env, blocks, kChestBlockField, kBlockSig);
    jfieldID trappedField = StaticField(env, blocks, kTrappedChestField, kBlockSig);
    Settle(env);
    jobject chestBlockObj = chestField ? env->GetStaticObjectField(blocks, chestField) : nullptr;
    jobject trappedChestObj =
        trappedField ? env->GetStaticObjectField(blocks, trappedField) : nullptr;
    jobject chestTypeProperty =
        chestTypeField ? env->GetStaticObjectField(chestBlock, chestTypeField) : nullptr;
    if (env->ExceptionCheck())
        env->ExceptionClear();

    // Every class a later NewObject or IsInstanceOf is handed has to gate the
    // layer: those two crash on a null class rather than failing the call.
    const bool ok = minecraft && gameRenderer && camera && level && chunk && blockEntity &&
                    chestEntity && blockState && shape && chestBlock && blocks && options &&
                    pos && vec && aabb && map && collection && iterator && enumClass &&
                    blockEvent && getInstance && getMainCamera && getFov && cameraPosition &&
                    cameraYRot && cameraXRot && renderDistance && getChunk && getBlockEntities &&
                    mapValues && collectionIterator && iteratorHasNext && iteratorNext &&
                    getBlockState && getBlockPos && getValue && hasProperty &&
                    connectedDirection && getShape && shapeBounds && shapeEmpty && levelGetState &&
                    posCtor && posRelative && posX && posY && posZ && ordinal && eventGetBlock &&
                    eventGetB0 && eventGetB1 && eventGetPos && gameRendererField && optionsField &&
                    levelField && chestTypeField && vecX && vecY && vecZ && aabbMinX && aabbMinY &&
                    aabbMinZ && aabbMaxX && aabbMaxY && aabbMaxZ && chestBlockObj &&
                    trappedChestObj && chestTypeProperty && getProjection && optionGet &&
                    bobViewField && matrixFields[0] && matrixFields[15] && getWindow &&
                    windowWidth && windowHeight && booleanValue && localPlayer && playerField &&
                    walkDistField && walkDistOField && bobField && oBobField;

    if (!ok) {
        g_chestResolveFailed = true;
        g_chestRetryAt =
            std::chrono::steady_clock::now() + std::chrono::seconds(kResolveRetrySeconds);
        if (!g_chestLogged) {
            g_chestLogged = true;
            LogTo("Base: chest layer incomplete -> mc=%d renderer=%d camera=%d level=%d chunk=%d "
                  "be=%d chestBE=%d state=%d shape=%d chestBlock=%d blocks=%d options=%d pos=%d "
                  "vec=%d aabb=%d map=%d coll=%d iter=%d enum=%d event=%d "
                  "camSrc=%d/%d/%d/%d rd=%d chunkSrc=%d/%d mapSrc=%d iterSrc=%d/%d "
                  "stateSrc=%d/%d value=%d/%d conn=%d shapeSrc=%d/%d/%d stateGet=%d "
                  "posSrc=%d/%d/%d/%d/%d ord=%d evSrc=%d/%d/%d/%d "
                  "fields=%d/%d/%d/%d vecF=%d/%d/%d aabbF=%d/%d/%d/%d/%d/%d blocks=%d/%d",
                  minecraft != nullptr, gameRenderer != nullptr, camera != nullptr,
                  level != nullptr, chunk != nullptr, blockEntity != nullptr,
                  chestEntity != nullptr, blockState != nullptr, shape != nullptr,
                  chestBlock != nullptr, blocks != nullptr, options != nullptr, pos != nullptr,
                  vec != nullptr, aabb != nullptr, map != nullptr, collection != nullptr,
                  iterator != nullptr, enumClass != nullptr, blockEvent != nullptr,
                  getInstance != nullptr, getMainCamera != nullptr, getFov != nullptr,
                  cameraPosition != nullptr, cameraYRot != nullptr, cameraXRot != nullptr,
                  renderDistance != nullptr, getChunk != nullptr, getBlockEntities != nullptr,
                  mapValues != nullptr, collectionIterator != nullptr, iteratorHasNext != nullptr,
                  iteratorNext != nullptr, getBlockState != nullptr, getBlockPos != nullptr,
                  getValue != nullptr, hasProperty != nullptr, connectedDirection != nullptr,
                  getShape != nullptr, shapeBounds != nullptr, shapeEmpty != nullptr,
                  levelGetState != nullptr, posCtor != nullptr, posRelative != nullptr,
                  posX != nullptr, posY != nullptr, posZ != nullptr, ordinal != nullptr,
                  eventGetBlock != nullptr, eventGetB0 != nullptr, eventGetB1 != nullptr,
                  eventGetPos != nullptr, gameRendererField != nullptr, optionsField != nullptr,
                  levelField != nullptr, chestTypeField != nullptr, vecX != nullptr,
                  vecY != nullptr, vecZ != nullptr, aabbMinX != nullptr, aabbMinY != nullptr,
                  aabbMinZ != nullptr, aabbMaxX != nullptr, aabbMaxY != nullptr,
                  aabbMaxZ != nullptr, chestBlockObj != nullptr, trappedChestObj != nullptr);
        }
    } else {
        g_chest.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));
        g_chest.chestEntity = static_cast<jclass>(env->NewGlobalRef(chestEntity));
        g_chest.chestBlock = static_cast<jclass>(env->NewGlobalRef(chestBlock));
        g_chest.blockEvent = static_cast<jclass>(env->NewGlobalRef(blockEvent));
        g_chest.pos = static_cast<jclass>(env->NewGlobalRef(pos));

        g_chest.getInstance = getInstance;
        g_chest.getMainCamera = getMainCamera;
        g_chest.getFov = getFov;
        g_chest.getProjection = getProjection;
        g_chest.optionGet = optionGet;
        g_chest.getWindow = getWindow;
        g_chest.windowWidth = windowWidth;
        g_chest.windowHeight = windowHeight;
        g_chest.booleanValue = booleanValue;
        g_chest.player = playerField;
        g_chest.walkDist = walkDistField;
        g_chest.walkDistO = walkDistOField;
        g_chest.bob = bobField;
        g_chest.oBob = oBobField;
        g_chest.cameraPosition = cameraPosition;
        g_chest.cameraYRot = cameraYRot;
        g_chest.cameraXRot = cameraXRot;
        g_chest.renderDistance = renderDistance;
        g_chest.getChunk = getChunk;
        g_chest.getBlockEntities = getBlockEntities;
        g_chest.mapValues = mapValues;
        g_chest.collectionIterator = collectionIterator;
        g_chest.iteratorHasNext = iteratorHasNext;
        g_chest.iteratorNext = iteratorNext;
        g_chest.getBlockState = getBlockState;
        g_chest.getBlockPos = getBlockPos;
        g_chest.getValue = getValue;
        g_chest.hasProperty = hasProperty;
        g_chest.connectedDirection = connectedDirection;
        g_chest.getShape = getShape;
        g_chest.shapeBounds = shapeBounds;
        g_chest.shapeEmpty = shapeEmpty;
        g_chest.levelGetState = levelGetState;
        g_chest.posCtor = posCtor;
        g_chest.posRelative = posRelative;
        g_chest.posX = posX;
        g_chest.posY = posY;
        g_chest.posZ = posZ;
        g_chest.ordinal = ordinal;
        g_chest.eventGetBlock = eventGetBlock;
        g_chest.eventGetB0 = eventGetB0;
        g_chest.eventGetB1 = eventGetB1;
        g_chest.eventGetPos = eventGetPos;

        g_chest.gameRenderer = gameRendererField;
        g_chest.options = optionsField;
        g_chest.level = levelField;
        g_chest.chestType = chestTypeField;
        g_chest.bobView = bobViewField;
        for (int i = 0; i < 16; ++i)
            g_chest.matrix[i] = matrixFields[i];
        g_chest.vecX = vecX;
        g_chest.vecY = vecY;
        g_chest.vecZ = vecZ;
        g_chest.aabbMinX = aabbMinX;
        g_chest.aabbMinY = aabbMinY;
        g_chest.aabbMinZ = aabbMinZ;
        g_chest.aabbMaxX = aabbMaxX;
        g_chest.aabbMaxY = aabbMaxY;
        g_chest.aabbMaxZ = aabbMaxZ;

        g_chest.chestBlockObj = env->NewGlobalRef(chestBlockObj);
        g_chest.trappedChestObj = env->NewGlobalRef(trappedChestObj);
        g_chest.chestTypeProperty = env->NewGlobalRef(chestTypeProperty);

        g_chest.ok = g_chest.minecraft && g_chest.chestEntity && g_chest.chestBlock &&
                     g_chest.blockEvent && g_chest.pos && g_chest.chestBlockObj &&
                     g_chest.trappedChestObj && g_chest.chestTypeProperty;
    }

    if (minecraft) env->DeleteLocalRef(minecraft);
    if (gameRenderer) env->DeleteLocalRef(gameRenderer);
    if (camera) env->DeleteLocalRef(camera);
    if (level) env->DeleteLocalRef(level);
    if (chunk) env->DeleteLocalRef(chunk);
    if (blockEntity) env->DeleteLocalRef(blockEntity);
    if (chestEntity) env->DeleteLocalRef(chestEntity);
    if (blockState) env->DeleteLocalRef(blockState);
    if (shape) env->DeleteLocalRef(shape);
    if (chestBlock) env->DeleteLocalRef(chestBlock);
    if (blocks) env->DeleteLocalRef(blocks);
    if (options) env->DeleteLocalRef(options);
    if (pos) env->DeleteLocalRef(pos);
    if (vec) env->DeleteLocalRef(vec);
    if (aabb) env->DeleteLocalRef(aabb);
    if (map) env->DeleteLocalRef(map);
    if (collection) env->DeleteLocalRef(collection);
    if (iterator) env->DeleteLocalRef(iterator);
    if (enumClass) env->DeleteLocalRef(enumClass);
    if (blockEvent) env->DeleteLocalRef(blockEvent);
    if (chestBlockObj) env->DeleteLocalRef(chestBlockObj);
    if (trappedChestObj) env->DeleteLocalRef(trappedChestObj);
    if (chestTypeProperty) env->DeleteLocalRef(chestTypeProperty);
    if (localPlayer) env->DeleteLocalRef(localPlayer);
    return g_chest.ok;
}

bool ChestReady(JNIEnv* env) { return ResolveChest(env); }

jobject ChestClient(JNIEnv* env) {
    jobject instance = env->CallStaticObjectMethod(g_chest.minecraft, g_chest.getInstance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return instance;
}

// A BlockPos, as a local reference the caller releases.
jobject MakeBlockPos(JNIEnv* env, int x, int y, int z) {
    jobject made = env->NewObject(g_chest.pos, g_chest.posCtor, static_cast<jint>(x),
                                  static_cast<jint>(y), static_cast<jint>(z));
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return made;
}

bool ReadBlockPos(JNIEnv* env, jobject pos, int& x, int& y, int& z) {
    if (pos == nullptr)
        return false;
    x = static_cast<int>(env->CallIntMethod(pos, g_chest.posX));
    y = static_cast<int>(env->CallIntMethod(pos, g_chest.posY));
    z = static_cast<int>(env->CallIntMethod(pos, g_chest.posZ));
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

// BlockUtils.getBoundingBox: getShape(level, pos).bounds().move(pos). The move is
// done here as six additions rather than another crossing.
bool OutlineBox(JNIEnv* env, jobject level, jobject state, int x, int y, int z, ChestBox& out) {
    jobject blockPos = MakeBlockPos(env, x, y, z);
    if (blockPos == nullptr)
        return false;

    jobject shape = env->CallObjectMethod(state, g_chest.getShape, level, blockPos);
    env->DeleteLocalRef(blockPos);
    if (env->ExceptionCheck() || shape == nullptr) {
        env->ExceptionClear();
        if (shape) env->DeleteLocalRef(shape);
        return false;
    }

    jobject bounds = env->CallObjectMethod(shape, g_chest.shapeBounds);
    env->DeleteLocalRef(shape);
    if (env->ExceptionCheck() || bounds == nullptr) {
        env->ExceptionClear();
        if (bounds) env->DeleteLocalRef(bounds);
        return false;
    }

    out.minX = static_cast<double>(env->GetDoubleField(bounds, g_chest.aabbMinX)) + x;
    out.minY = static_cast<double>(env->GetDoubleField(bounds, g_chest.aabbMinY)) + y;
    out.minZ = static_cast<double>(env->GetDoubleField(bounds, g_chest.aabbMinZ)) + z;
    out.maxX = static_cast<double>(env->GetDoubleField(bounds, g_chest.aabbMaxX)) + x;
    out.maxY = static_cast<double>(env->GetDoubleField(bounds, g_chest.aabbMaxY)) + y;
    out.maxZ = static_cast<double>(env->GetDoubleField(bounds, g_chest.aabbMaxZ)) + z;
    env->DeleteLocalRef(bounds);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

// A block state, as a local reference the caller releases.
jobject StateAt(JNIEnv* env, jobject level, int x, int y, int z) {
    jobject blockPos = MakeBlockPos(env, x, y, z);
    if (blockPos == nullptr)
        return nullptr;
    jobject state = env->CallObjectMethod(level, g_chest.levelGetState, blockPos);
    env->DeleteLocalRef(blockPos);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return state;
}

// BlockUtils.canBeClicked: the block has an outline shape at all, which is what
// makes it something the second half of a double chest can be joined to.
bool CanBeClicked(JNIEnv* env, jobject level, jobject state, int x, int y, int z) {
    if (state == nullptr)
        return false;
    jobject blockPos = MakeBlockPos(env, x, y, z);
    if (blockPos == nullptr)
        return false;

    jobject shape = env->CallObjectMethod(state, g_chest.getShape, level, blockPos);
    env->DeleteLocalRef(blockPos);
    if (env->ExceptionCheck() || shape == nullptr) {
        env->ExceptionClear();
        if (shape) env->DeleteLocalRef(shape);
        return false;
    }

    const jboolean empty = env->CallBooleanMethod(shape, g_chest.shapeEmpty);
    env->DeleteLocalRef(shape);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return empty != JNI_TRUE;
}

// One block entity, when it is a chest: the box it occupies, with the other half
// merged in when it is a double. The left half reports nothing, because the right
// half already covers both.
bool ChestBoxAt(JNIEnv* env, jobject level, jobject blockEntity, ChestBox& out) {
    jobject state = env->CallObjectMethod(blockEntity, g_chest.getBlockState);
    if (env->ExceptionCheck() || state == nullptr) {
        env->ExceptionClear();
        if (state) env->DeleteLocalRef(state);
        return false;
    }

    const jboolean hasType =
        env->CallBooleanMethod(state, g_chest.hasProperty, g_chest.chestTypeProperty);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(state);
        return false;
    }
    if (hasType != JNI_TRUE) {
        env->DeleteLocalRef(state);
        return false;
    }

    jobject chestType = env->CallObjectMethod(state, g_chest.getValue, g_chest.chestTypeProperty);
    if (env->ExceptionCheck() || chestType == nullptr) {
        env->ExceptionClear();
        if (chestType) env->DeleteLocalRef(chestType);
        env->DeleteLocalRef(state);
        return false;
    }
    const jint type = env->CallIntMethod(chestType, g_chest.ordinal);
    env->DeleteLocalRef(chestType);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(state);
        return false;
    }
    if (type == kChestTypeLeft) {
        env->DeleteLocalRef(state);
        return false;
    }

    jobject pos = env->CallObjectMethod(blockEntity, g_chest.getBlockPos);
    if (env->ExceptionCheck() || pos == nullptr) {
        env->ExceptionClear();
        if (pos) env->DeleteLocalRef(pos);
        env->DeleteLocalRef(state);
        return false;
    }
    int x = 0, y = 0, z = 0;
    const bool havePos = ReadBlockPos(env, pos, x, y, z);
    env->DeleteLocalRef(pos);
    if (!havePos) {
        env->DeleteLocalRef(state);
        return false;
    }

    const bool haveBox = OutlineBox(env, level, state, x, y, z, out);

    if (haveBox && type != kChestTypeSingle) {
        jobject direction =
            env->CallStaticObjectMethod(g_chest.chestBlock, g_chest.connectedDirection, state);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            direction = nullptr;
        }
        if (direction != nullptr) {
            int nx = x;
            int ny = y;
            int nz = z;
            jobject here = MakeBlockPos(env, x, y, z);
            jobject neighbour = here ? env->CallObjectMethod(here, g_chest.posRelative, direction)
                                     : nullptr;
            if (here) env->DeleteLocalRef(here);
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                neighbour = nullptr;
            }
            if (neighbour != nullptr) {
                if (ReadBlockPos(env, neighbour, nx, ny, nz)) {
                    // The neighbour has its own state, and the original reads its
                    // own outline rather than assuming the two halves match.
                    jobject neighbourState = StateAt(env, level, nx, ny, nz);
                    if (CanBeClicked(env, level, neighbourState, nx, ny, nz)) {
                        ChestBox second;
                        if (OutlineBox(env, level, neighbourState, nx, ny, nz, second)) {
                            out.minX = std::min(out.minX, second.minX);
                            out.minY = std::min(out.minY, second.minY);
                            out.minZ = std::min(out.minZ, second.minZ);
                            out.maxX = std::max(out.maxX, second.maxX);
                            out.maxY = std::max(out.maxY, second.maxY);
                            out.maxZ = std::max(out.maxZ, second.maxZ);
                        }
                    }
                    if (neighbourState) env->DeleteLocalRef(neighbourState);
                }
                env->DeleteLocalRef(neighbour);
            }
            env->DeleteLocalRef(direction);
        }
    }

    env->DeleteLocalRef(state);
    if (!haveBox)
        return false;

    out.x = x;
    out.y = y;
    out.z = z;
    return true;
}

}  // namespace

bool LoadedChestBoxes(std::vector<ChestBox>& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::LoadedChestBoxes(out);
    if (compat::IsLegacyLunar()) return compat::lunar::LoadedChestBoxes(out);
    out.clear();
    JniAttach attach;
    if (!attach || !ChestReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject client = ChestClient(env);
    if (client == nullptr)
        return false;

    jobject level = env->GetObjectField(client, g_chest.level);
    jobject options = env->GetObjectField(client, g_chest.options);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck())
        env->ExceptionClear();
    if (level == nullptr) {
        if (options) env->DeleteLocalRef(options);
        return false;
    }

    int renderDistance = 8;
    if (options != nullptr) {
        const jint distance = env->CallIntMethod(options, g_chest.renderDistance);
        if (!env->ExceptionCheck())
            renderDistance = static_cast<int>(distance);
        else
            env->ExceptionClear();
        env->DeleteLocalRef(options);
    }

    // The chunk square the original walks, centred on the player: the render
    // distance with a floor of two, widened by three.
    const int radius = std::max(2, renderDistance) + 3;
    const int diameter = radius * 2 + 1;

    EntityHandle self = PlayerEntity();
    rotation::Vec3 where;
    if (!self.Valid() || !self.Position(where)) {
        env->DeleteLocalRef(level);
        return false;
    }
    const int centerX = static_cast<int>(std::floor(where.x)) >> 4;
    const int centerZ = static_cast<int>(std::floor(where.z)) >> 4;

    for (int dx = 0; dx < diameter; ++dx) {
        for (int dz = 0; dz < diameter; ++dz) {
            const int chunkX = centerX - radius + dx;
            const int chunkZ = centerZ - radius + dz;

            jobject chunk = env->CallObjectMethod(level, g_chest.getChunk, static_cast<jint>(chunkX),
                                                  static_cast<jint>(chunkZ));
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                continue;
            }
            if (chunk == nullptr)
                continue;

            jobject map = env->CallObjectMethod(chunk, g_chest.getBlockEntities);
            env->DeleteLocalRef(chunk);
            if (env->ExceptionCheck() || map == nullptr) {
                env->ExceptionClear();
                if (map) env->DeleteLocalRef(map);
                continue;
            }

            jobject values = env->CallObjectMethod(map, g_chest.mapValues);
            env->DeleteLocalRef(map);
            if (env->ExceptionCheck() || values == nullptr) {
                env->ExceptionClear();
                if (values) env->DeleteLocalRef(values);
                continue;
            }

            jobject iterator = env->CallObjectMethod(values, g_chest.collectionIterator);
            env->DeleteLocalRef(values);
            if (env->ExceptionCheck() || iterator == nullptr) {
                env->ExceptionClear();
                if (iterator) env->DeleteLocalRef(iterator);
                continue;
            }

            for (;;) {
                const jboolean more = env->CallBooleanMethod(iterator, g_chest.iteratorHasNext);
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                    break;
                }
                if (more != JNI_TRUE)
                    break;

                jobject entity = env->CallObjectMethod(iterator, g_chest.iteratorNext);
                if (env->ExceptionCheck() || entity == nullptr) {
                    env->ExceptionClear();
                    if (entity) env->DeleteLocalRef(entity);
                    continue;
                }

                if (env->IsInstanceOf(entity, g_chest.chestEntity) == JNI_TRUE) {
                    ChestBox box;
                    if (ChestBoxAt(env, level, entity, box))
                        out.push_back(box);
                }
                if (env->ExceptionCheck())
                    env->ExceptionClear();
                env->DeleteLocalRef(entity);
            }

            env->DeleteLocalRef(iterator);
        }
    }

    env->DeleteLocalRef(level);
    return true;
}

void JNICALL ForgeWorldCameraHook(JNIEnv* env, jclass, jobject camera,
                                   jobject poseStack, jobject projection) {
    InvalidateWorldCamera();
    if (IsCircuitOpen() || !camera || !poseStack || !projection ||
        env->ExceptionCheck() || env->PushLocalFrame(32) != JNI_OK) return;
    if (ChestReady(env)) {
        // These are the exact arguments supplied to LevelRenderer.renderLevel.
        // The projection already contains hurt/bob/portal effects; the pose
        // also contains Forge's camera roll. Do not apply either a second time.
        auto stackClass = env->GetObjectClass(poseStack);
        auto last = Method(env, stackClass, "m_85850_", "()Lcom/mojang/blaze3d/vertex/PoseStack$Pose;");
        auto pose = last ? env->CallObjectMethod(poseStack, last) : nullptr;
        auto poseClass = pose && !env->ExceptionCheck() ? env->GetObjectClass(pose) : nullptr;
        auto poseMatrix = Method(env, poseClass, "m_252922_", "()Lorg/joml/Matrix4f;");
        auto modelView = poseMatrix ? env->CallObjectMethod(pose, poseMatrix) : nullptr;
        auto position = env->CallObjectMethod(camera, g_chest.cameraPosition);
        auto client = ChestClient(env);
        auto window = client ? env->CallObjectMethod(client, g_chest.getWindow) : nullptr;
        if (modelView && position && window && !env->ExceptionCheck()) {
            CameraView view;
            view.x = env->GetDoubleField(position, g_chest.vecX);
            view.y = env->GetDoubleField(position, g_chest.vecY);
            view.z = env->GetDoubleField(position, g_chest.vecZ);
            view.yaw = env->CallFloatMethod(camera, g_chest.cameraYRot);
            view.pitch = env->CallFloatMethod(camera, g_chest.cameraXRot);
            for (int i = 0; i < 16; ++i) {
                view.projection[i] = env->GetFloatField(projection, g_chest.matrix[i]);
                view.modelView[i] = env->GetFloatField(modelView, g_chest.matrix[i]);
            }
            view.hasModelView = true;
            view.windowWidth = env->CallIntMethod(window, g_chest.windowWidth);
            view.windowHeight = env->CallIntMethod(window, g_chest.windowHeight);
            if (!env->ExceptionCheck()) PublishWorldCamera(view);
        }
    }
    Settle(env);
    env->PopLocalFrame(nullptr);
}

bool ReadCameraView(float partialTick, CameraView& out) {
    // 26.3 exposes the equivalent snapshot in its native world render state.
    if (compat::UsesModernRuntime() && !compat::UsesGlfwModernRuntime())
        return compat::mc263::ReadCameraView(partialTick, out);
    return ReadWorldCamera(out);
}
bool ReadChestOpenEvent(jobject packet, int& x, int& y, int& z) {
    if (compat::UsesModernRuntime()) return compat::mc263::ReadChestOpenEvent(packet, x, y, z);
    if (compat::IsLegacyLunar()) return compat::lunar::ReadChestOpenEvent(packet, x, y, z);
    x = 0;
    y = 0;
    z = 0;
    if (packet == nullptr)
        return false;

    JniAttach attach;
    if (!attach || !ChestReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    if (env->IsInstanceOf(packet, g_chest.blockEvent) != JNI_TRUE)
        return false;

    jobject block = env->CallObjectMethod(packet, g_chest.eventGetBlock);
    if (env->ExceptionCheck() || block == nullptr) {
        env->ExceptionClear();
        if (block) env->DeleteLocalRef(block);
        return false;
    }
    const bool isChest = env->IsSameObject(block, g_chest.chestBlockObj) ||
                         env->IsSameObject(block, g_chest.trappedChestObj);
    env->DeleteLocalRef(block);
    if (!isChest)
        return false;

    const jint b0 = env->CallIntMethod(packet, g_chest.eventGetB0);
    const jint b1 = env->CallIntMethod(packet, g_chest.eventGetB1);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (b0 != 1 || b1 != 1)
        return false;

    jobject pos = env->CallObjectMethod(packet, g_chest.eventGetPos);
    if (env->ExceptionCheck() || pos == nullptr) {
        env->ExceptionClear();
        if (pos) env->DeleteLocalRef(pos);
        return false;
    }
    const bool have = ReadBlockPos(env, pos, x, y, z);
    env->DeleteLocalRef(pos);
    return have;
}

}
