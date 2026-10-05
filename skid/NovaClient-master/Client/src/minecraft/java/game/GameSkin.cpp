#include "minecraft/java/api/game_skin.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"

#include <chrono>

#include "minecraft/java/api/jni_lookup.h"

// The target HUD's slice of the game: the GL texture a player's skin lives in.
//
// A sixth self-contained handle set, for the reason the earlier ones are. The chain
// is short -- AbstractClientPlayer.getSkinTextureLocation -> Minecraft's
// TextureManager.getTexture -> AbstractTexture.getId -- but it is four members this
// client has no other use for, so it resolves together and stands down together.
//
// The id is an OpenGL texture name rather than anything the game will look up
// again, which is what makes it safe to carry across the thread boundary: the
// overlay binds it by hand with ImGui's AddImage, the same way it binds the font.

namespace nova::java {
namespace {

#define NOVA_FORGE1201_SKIN
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_SKIN

struct SkinHandles {
    jclass    minecraft = nullptr;
    jclass    playerClass = nullptr;

    jmethodID getInstance = nullptr;
    jmethodID getSkin = nullptr;
    jmethodID getTextureManager = nullptr;
    jmethodID getTexture = nullptr;
    jmethodID getId = nullptr;

    bool ok = false;
};

SkinHandles g_skin;
bool g_skinLogged = false;
bool g_skinResolveFailed = false;
std::chrono::steady_clock::time_point g_skinRetryAt;

constexpr int kResolveRetrySeconds = 10;

// A failed GetMethodID leaves a NoSuchMethodError pending, and the JVM then fails
// every lookup after it -- which turns one absent member into a cascade and hides
// which one was really missing. Each lookup gets its own answer.
jmethodID Method(JNIEnv* env, jclass cls, const char* name, const char* desc) {
    if (cls == nullptr)
        return nullptr;
    jmethodID id = env->GetMethodID(cls, name, desc);
    if (env->ExceptionCheck())
        env->ExceptionClear();
    return id;
}

bool ResolveSkin(JNIEnv* env) {
    if (!env)
        return false;
    if (g_skin.ok)
        return true;
    if (g_skinResolveFailed && std::chrono::steady_clock::now() < g_skinRetryAt)
        return false;

    jobject loader = GetMinecraftClassLoader(env);
    jclass minecraft = nova_base::loadOrFind(env, loader, kMinecraftDot, kMinecraftSig);
    jclass player = nova_base::loadOrFind(env, loader, kPlayerDot, kPlayerSig);
    jclass manager = nova_base::loadOrFind(env, loader, kManagerDot, kManagerSig);
    jclass texture = nova_base::loadOrFind(env, loader, kTextureDot, kTextureSig);
    jclass location = nova_base::loadOrFind(env, loader, kLocationDot, kLocationSig);
    if (loader)
        env->DeleteGlobalRef(loader);

    jmethodID getInstance = nullptr;
    if (minecraft != nullptr) {
        getInstance = env->GetStaticMethodID(minecraft, kGetInstance, kGetInstanceDesc);
        if (env->ExceptionCheck())
            env->ExceptionClear();
    }
    jmethodID getSkin = Method(env, player, kGetSkin, kGetSkinDesc);
    jmethodID getTextureManager = Method(env, minecraft, kGetManager, kGetManagerDesc);
    jmethodID getTexture = Method(env, manager, kGetTexture, kGetTextureDesc);
    jmethodID getId = Method(env, texture, kGetId, kGetIdDesc);

    const bool ok = minecraft && player && manager && texture && getInstance && getSkin &&
                    getTextureManager && getTexture && getId;

    if (!ok) {
        g_skinResolveFailed = true;
        g_skinRetryAt =
            std::chrono::steady_clock::now() + std::chrono::seconds(kResolveRetrySeconds);
        if (!g_skinLogged) {
            g_skinLogged = true;
            LogTo("Base: skin layer incomplete -> mc=%d player=%d manager=%d texture=%d "
                  "instance=%d skin=%d textureManager=%d getTexture=%d getId=%d",
                  minecraft != nullptr, player != nullptr, manager != nullptr,
                  texture != nullptr, getInstance != nullptr, getSkin != nullptr,
                  getTextureManager != nullptr, getTexture != nullptr, getId != nullptr);
        }
    } else {
        g_skin.minecraft = static_cast<jclass>(env->NewGlobalRef(minecraft));
        g_skin.playerClass = static_cast<jclass>(env->NewGlobalRef(player));
        g_skin.getInstance = getInstance;
        g_skin.getSkin = getSkin;
        g_skin.getTextureManager = getTextureManager;
        g_skin.getTexture = getTexture;
        g_skin.getId = getId;
        g_skin.ok = g_skin.minecraft && g_skin.playerClass;
    }

    if (minecraft) env->DeleteLocalRef(minecraft);
    if (player) env->DeleteLocalRef(player);
    if (manager) env->DeleteLocalRef(manager);
    if (texture) env->DeleteLocalRef(texture);
    if (location) env->DeleteLocalRef(location);
    return g_skin.ok;
}

}  // namespace

std::uint64_t PlayerSkinTexture(const EntityHandle& entity) {
    if (compat::UsesModernRuntime()) return compat::mc263::PlayerSkinTexture(entity);
    if (compat::IsLegacyLunar()) return compat::lunar::PlayerSkinTexture(entity);
    if (!entity.Valid())
        return 0;

    JniAttach attach;
    if (!attach || !ResolveSkin(attach.env))
        return 0;
    JNIEnv* env = attach.env;

    jobject player = static_cast<jobject>(entity.raw());
    if (env->IsInstanceOf(player, g_skin.playerClass) != JNI_TRUE) {
        if (env->ExceptionCheck())
            env->ExceptionClear();
        return 0;
    }

    jobject location = env->CallObjectMethod(player, g_skin.getSkin);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return 0;
    }
    if (location == nullptr)
        return 0;

    jobject client = env->CallStaticObjectMethod(g_skin.minecraft, g_skin.getInstance);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        client = nullptr;
    }

    jobject manager = nullptr;
    if (client != nullptr) {
        manager = env->CallObjectMethod(client, g_skin.getTextureManager);
        env->DeleteLocalRef(client);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            manager = nullptr;
        }
    }

    unsigned int texture = 0;
    if (manager != nullptr) {
        jobject found = env->CallObjectMethod(manager, g_skin.getTexture, location);
        env->DeleteLocalRef(manager);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            found = nullptr;
        }
        if (found != nullptr) {
            // A skin whose image has not been stitched yet still has a texture
            // object; its GL name is only real once the upload has happened, and
            // until then it reports a non-positive id.
            const jint id = env->CallIntMethod(found, g_skin.getId);
            if (env->ExceptionCheck())
                env->ExceptionClear();
            else if (id > 0)
                texture = static_cast<unsigned int>(id);
            env->DeleteLocalRef(found);
        }
    }

    env->DeleteLocalRef(location);
    return texture;
}

}
