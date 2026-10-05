#include "minecraft/java/api/game_rotation.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"

#include <chrono>

#include "minecraft/java/api/base.h"
#include "minecraft/java/api/jni_lookup.h"

namespace nova::java {
namespace {

#define NOVA_FORGE1201_ROTATION
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_ROTATION

struct Handles {
    jclass    minecraft = nullptr;
    jmethodID getInstance = nullptr;
    jfieldID  player = nullptr;
    jfieldID  options = nullptr;
    jmethodID getFps = nullptr;

    jmethodID getYRot = nullptr;
    jmethodID getXRot = nullptr;
    jmethodID setYRot = nullptr;
    jmethodID setXRot = nullptr;
    jmethodID getEye = nullptr;
    jfieldID  yRotO = nullptr;
    jfieldID  xRotO = nullptr;
    jfieldID  vecX = nullptr;
    jfieldID  vecY = nullptr;
    jfieldID  vecZ = nullptr;

    jmethodID sensitivity = nullptr;
    jfieldID  optionValue = nullptr;
    jmethodID doubleValue = nullptr;

    jfieldID  gameRenderer = nullptr;
    jmethodID pick = nullptr;

    jmethodID renderDistance = nullptr;
    jmethodID fov = nullptr;
    jmethodID intValue = nullptr;

    jclass    posRot = nullptr;
    jmethodID posRotCtor = nullptr;
    jmethodID posRotGetX = nullptr;
    jmethodID posRotGetY = nullptr;
    jmethodID posRotGetZ = nullptr;
    jmethodID posRotGetYaw = nullptr;
    jmethodID posRotGetPitch = nullptr;
    jmethodID posRotGround = nullptr;
};

Handles g_rotation;
bool g_rotationLogged = false;
bool g_rotationResolveFailed = false;
std::chrono::steady_clock::time_point g_rotationResolveRetryAt;

constexpr int kRotationResolveRetrySeconds = 10;

bool Resolve(JNIEnv* env) {
    jobject loader = GetMinecraftClassLoader(env);
    jclass minecraft = nova_base::loadOrFind(env, loader, kMinecraftDot, kMinecraftSig);
    jclass entity = nova_base::loadOrFind(env, loader, kEntityDot, kEntitySig);
    jclass options = nova_base::loadOrFind(env, loader, kOptionsDot, kOptionsSig);
    jclass option = nova_base::loadOrFind(env, loader, kOptionDot, kOptionSig);
    jclass vec3 = nova_base::loadOrFind(env, loader, kMappedVec3Dot, kVec3Sig);
    jclass boxed = nova_base::loadOrFind(env, loader, kDoubleDot, kMappedDoubleSig);
    jclass renderer = nova_base::loadOrFind(env, loader, kMappedGameRendererDot,
                                            kGameRendererSig);
    jclass boxedInt = nova_base::loadOrFind(env, loader, kIntDot, kMappedIntegerSig);
    jclass posRot = nova_base::loadOrFind(env, loader, kPosRotDot, kPosRotSig);
    if (loader)
        env->DeleteGlobalRef(loader);

    if (!minecraft || !entity || !options || !option || !vec3 || !boxed || !renderer ||
        !boxedInt || !posRot) {
        if (minecraft) env->DeleteLocalRef(minecraft);
        if (entity) env->DeleteLocalRef(entity);
        if (options) env->DeleteLocalRef(options);
        if (option) env->DeleteLocalRef(option);
        if (vec3) env->DeleteLocalRef(vec3);
        if (boxed) env->DeleteLocalRef(boxed);
        if (renderer) env->DeleteLocalRef(renderer);
        if (boxedInt) env->DeleteLocalRef(boxedInt);
        if (posRot) env->DeleteLocalRef(posRot);
        return false;
    }

    jmethodID getInstance = env->GetStaticMethodID(minecraft, kGetInstance, kGetInstanceDesc);
    jfieldID player = env->GetFieldID(minecraft, kPlayerField, kPlayerSig);
    jfieldID optionsField = env->GetFieldID(minecraft, kOptionsField, kOptionsSig);
    jmethodID getFps = env->GetMethodID(minecraft, kGetFps, kGetFpsDesc);

    jmethodID getYRot = env->GetMethodID(entity, kGetYRot, kGetYRotDesc);
    jmethodID getXRot = env->GetMethodID(entity, kGetXRot, kGetXRotDesc);
    jmethodID setYRot = env->GetMethodID(entity, kSetYRot, kSetYRotDesc);
    jmethodID setXRot = env->GetMethodID(entity, kSetXRot, kSetXRotDesc);
    jmethodID getEye = env->GetMethodID(entity, kGetEye, kGetEyeDesc);
    jfieldID yRotO = env->GetFieldID(entity, kYRotO, kFloatDesc);
    jfieldID xRotO = env->GetFieldID(entity, kXRotO, kFloatDesc);
    jfieldID vecX = env->GetFieldID(vec3, kVecX, kMappedPrimitiveDoubleSig);
    jfieldID vecY = env->GetFieldID(vec3, kVecY, kMappedPrimitiveDoubleSig);
    jfieldID vecZ = env->GetFieldID(vec3, kVecZ, kMappedPrimitiveDoubleSig);

    jmethodID sensitivity = env->GetMethodID(options, kSensitivity, kSensitivityDesc);
    jfieldID optionValue = env->GetFieldID(option, kValueField, kValueDesc);
    jmethodID doubleValue = env->GetMethodID(boxed, kDoubleValue, kDoubleValueDesc);

    jfieldID gameRenderer = env->GetFieldID(minecraft, kGameRenderer, kGameRendererSig);
    jmethodID pick = env->GetMethodID(renderer, kPick, kPickDesc);

    jmethodID renderDistance = env->GetMethodID(options, kRenderDistance, kSensitivityDesc);
    jmethodID fov = env->GetMethodID(options, kFov, kSensitivityDesc);
    jmethodID intValue = env->GetMethodID(boxedInt, kIntValue, kIntValueDesc);

    jmethodID posRotCtor = env->GetMethodID(posRot, kPosRotCtor, kPosRotCtorDesc);
    jmethodID posRotGetX = env->GetMethodID(posRot, kPosRotGetX, kPosRotGetD);
    jmethodID posRotGetY = env->GetMethodID(posRot, kPosRotGetY, kPosRotGetD);
    jmethodID posRotGetZ = env->GetMethodID(posRot, kPosRotGetZ, kPosRotGetD);
    jmethodID posRotGetYaw = env->GetMethodID(posRot, kPosRotGetYaw, kPosRotGetF);
    jmethodID posRotGetPitch = env->GetMethodID(posRot, kPosRotGetPitch, kPosRotGetF);
    jmethodID posRotGround = env->GetMethodID(posRot, kPosRotGround, kMappedNoArgsToBooleanDesc);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    const bool ok = getInstance && player && optionsField && getFps && getYRot && getXRot &&
                    setYRot && setXRot && getEye && yRotO && xRotO && vecX && vecY && vecZ &&
                    sensitivity && optionValue && doubleValue && gameRenderer && pick &&
                    renderDistance && fov && intValue && posRotCtor && posRotGetX &&
                    posRotGetY && posRotGetZ && posRotGetYaw && posRotGetPitch && posRotGround;
    if (!ok) {
        if (!g_rotationLogged) {
            g_rotationLogged = true;
            LogTo("Base: rotation unavailable -> getInstance=%d player=%d options=%d fps=%d "
                  "getYRot=%d getXRot=%d setYRot=%d setXRot=%d getEye=%d yRotO=%d xRotO=%d "
                  "vecX=%d vecY=%d vecZ=%d sensitivity=%d optionValue=%d doubleValue=%d "
                  "gameRenderer=%d pick=%d renderDistance=%d fov=%d intValue=%d posRotCtor=%d "
                  "posRotGetX=%d posRotGetY=%d posRotGetZ=%d posRotGetYaw=%d posRotGetPitch=%d "
                  "posRotGround=%d",
                  getInstance != nullptr, player != nullptr, optionsField != nullptr,
                  getFps != nullptr, getYRot != nullptr, getXRot != nullptr,
                  setYRot != nullptr, setXRot != nullptr, getEye != nullptr,
                  yRotO != nullptr, xRotO != nullptr, vecX != nullptr, vecY != nullptr,
                  vecZ != nullptr, sensitivity != nullptr, optionValue != nullptr,
                  doubleValue != nullptr, gameRenderer != nullptr, pick != nullptr,
                  renderDistance != nullptr, fov != nullptr, intValue != nullptr,
                  posRotCtor != nullptr, posRotGetX != nullptr, posRotGetY != nullptr,
                  posRotGetZ != nullptr, posRotGetYaw != nullptr, posRotGetPitch != nullptr,
                  posRotGround != nullptr);
        }
        env->DeleteLocalRef(minecraft);
        env->DeleteLocalRef(options);
        env->DeleteLocalRef(option);
        env->DeleteLocalRef(vec3);
        env->DeleteLocalRef(boxed);
        env->DeleteLocalRef(renderer);
        env->DeleteLocalRef(boxedInt);
        env->DeleteLocalRef(posRot);
        return false;
    }

    g_rotation.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));
    g_rotation.posRot = static_cast<jclass>(env->NewGlobalRef(posRot));
    g_rotation.posRotCtor = posRotCtor;
    g_rotation.posRotGetX = posRotGetX;
    g_rotation.posRotGetY = posRotGetY;
    g_rotation.posRotGetZ = posRotGetZ;
    g_rotation.posRotGetYaw = posRotGetYaw;
    g_rotation.posRotGetPitch = posRotGetPitch;
    g_rotation.posRotGround = posRotGround;
    g_rotation.getInstance = getInstance;
    g_rotation.player = player;
    g_rotation.options = optionsField;
    g_rotation.getFps = getFps;
    g_rotation.getYRot = getYRot;
    g_rotation.getXRot = getXRot;
    g_rotation.setYRot = setYRot;
    g_rotation.setXRot = setXRot;
    g_rotation.getEye = getEye;
    g_rotation.yRotO = yRotO;
    g_rotation.xRotO = xRotO;
    g_rotation.vecX = vecX;
    g_rotation.vecY = vecY;
    g_rotation.vecZ = vecZ;
    g_rotation.sensitivity = sensitivity;
    g_rotation.optionValue = optionValue;
    g_rotation.doubleValue = doubleValue;
    g_rotation.gameRenderer = gameRenderer;
    g_rotation.pick = pick;
    g_rotation.renderDistance = renderDistance;
    g_rotation.fov = fov;
    g_rotation.intValue = intValue;

    env->DeleteLocalRef(renderer);
    env->DeleteLocalRef(boxedInt);
    env->DeleteLocalRef(posRot);
    env->DeleteLocalRef(minecraft);
    env->DeleteLocalRef(options);
    env->DeleteLocalRef(option);
    env->DeleteLocalRef(vec3);
    env->DeleteLocalRef(boxed);
    return g_rotation.minecraft != nullptr;
}

jobject Client(JNIEnv* env) {
    jobject instance = env->CallStaticObjectMethod(g_rotation.minecraft, g_rotation.getInstance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return instance;
}

bool OptionIntValue(JNIEnv* env, jmethodID accessor, int& out) {
    jobject client = Client(env);
    if (client == nullptr)
        return false;

    jobject options = env->GetObjectField(client, g_rotation.options);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (options == nullptr)
        return false;

    jobject instance = env->CallObjectMethod(options, accessor);
    env->DeleteLocalRef(options);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (instance == nullptr)
        return false;

    jobject value = env->GetObjectField(instance, g_rotation.optionValue);
    env->DeleteLocalRef(instance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (value == nullptr)
        return false;

    const jint result = env->CallIntMethod(value, g_rotation.intValue);
    env->DeleteLocalRef(value);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }

    out = static_cast<int>(result);
    return true;
}

bool EnsureResolved(JNIEnv* env) {
    if (g_rotation.minecraft != nullptr)
        return true;

    const auto now = std::chrono::steady_clock::now();
    if (g_rotationResolveFailed &&
        now - g_rotationResolveRetryAt < std::chrono::seconds(kRotationResolveRetrySeconds)) {
        return false;
    }

    if (Resolve(env))
        return true;

    g_rotationResolveFailed = true;
    g_rotationResolveRetryAt = now;
    return false;
}

jobject Player(JNIEnv* env) {
    jobject client = Client(env);
    if (client == nullptr)
        return nullptr;
    jobject player = env->GetObjectField(client, g_rotation.player);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return player;
}

}

bool PlayerRotation(float& yaw, float& pitch) {
    if (compat::UsesModernRuntime()) return compat::mc263::PlayerRotation(yaw, pitch);
    if (compat::IsLegacyLunar()) return compat::lunar::PlayerRotation(yaw, pitch);
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!EnsureResolved(env))
        return false;

    jobject player = Player(env);
    if (player == nullptr)
        return false;

    const jfloat outYaw = env->CallFloatMethod(player, g_rotation.getYRot);
    const jfloat outPitch = env->CallFloatMethod(player, g_rotation.getXRot);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }

    yaw = static_cast<float>(outYaw);
    pitch = static_cast<float>(outPitch);
    return true;
}

bool PlayerPreviousRotation(float& yaw, float& pitch) {
    if (compat::UsesModernRuntime()) return compat::mc263::PlayerRotation(yaw, pitch, true);
    if (compat::IsLegacyLunar()) return compat::lunar::PlayerRotation(yaw, pitch, true);
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!EnsureResolved(env))
        return false;

    jobject player = Player(env);
    if (player == nullptr)
        return false;

    yaw = static_cast<float>(env->GetFloatField(player, g_rotation.yRotO));
    pitch = static_cast<float>(env->GetFloatField(player, g_rotation.xRotO));
    const bool clean = !env->ExceptionCheck();
    if (env->ExceptionCheck())
        env->ExceptionClear();
    env->DeleteLocalRef(player);
    return clean;
}

bool SetPlayerRotation(float yaw, float pitch) {
    if(FreeCamActive())return false;
    if (compat::UsesModernRuntime()) return compat::mc263::SetPlayerRotation(yaw, pitch);
    if (compat::IsLegacyLunar()) return compat::lunar::SetPlayerRotation(yaw, pitch);
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!EnsureResolved(env))
        return false;

    jobject player = Player(env);
    if (player == nullptr)
        return false;

    env->CallVoidMethod(player, g_rotation.setYRot, static_cast<jfloat>(yaw));
    env->CallVoidMethod(player, g_rotation.setXRot, static_cast<jfloat>(pitch));
    const bool clean = !env->ExceptionCheck();
    if (env->ExceptionCheck())
        env->ExceptionClear();
    env->DeleteLocalRef(player);
    return clean;
}

bool PlayerEyePosition(double& x, double& y, double& z) {
    if (compat::UsesModernRuntime()) return compat::mc263::PlayerEyePosition(x, y, z);
    if (compat::IsLegacyLunar()) return compat::lunar::PlayerEyePosition(x, y, z);
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!EnsureResolved(env))
        return false;

    jobject player = Player(env);
    if (player == nullptr)
        return false;

    jobject eye = env->CallObjectMethod(player, g_rotation.getEye);
    env->DeleteLocalRef(player);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (eye == nullptr)
        return false;

    x = env->GetDoubleField(eye, g_rotation.vecX);
    y = env->GetDoubleField(eye, g_rotation.vecY);
    z = env->GetDoubleField(eye, g_rotation.vecZ);
    const bool clean = !env->ExceptionCheck();
    if (env->ExceptionCheck())
        env->ExceptionClear();
    env->DeleteLocalRef(eye);
    return clean;
}

bool PlayerSensitivity(double& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::PlayerSensitivity(out);
    if (compat::IsLegacyLunar()) return compat::lunar::PlayerSensitivity(out);
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!EnsureResolved(env))
        return false;

    jobject client = Client(env);
    if (client == nullptr)
        return false;

    jobject options = env->GetObjectField(client, g_rotation.options);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (options == nullptr)
        return false;

    jobject instance = env->CallObjectMethod(options, g_rotation.sensitivity);
    env->DeleteLocalRef(options);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (instance == nullptr)
        return false;

    jobject value = env->GetObjectField(instance, g_rotation.optionValue);
    env->DeleteLocalRef(instance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (value == nullptr)
        return false;

    const jdouble result = env->CallDoubleMethod(value, g_rotation.doubleValue);
    env->DeleteLocalRef(value);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }

    out = static_cast<double>(result);
    return true;
}

bool RenderDistance(int& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::RenderDistance(out);
    if (compat::IsLegacyLunar()) return compat::lunar::RenderDistance(out);
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;
    if (!EnsureResolved(env))
        return false;
    return OptionIntValue(env, g_rotation.renderDistance, out);
}

bool Fov(int& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::Fov(out);
    if (compat::IsLegacyLunar()) return compat::lunar::Fov(out);
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;
    if (!EnsureResolved(env))
        return false;
    return OptionIntValue(env, g_rotation.fov, out);
}

int ClientFps() {
    if (compat::UsesModernRuntime()) return compat::mc263::ClientFps();
    if (compat::IsLegacyLunar()) return compat::lunar::ClientFps();
    JniAttach attach;
    if (!attach)
        return 0;
    JNIEnv* env = attach.env;

    if (!EnsureResolved(env))
        return 0;

    jobject client = Client(env);
    if (client == nullptr)
        return 0;

    const jint fps = env->CallIntMethod(client, g_rotation.getFps);
    const bool clean = !env->ExceptionCheck();
    if (env->ExceptionCheck())
        env->ExceptionClear();
    env->DeleteLocalRef(client);
    return clean ? static_cast<int>(fps) : 0;
}

void PickCursor(float partialTicks) {
    if (compat::UsesModernRuntime()) { compat::mc263::PickCursor(partialTicks); return; }
    if (compat::IsLegacyLunar()) { compat::lunar::PickCursor(partialTicks); return; }
    JniAttach attach;
    if (!attach)
        return;
    JNIEnv* env = attach.env;

    if (!EnsureResolved(env))
        return;

    jobject client = Client(env);
    if (client == nullptr)
        return;

    jobject renderer = env->GetObjectField(client, g_rotation.gameRenderer);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return;
    }
    if (renderer == nullptr)
        return;

    env->CallVoidMethod(renderer, g_rotation.pick, static_cast<jfloat>(partialTicks));
    if (env->ExceptionCheck())
        env->ExceptionClear();
    env->DeleteLocalRef(renderer);
}

bool IsPosRot(void* packet) {
    if (compat::UsesModernRuntime()) return compat::mc263::IsPosRot(packet);
    if (compat::IsLegacyLunar()) return compat::lunar::IsPosRot(packet);
    if (!packet)
        return false;
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!EnsureResolved(env))
        return false;

    const jboolean match = env->IsInstanceOf(static_cast<jobject>(packet), g_rotation.posRot);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return match != JNI_FALSE;
}

bool ReadPosRot(void* packet, PosRot& out) {
    if (compat::UsesModernRuntime()) return compat::mc263::ReadPosRot(packet, out);
    if (compat::IsLegacyLunar()) return compat::lunar::ReadPosRot(packet, out);
    if (!packet)
        return false;
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!EnsureResolved(env))
        return false;

    jobject value = static_cast<jobject>(packet);
    if (!env->IsInstanceOf(value, g_rotation.posRot)) {
        if (env->ExceptionCheck())
            env->ExceptionClear();
        return false;
    }

    PosRot result;
    result.x = static_cast<double>(
        env->CallDoubleMethod(value, g_rotation.posRotGetX, 0.0));
    result.y = static_cast<double>(
        env->CallDoubleMethod(value, g_rotation.posRotGetY, 0.0));
    result.z = static_cast<double>(
        env->CallDoubleMethod(value, g_rotation.posRotGetZ, 0.0));
    result.yaw = static_cast<float>(
        env->CallFloatMethod(value, g_rotation.posRotGetYaw, 0.0f));
    result.pitch = static_cast<float>(
        env->CallFloatMethod(value, g_rotation.posRotGetPitch, 0.0f));
    result.onGround = env->CallBooleanMethod(value, g_rotation.posRotGround) != JNI_FALSE;

    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }

    out = result;
    return true;
}

void* MakePosRot(const PosRot& value) {
    if (compat::UsesModernRuntime()) return compat::mc263::MakePosRot(value);
    if (compat::IsLegacyLunar()) return compat::lunar::MakePosRot(value);
    JniAttach attach;
    if (!attach)
        return nullptr;
    JNIEnv* env = attach.env;

    if (!EnsureResolved(env))
        return nullptr;

    jobject packet = env->NewObject(g_rotation.posRot, g_rotation.posRotCtor,
                                    static_cast<jdouble>(value.x),
                                    static_cast<jdouble>(value.y),
                                    static_cast<jdouble>(value.z),
                                    static_cast<jfloat>(value.yaw),
                                    static_cast<jfloat>(value.pitch),
                                    value.onGround ? JNI_TRUE : JNI_FALSE);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return static_cast<void*>(packet);
}

}
