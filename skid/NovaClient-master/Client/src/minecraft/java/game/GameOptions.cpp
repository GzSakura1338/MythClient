#include "minecraft/java/api/game_options.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/minecraft263/Minecraft263.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"

#include "minecraft/java/api/base.h"
#include "minecraft/java/api/jni_lookup.h"

namespace nova::java {
namespace {

struct Handles {
    jclass    minecraft = nullptr;
    jmethodID getInstance = nullptr;
    jfieldID  options = nullptr;
    jfieldID  sprint = nullptr;
    jmethodID setDown = nullptr;
    jobject   sprintKey = nullptr;
};

Handles g_handles;
bool g_failureLogged = false;

#define NOVA_FORGE1201_OPTIONS
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_OPTIONS

bool Resolve(JNIEnv* env) {
    jobject loader = GetMinecraftClassLoader(env);
    jclass minecraft = nova_base::loadOrFind(env, loader, kMinecraftDot, kMinecraftSig);
    jclass options = nova_base::loadOrFind(env, loader, kOptionsDot, kOptionsSig);
    jclass keyMapping = nova_base::loadOrFind(env, loader, kKeyMappingDot, kKeyMappingSig);
    if (loader)
        env->DeleteGlobalRef(loader);

    if (!minecraft || !options || !keyMapping) {
        if (minecraft) env->DeleteLocalRef(minecraft);
        if (options) env->DeleteLocalRef(options);
        if (keyMapping) env->DeleteLocalRef(keyMapping);
        return false;
    }

    jmethodID getInstance = env->GetStaticMethodID(minecraft, kGetInstance, kGetInstanceDesc);
    jfieldID optionsField = env->GetFieldID(minecraft, kOptionsField, kOptionsSig);
    jfieldID sprintField = env->GetFieldID(options, kSprintField, kKeyMappingSig);
    jmethodID setDown = env->GetMethodID(keyMapping, kSetDown, kSetDownDesc);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    jobject instance = getInstance
        ? env->CallStaticObjectMethod(minecraft, getInstance) : nullptr;
    jobject gameOptions = (instance && optionsField)
        ? env->GetObjectField(instance, optionsField) : nullptr;
    jobject sprintKey = (gameOptions && sprintField)
        ? env->GetObjectField(gameOptions, sprintField) : nullptr;
    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (!getInstance || !optionsField || !sprintField || !setDown || !sprintKey) {
        if (!g_failureLogged) {
            g_failureLogged = true;
            LogTo("Base: sprint key unavailable (getInstance=%d, options=%d, "
                  "keySprint=%d, setDown=%d, instance=%d, key=%d)",
                  getInstance != nullptr, optionsField != nullptr,
                  sprintField != nullptr, setDown != nullptr,
                  instance != nullptr, sprintKey != nullptr);
        }
        if (instance) env->DeleteLocalRef(instance);
        if (gameOptions) env->DeleteLocalRef(gameOptions);
        if (sprintKey) env->DeleteLocalRef(sprintKey);
        env->DeleteLocalRef(minecraft);
        env->DeleteLocalRef(options);
        env->DeleteLocalRef(keyMapping);
        return false;
    }

    g_handles.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));
    g_handles.sprintKey = env->NewGlobalRef(sprintKey);
    g_handles.getInstance = getInstance;
    g_handles.options = optionsField;
    g_handles.sprint = sprintField;
    g_handles.setDown = setDown;

    env->DeleteLocalRef(instance);
    env->DeleteLocalRef(gameOptions);
    env->DeleteLocalRef(sprintKey);
    env->DeleteLocalRef(minecraft);
    env->DeleteLocalRef(options);
    env->DeleteLocalRef(keyMapping);
    return g_handles.minecraft && g_handles.sprintKey;
}

struct GammaHandles {
    jclass    minecraft = nullptr;
    jmethodID getInstance = nullptr;
    jfieldID  options = nullptr;
    jmethodID gamma = nullptr;
    jfieldID  value = nullptr;
    jobject   instance = nullptr;
    jclass    boxed = nullptr;
    jmethodID box = nullptr;
    jmethodID unbox = nullptr;
};

GammaHandles g_gamma;
bool g_gammaLogged = false;

bool ResolveGamma(JNIEnv* env) {
    jobject loader = GetMinecraftClassLoader(env);
    jclass minecraft = nova_base::loadOrFind(env, loader, kMinecraftDot, kMinecraftSig);
    jclass options = nova_base::loadOrFind(env, loader, kOptionsDot, kOptionsSig);
    jclass optionInstance =
        nova_base::loadOrFind(env, loader, kOptionInstanceDot, kOptionInstanceSig);
    if (loader)
        env->DeleteGlobalRef(loader);

    if (!minecraft || !options || !optionInstance) {
        if (minecraft) env->DeleteLocalRef(minecraft);
        if (options) env->DeleteLocalRef(options);
        if (optionInstance) env->DeleteLocalRef(optionInstance);
        return false;
    }

    jmethodID getInstance = env->GetStaticMethodID(minecraft, kGetInstance, kGetInstanceDesc);
    jfieldID optionsField = env->GetFieldID(minecraft, kOptionsField, kOptionsSig);
    jmethodID gamma = env->GetMethodID(options, kGamma, kGammaDesc);
    jfieldID value = env->GetFieldID(optionInstance, kValueField, kValueDesc);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    jclass boxed = env->FindClass(kDoubleDot);
    if (env->ExceptionCheck())
        env->ExceptionClear();
    jmethodID box = boxed ? env->GetStaticMethodID(boxed, kValueOf, kValueOfDesc) : nullptr;
    jmethodID unbox = boxed ? env->GetMethodID(boxed, kDoubleValue, kDoubleValueDesc) : nullptr;
    if (env->ExceptionCheck())
        env->ExceptionClear();

    jobject instance = getInstance ? env->CallStaticObjectMethod(minecraft, getInstance) : nullptr;
    jobject gameOptions = (instance && optionsField)
        ? env->GetObjectField(instance, optionsField) : nullptr;
    jobject gammaInstance = (gameOptions && gamma)
        ? env->CallObjectMethod(gameOptions, gamma) : nullptr;
    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (!getInstance || !optionsField || !gamma || !value || !box || !unbox || !gammaInstance) {
        if (!g_gammaLogged) {
            g_gammaLogged = true;
            LogTo("Base: gamma unavailable (getInstance=%d, options=%d, gamma=%d, value=%d, "
                  "box=%d, unbox=%d, instance=%d)",
                  getInstance != nullptr, optionsField != nullptr, gamma != nullptr,
                  value != nullptr, box != nullptr, unbox != nullptr, gammaInstance != nullptr);
        }
        if (instance) env->DeleteLocalRef(instance);
        if (gameOptions) env->DeleteLocalRef(gameOptions);
        if (gammaInstance) env->DeleteLocalRef(gammaInstance);
        env->DeleteLocalRef(minecraft);
        env->DeleteLocalRef(options);
        env->DeleteLocalRef(optionInstance);
        if (boxed) env->DeleteLocalRef(boxed);
        return false;
    }

    g_gamma.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));
    g_gamma.instance = env->NewGlobalRef(gammaInstance);
    g_gamma.boxed = static_cast<jclass>(env->NewGlobalRef(boxed));
    g_gamma.getInstance = getInstance;
    g_gamma.options = optionsField;
    g_gamma.gamma = gamma;
    g_gamma.value = value;
    g_gamma.box = box;
    g_gamma.unbox = unbox;

    env->DeleteLocalRef(instance);
    env->DeleteLocalRef(gameOptions);
    env->DeleteLocalRef(gammaInstance);
    env->DeleteLocalRef(minecraft);
    env->DeleteLocalRef(options);
    env->DeleteLocalRef(optionInstance);
    env->DeleteLocalRef(boxed);
    return g_gamma.minecraft && g_gamma.instance && g_gamma.boxed;
}

}

bool SetSprintKeyDown(bool down) {
    if (compat::UsesModernRuntime()) return compat::mc263::SetSprintKeyDown(down);
    if (compat::IsLegacyLunar()) return compat::lunar::SetSprintKeyDown(down);
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_handles.sprintKey && !Resolve(env))
        return false;

    env->CallVoidMethod(g_handles.sprintKey, g_handles.setDown, down ? JNI_TRUE : JNI_FALSE);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

bool GetGamma(double* out) {
    if (compat::UsesModernRuntime()) return compat::mc263::GetGamma(out);
    if (compat::IsLegacyLunar()) return compat::lunar::GetGamma(out);
    if (out == nullptr)
        return false;

    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_gamma.instance && !ResolveGamma(env))
        return false;

    jobject boxed = env->GetObjectField(g_gamma.instance, g_gamma.value);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (boxed == nullptr)
        return false;

    const jdouble value = env->CallDoubleMethod(boxed, g_gamma.unbox);
    env->DeleteLocalRef(boxed);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }

    *out = value;
    return true;
}

bool SetGamma(double value) {
    if (compat::UsesModernRuntime()) return compat::mc263::SetGamma(value);
    if (compat::IsLegacyLunar()) return compat::lunar::SetGamma(value);
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_gamma.instance && !ResolveGamma(env))
        return false;

    jobject boxed =
        env->CallStaticObjectMethod(g_gamma.boxed, g_gamma.box, static_cast<jdouble>(value));
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (boxed == nullptr)
        return false;

    env->SetObjectField(g_gamma.instance, g_gamma.value, boxed);
    env->DeleteLocalRef(boxed);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

}
