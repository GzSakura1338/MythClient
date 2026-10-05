#include "minecraft/java/api/game_compass.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"

#include <chrono>

#include "minecraft/java/api/jni_lookup.h"

// The compass's slice of the world.
//
// A fourth self-contained handle set, for the reason the earlier ones are: the
// compass wants four things no other layer resolves -- the entity's previous-tick
// position, the dimension type, the shared spawn, and the window's GUI scale --
// and a missing member here must not be able to take placement or the inventory
// down with it.
//
// The two interpolated reads are per frame, so the cost of this layer is one class
// loader walk once and then a handful of field reads per frame after that.

namespace nova::java {
namespace {

#define NOVA_FORGE1201_COMPASS
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_COMPASS

// DimensionType is a record, so `natural` exists as both a private field and a
// public accessor with the same SRG name. The accessor is the one wanted.

struct CompassHandles {
    // Only the Minecraft class is held: it is what the static getInstance is
    // called through. Everything else here is a field or method id, which stays
    // valid without keeping its class alive.
    jclass    minecraft = nullptr;

    jmethodID getInstance = nullptr;
    jmethodID getWindow = nullptr;
    jmethodID windowWidth = nullptr;
    jmethodID windowHeight = nullptr;
    jmethodID windowGuiScale = nullptr;
    jmethodID getX = nullptr;
    jmethodID getZ = nullptr;
    jmethodID getYRot = nullptr;
    jmethodID getSpawnPos = nullptr;
    jmethodID dimensionType = nullptr;
    jmethodID natural = nullptr;
    jmethodID vec3iGetX = nullptr;
    jmethodID vec3iGetZ = nullptr;

    jfieldID  level = nullptr;
    jfieldID  player = nullptr;
    jfieldID  xOld = nullptr;
    jfieldID  zOld = nullptr;
    jfieldID  yRotO = nullptr;

    bool ok = false;
};

CompassHandles g_compass;
bool g_compassLogged = false;
bool g_compassResolveFailed = false;
std::chrono::steady_clock::time_point g_compassRetryAt;

constexpr int kResolveRetrySeconds = 10;

bool ResolveCompass(JNIEnv* env) {
    if (!env)
        return false;
    if (g_compass.ok)
        return true;
    if (g_compassResolveFailed && std::chrono::steady_clock::now() < g_compassRetryAt)
        return false;

    jobject loader = GetMinecraftClassLoader(env);
    jclass minecraft = nova_base::loadOrFind(env, loader, kMinecraftDot, kMinecraftSig);
    jclass window = nova_base::loadOrFind(env, loader, kWindowDot, kWindowSig);
    jclass level = nova_base::loadOrFind(env, loader, kLevelDot, kLevelSig);
    jclass entity = nova_base::loadOrFind(env, loader, kEntityDot, kEntitySig);
    jclass dimension =
        nova_base::loadOrFind(env, loader, kDimensionDot, kDimensionSig);
    jclass pos = nova_base::loadOrFind(env, loader, kMappedBlockPosDot, kPosSig);
    if (loader)
        env->DeleteGlobalRef(loader);

    jmethodID getInstance =
        minecraft ? env->GetStaticMethodID(minecraft, kGetInstance, kGetInstanceDesc) : nullptr;
    jmethodID getWindow = minecraft ? env->GetMethodID(minecraft, kGetWindow, kGetWindowDesc) : nullptr;
    jmethodID windowWidth = window ? env->GetMethodID(window, kWindowWidth, kIntDesc) : nullptr;
    jmethodID windowHeight = window ? env->GetMethodID(window, kWindowHeight, kIntDesc) : nullptr;
    jmethodID windowGuiScale =
        window ? env->GetMethodID(window, kWindowGuiScale, kDoubleDesc) : nullptr;
    jmethodID getX = entity ? env->GetMethodID(entity, kGetX, kDoubleDesc) : nullptr;
    jmethodID getZ = entity ? env->GetMethodID(entity, kGetZ, kDoubleDesc) : nullptr;
    jmethodID getYRot = entity ? env->GetMethodID(entity, kGetYRot, kFloatDesc) : nullptr;
    jmethodID getSpawnPos =
        level ? env->GetMethodID(level, kGetSpawnPos, kGetSpawnPosDesc) : nullptr;
    jmethodID dimensionType =
        level ? env->GetMethodID(level, kDimensionType, kDimensionTypeDesc) : nullptr;
    jmethodID natural = dimension ? env->GetMethodID(dimension, kNatural, kBoolDesc) : nullptr;
    jmethodID vec3iGetX = pos ? env->GetMethodID(pos, kVec3iGetX, kIntDesc) : nullptr;
    jmethodID vec3iGetZ = pos ? env->GetMethodID(pos, kVec3iGetZ, kIntDesc) : nullptr;

    jfieldID levelField = minecraft ? env->GetFieldID(minecraft, kLevelField, kClientLevelSig) : nullptr;
    jfieldID playerField = minecraft ? env->GetFieldID(minecraft, kPlayerField, kLocalPlayerSig) : nullptr;
    jfieldID xOld = entity ? env->GetFieldID(entity, kXOld, kDoubleField) : nullptr;
    jfieldID zOld = entity ? env->GetFieldID(entity, kZOld, kDoubleField) : nullptr;
    jfieldID yRotO = entity ? env->GetFieldID(entity, kYRotO, kFloatField) : nullptr;
    if (env->ExceptionCheck())
        env->ExceptionClear();

    const bool ok = minecraft && window && level && entity && dimension && pos && getInstance &&
                    getWindow && windowWidth && windowHeight && windowGuiScale && getX && getZ &&
                    getYRot && getSpawnPos && dimensionType && natural && vec3iGetX && vec3iGetZ &&
                    levelField && playerField && xOld && zOld && yRotO;

    if (!ok) {
        g_compassResolveFailed = true;
        g_compassRetryAt =
            std::chrono::steady_clock::now() + std::chrono::seconds(kResolveRetrySeconds);
        if (!g_compassLogged) {
            g_compassLogged = true;
            LogTo("Base: compass layer incomplete -> mc=%d window=%d level=%d entity=%d dim=%d "
                  "pos=%d getInstance=%d getWindow=%d size=%d/%d scale=%d xyz=%d/%d/%d "
                  "spawn=%d dimType=%d natural=%d vec3i=%d/%d fields=%d/%d/%d/%d/%d",
                  minecraft != nullptr, window != nullptr, level != nullptr, entity != nullptr,
                  dimension != nullptr, pos != nullptr, getInstance != nullptr,
                  getWindow != nullptr, windowWidth != nullptr, windowHeight != nullptr,
                  windowGuiScale != nullptr, getX != nullptr, getZ != nullptr, getYRot != nullptr,
                  getSpawnPos != nullptr, dimensionType != nullptr, natural != nullptr,
                  vec3iGetX != nullptr, vec3iGetZ != nullptr, levelField != nullptr,
                  playerField != nullptr, xOld != nullptr, zOld != nullptr, yRotO != nullptr);
        }
    } else {
        g_compass.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));

        g_compass.getInstance = getInstance;
        g_compass.getWindow = getWindow;
        g_compass.windowWidth = windowWidth;
        g_compass.windowHeight = windowHeight;
        g_compass.windowGuiScale = windowGuiScale;
        g_compass.getX = getX;
        g_compass.getZ = getZ;
        g_compass.getYRot = getYRot;
        g_compass.getSpawnPos = getSpawnPos;
        g_compass.dimensionType = dimensionType;
        g_compass.natural = natural;
        g_compass.vec3iGetX = vec3iGetX;
        g_compass.vec3iGetZ = vec3iGetZ;

        g_compass.level = levelField;
        g_compass.player = playerField;
        g_compass.xOld = xOld;
        g_compass.zOld = zOld;
        g_compass.yRotO = yRotO;

        g_compass.ok = g_compass.minecraft != nullptr;
    }

    if (minecraft) env->DeleteLocalRef(minecraft);
    if (window) env->DeleteLocalRef(window);
    if (level) env->DeleteLocalRef(level);
    if (entity) env->DeleteLocalRef(entity);
    if (dimension) env->DeleteLocalRef(dimension);
    if (pos) env->DeleteLocalRef(pos);
    return g_compass.ok;
}

bool CompassReady(JNIEnv* env) { return ResolveCompass(env); }

jobject CompassClient(JNIEnv* env) {
    jobject instance = env->CallStaticObjectMethod(g_compass.minecraft, g_compass.getInstance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return instance;
}

}  // namespace

bool ReadCompassView(float partialTick, CompassView& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::ReadCompassView(partialTick, out);
    if (compat::IsLegacyLunar()) return compat::lunar::ReadCompassView(partialTick, out);
    out = CompassView{};
    JniAttach attach;
    if (!attach || !CompassReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject client = CompassClient(env);
    if (client == nullptr)
        return false;

    jobject player = env->GetObjectField(client, g_compass.player);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (player == nullptr)
        return false;

    const jdouble xOld = env->GetDoubleField(player, g_compass.xOld);
    const jdouble zOld = env->GetDoubleField(player, g_compass.zOld);
    const jfloat yRotO = env->GetFloatField(player, g_compass.yRotO);
    const jdouble x = env->CallDoubleMethod(player, g_compass.getX);
    const jdouble z = env->CallDoubleMethod(player, g_compass.getZ);
    const jfloat yaw = env->CallFloatMethod(player, g_compass.getYRot);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }

    const double delta = static_cast<double>(partialTick);
    out.x = xOld + delta * (static_cast<double>(x) - xOld);
    out.z = zOld + delta * (static_cast<double>(z) - zOld);
    out.yaw = static_cast<float>(static_cast<double>(yRotO) +
                                delta * (static_cast<double>(yaw) - static_cast<double>(yRotO)));
    return true;
}

bool WorldSpawn(int& x, int& z) {
    if (compat::UsesModernRuntime()) return compat::mc263::WorldSpawn(x, z);
    if (compat::IsLegacyLunar()) return compat::lunar::WorldSpawn(x, z);
    x = 0;
    z = 0;
    JniAttach attach;
    if (!attach || !CompassReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject client = CompassClient(env);
    if (client == nullptr)
        return false;

    jobject level = env->GetObjectField(client, g_compass.level);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (level == nullptr)
        return false;

    jobject dimension = env->CallObjectMethod(level, g_compass.dimensionType);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        dimension = nullptr;
    }
    if (dimension == nullptr) {
        env->DeleteLocalRef(level);
        return false;
    }

    const jboolean natural = env->CallBooleanMethod(dimension, g_compass.natural);
    env->DeleteLocalRef(dimension);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(level);
        return false;
    }
    if (natural != JNI_TRUE) {
        env->DeleteLocalRef(level);
        return false;
    }

    jobject pos = env->CallObjectMethod(level, g_compass.getSpawnPos);
    env->DeleteLocalRef(level);
    if (env->ExceptionCheck() || pos == nullptr) {
        env->ExceptionClear();
        if (pos) env->DeleteLocalRef(pos);
        return false;
    }

    const jint bx = env->CallIntMethod(pos, g_compass.vec3iGetX);
    const jint bz = env->CallIntMethod(pos, g_compass.vec3iGetZ);
    env->DeleteLocalRef(pos);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }

    x = static_cast<int>(bx);
    z = static_cast<int>(bz);
    return true;
}

bool WindowMetrics(int& width, int& height, double& guiScale) {
    if (compat::UsesModernRuntime()) return compat::mc263::WindowMetrics(width, height, guiScale);
    if (compat::IsLegacyLunar()) return compat::lunar::WindowMetrics(width, height, guiScale);
    width = 0;
    height = 0;
    guiScale = 0.0;
    JniAttach attach;
    if (!attach || !CompassReady(attach.env))
        return false;
    JNIEnv* env = attach.env;

    jobject client = CompassClient(env);
    if (client == nullptr)
        return false;

    jobject window = env->CallObjectMethod(client, g_compass.getWindow);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck() || window == nullptr) {
        env->ExceptionClear();
        if (window) env->DeleteLocalRef(window);
        return false;
    }

    const jint w = env->CallIntMethod(window, g_compass.windowWidth);
    const jint h = env->CallIntMethod(window, g_compass.windowHeight);
    const jdouble scale = env->CallDoubleMethod(window, g_compass.windowGuiScale);
    env->DeleteLocalRef(window);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }

    width = static_cast<int>(w);
    height = static_cast<int>(h);
    guiScale = static_cast<double>(scale);
    return true;
}

}
