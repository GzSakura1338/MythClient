#include "minecraft/java/api/game_input.h"
#include "minecraft/java/api/game_freecam.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"

#include "minecraft/java/api/base.h"
#include "minecraft/java/api/jni_lookup.h"

namespace nova::java {
namespace {

struct Handles {
    jclass    minecraft = nullptr;
    jmethodID getInstance = nullptr;
    jfieldID  mouseHandler = nullptr;
    jmethodID grabMouse = nullptr;
    jmethodID releaseMouse = nullptr;
};

Handles g_handles;
bool g_failureLogged = false;

#define NOVA_FORGE1201_INPUT
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_INPUT

bool Resolve(JNIEnv* env) {
    jobject loader = GetMinecraftClassLoader(env);
    jclass minecraft = nova_base::loadOrFind(env, loader, kMinecraftDot, kMinecraftSig);
    jclass handlerClass = nova_base::loadOrFind(env, loader, kMouseHandlerDot,
                                                kMouseHandlerSig);
    if (loader)
        env->DeleteGlobalRef(loader);
    if (!minecraft || !handlerClass) {
        if (minecraft) env->DeleteLocalRef(minecraft);
        if (handlerClass) env->DeleteLocalRef(handlerClass);
        return false;
    }

    jmethodID getInstance = env->GetStaticMethodID(minecraft, kGetInstance, kGetInstanceDesc);
    jfieldID mouseHandler = env->GetFieldID(minecraft, kMouseHandler, kMouseHandlerSig);
    jmethodID grabMouse = env->GetMethodID(handlerClass, kGrabMouse, kVoidDesc);
    jmethodID releaseMouse = env->GetMethodID(handlerClass, kReleaseMouse, kVoidDesc);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (!getInstance || !mouseHandler || !grabMouse || !releaseMouse) {
        env->DeleteLocalRef(handlerClass);
        env->DeleteLocalRef(minecraft);
        if (!g_failureLogged) {
            g_failureLogged = true;
            LogTo("Base: mouse release unavailable (client=%d, handler=%d, grab=%d, release=%d)",
                  getInstance != nullptr, mouseHandler != nullptr,
                  grabMouse != nullptr, releaseMouse != nullptr);
        }
        return false;
    }

    g_handles.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));
    g_handles.getInstance = getInstance;
    g_handles.mouseHandler = mouseHandler;
    g_handles.grabMouse = grabMouse;
    g_handles.releaseMouse = releaseMouse;
    env->DeleteLocalRef(handlerClass);
    env->DeleteLocalRef(minecraft);
    return g_handles.minecraft != nullptr;
}

struct ClickHandles {
    jclass    minecraft = nullptr;
    jmethodID getInstance = nullptr;
    jfieldID  mouseHandler = nullptr;
    jfieldID  grabbed = nullptr;
    jclass    keyMapping = nullptr;
    jobject   mouseKey = nullptr;
    jmethodID clickKey = nullptr;
    jmethodID setKey = nullptr;
};

ClickHandles g_click;
bool g_clickLogged = false;

constexpr jint kAttackMouseButton = 0;

bool ResolveClick(JNIEnv* env) {
    jobject loader = GetMinecraftClassLoader(env);
    jclass minecraft = nova_base::loadOrFind(env, loader, kMinecraftDot, kMinecraftSig);
    jclass keyMapping = nova_base::loadOrFind(env, loader, kKeyMappingDot, kKeyMappingSig);
    jclass inputType = nova_base::loadOrFind(env, loader, kInputTypeDot, kInputTypeSig);
    jclass handlerClass = nova_base::loadOrFind(env, loader, kMouseHandlerDot, kMouseHandlerSig);
    if (loader)
        env->DeleteGlobalRef(loader);

    if (!minecraft || !keyMapping || !inputType || !handlerClass) {
        if (minecraft) env->DeleteLocalRef(minecraft);
        if (keyMapping) env->DeleteLocalRef(keyMapping);
        if (inputType) env->DeleteLocalRef(inputType);
        if (handlerClass) env->DeleteLocalRef(handlerClass);
        return false;
    }

    jmethodID getInstance = env->GetStaticMethodID(minecraft, kGetInstance, kGetInstanceDesc);
    jfieldID mouseHandler = env->GetFieldID(minecraft, kMouseHandler, kMouseHandlerSig);
    jfieldID grabbed = env->GetFieldID(handlerClass, kMouseGrabbed, kMappedPrimitiveBooleanSig);
    jfieldID mouseType = env->GetStaticFieldID(inputType, kMouseType, kInputTypeSig);
    jmethodID getOrCreate = env->GetMethodID(inputType, kGetOrCreate, kGetOrCreateDesc);
    jmethodID clickKey = env->GetStaticMethodID(keyMapping, kClickKey, kClickKeyDesc);
    jmethodID setKey = env->GetStaticMethodID(keyMapping, kSetKey, kSetKeyDesc);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    jobject type = mouseType ? env->GetStaticObjectField(inputType, mouseType) : nullptr;
    jobject mouseKey = (type && getOrCreate)
        ? env->CallObjectMethod(type, getOrCreate, kAttackMouseButton) : nullptr;
    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (!getInstance || !mouseHandler || !grabbed || !clickKey || !setKey || !mouseKey) {
        if (!g_clickLogged) {
            g_clickLogged = true;
            LogTo("Base: attack click unavailable (getInstance=%d, mouseHandler=%d, grabbed=%d, "
                  "click=%d, set=%d, mouseKey=%d)",
                  getInstance != nullptr, mouseHandler != nullptr, grabbed != nullptr,
                  clickKey != nullptr, setKey != nullptr, mouseKey != nullptr);
        }
        if (type) env->DeleteLocalRef(type);
        if (mouseKey) env->DeleteLocalRef(mouseKey);
        env->DeleteLocalRef(minecraft);
        env->DeleteLocalRef(keyMapping);
        env->DeleteLocalRef(inputType);
        env->DeleteLocalRef(handlerClass);
        return false;
    }

    g_click.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));
    g_click.keyMapping = static_cast<jclass>(env->NewGlobalRef(keyMapping));
    g_click.mouseKey = env->NewGlobalRef(mouseKey);
    g_click.getInstance = getInstance;
    g_click.mouseHandler = mouseHandler;
    g_click.grabbed = grabbed;
    g_click.clickKey = clickKey;
    g_click.setKey = setKey;

    env->DeleteLocalRef(type);
    env->DeleteLocalRef(mouseKey);
    env->DeleteLocalRef(minecraft);
    env->DeleteLocalRef(keyMapping);
    env->DeleteLocalRef(inputType);
    env->DeleteLocalRef(handlerClass);
    return g_click.minecraft && g_click.keyMapping && g_click.mouseKey;
}

bool OnMouseHandler(jmethodID Handles::* which) {
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_handles.minecraft && !Resolve(env))
        return false;

    jobject client = env->CallStaticObjectMethod(g_handles.minecraft, g_handles.getInstance);
    if (env->ExceptionCheck() || !client) {
        env->ExceptionClear();
        return false;
    }

    jobject handler = env->GetObjectField(client, g_handles.mouseHandler);
    if (env->ExceptionCheck())
        env->ExceptionClear();

    bool clean = false;
    if (handler) {
        env->CallVoidMethod(handler, g_handles.*which);
        clean = !env->ExceptionCheck();
        env->ExceptionClear();
        env->DeleteLocalRef(handler);
    }
    env->DeleteLocalRef(client);
    return clean;
}

}

bool ReleaseMouse() { return (compat::IsLegacyLunar() || compat::UsesModernRuntime()) ? compat::CaptureMouse(true) : OnMouseHandler(&Handles::releaseMouse); }

bool GrabMouse() { return (compat::IsLegacyLunar() || compat::UsesModernRuntime()) ? compat::CaptureMouse(false) : OnMouseHandler(&Handles::grabMouse); }

bool MouseGrabbed() {
    if (compat::UsesModernRuntime()) return compat::mc263::MouseGrabbed();
    if (compat::IsLegacyLunar()) return compat::lunar::MouseGrabbed();
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_click.minecraft && !ResolveClick(env))
        return false;

    jobject client = env->CallStaticObjectMethod(g_click.minecraft, g_click.getInstance);
    if (env->ExceptionCheck() || !client) {
        env->ExceptionClear();
        return false;
    }

    jobject handler = env->GetObjectField(client, g_click.mouseHandler);
    env->DeleteLocalRef(client);
    if (env->ExceptionCheck())
        env->ExceptionClear();
    if (!handler)
        return false;

    const jboolean grabbed = env->GetBooleanField(handler, g_click.grabbed);
    env->DeleteLocalRef(handler);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return grabbed == JNI_TRUE;
}

bool ClickAttack() {
    if (FreeCamBlocksMouse(false)) return false;
    if (compat::UsesModernRuntime()) return compat::mc263::ClickAttack();
    if (compat::IsLegacyLunar()) return compat::lunar::ClickAttack();
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_click.minecraft && !ResolveClick(env))
        return false;

    env->CallStaticVoidMethod(g_click.keyMapping, g_click.clickKey, g_click.mouseKey);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

bool SetAttackDown(bool down) {
    if (compat::UsesModernRuntime()) return compat::mc263::SetAttackDown(down);
    if (compat::IsLegacyLunar()) return compat::lunar::SetAttackDown(down);
    JniAttach attach;
    if (!attach)
        return false;
    JNIEnv* env = attach.env;

    if (!g_click.minecraft && !ResolveClick(env))
        return false;

    env->CallStaticVoidMethod(g_click.keyMapping, g_click.setKey, g_click.mouseKey,
                              down ? JNI_TRUE : JNI_FALSE);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

}
