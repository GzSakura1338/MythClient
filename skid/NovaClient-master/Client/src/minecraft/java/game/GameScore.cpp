#include "minecraft/java/api/game_score.h"
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include "minecraft/compat/minecraft263/Minecraft263Game.h"

#include <chrono>

#include "minecraft/java/api/jni_lookup.h"

// The health bypass's slice of the game: one packet, and the three fields on it.
//
// A seventh self-contained handle set, for the reason the earlier ones are. The
// class is a plain record -- owner, objective, score, and whether the score is being
// set or taken away -- and the whole read is four calls, but it is four members this
// client has no other use for, so it resolves together and stands down together.

namespace nova::java {
namespace {

#define NOVA_FORGE1201_SCORE
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_SCORE

// ServerScoreboard.Method is CHANGE then REMOVE, and only the first one carries a
// score on the wire.
constexpr jint kMethodRemove = 1;

// The two objectives the original accepts. Both spellings are out there: "health"
// is the common one, "belowHealth" is what the health-bar plugins that hide the
// vanilla bar use.
constexpr const char* kObjectives[] = {"health", "belowHealth"};

struct ScoreHandles {
    jclass    packet = nullptr;

    jmethodID getOwner = nullptr;
    jmethodID getObjective = nullptr;
    jmethodID getScore = nullptr;
    jmethodID getMethod = nullptr;
    jmethodID ordinal = nullptr;

    bool ok = false;
};

ScoreHandles g_score;
bool g_scoreLogged = false;
bool g_scoreResolveFailed = false;
std::chrono::steady_clock::time_point g_scoreRetryAt;

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

bool ResolveScore(JNIEnv* env) {
    if (!env)
        return false;
    if (g_score.ok)
        return true;
    if (g_scoreResolveFailed && std::chrono::steady_clock::now() < g_scoreRetryAt)
        return false;

    jobject loader = GetMinecraftClassLoader(env);
    jclass packet = nova_base::loadOrFind(env, loader, kPacketDot, kPacketSig);
    jclass method = nova_base::loadOrFind(env, loader, kMethodDot, kMethodSig);
    jclass enumClass = nova_base::loadOrFind(env, loader, kEnumDot, kEnumSig);
    if (loader)
        env->DeleteGlobalRef(loader);

    jmethodID getOwner = Method(env, packet, kGetOwner, kStringDesc);
    jmethodID getObjective = Method(env, packet, kGetObjective, kStringDesc);
    jmethodID getScore = Method(env, packet, kGetScore, kIntDesc);
    jmethodID getMethod = Method(env, packet, kGetMethod, kMethodDesc);
    jmethodID ordinal = Method(env, enumClass, kMappedOrdinal, kOrdinalDesc);

    const bool ok = packet && method && enumClass && getOwner && getObjective && getScore &&
                    getMethod && ordinal;

    if (!ok) {
        g_scoreResolveFailed = true;
        g_scoreRetryAt =
            std::chrono::steady_clock::now() + std::chrono::seconds(kResolveRetrySeconds);
        if (!g_scoreLogged) {
            g_scoreLogged = true;
            LogTo("Base: score layer incomplete -> packet=%d method=%d enum=%d owner=%d "
                  "objective=%d score=%d getMethod=%d ordinal=%d",
                  packet != nullptr, method != nullptr, enumClass != nullptr,
                  getOwner != nullptr, getObjective != nullptr, getScore != nullptr,
                  getMethod != nullptr, ordinal != nullptr);
        }
    } else {
        g_score.packet = static_cast<jclass>(env->NewGlobalRef(packet));
        g_score.getOwner = getOwner;
        g_score.getObjective = getObjective;
        g_score.getScore = getScore;
        g_score.getMethod = getMethod;
        g_score.ordinal = ordinal;
        g_score.ok = g_score.packet != nullptr;
    }

    if (packet) env->DeleteLocalRef(packet);
    if (method) env->DeleteLocalRef(method);
    if (enumClass) env->DeleteLocalRef(enumClass);
    return g_score.ok;
}

// A Java string as UTF-8, through a local reference the caller still owns.
bool ReadString(JNIEnv* env, jobject text, std::string& out) {
    if (text == nullptr)
        return false;
    const char* utf = env->GetStringUTFChars(static_cast<jstring>(text), nullptr);
    if (utf != nullptr) {
        out = utf;
        env->ReleaseStringUTFChars(static_cast<jstring>(text), utf);
    }
    return !env->ExceptionCheck();
}

}  // namespace

bool ReadHealthScore(jobject packet, std::string& owner, int& score) {
    if (compat::UsesModernRuntime()) return compat::mc263::ReadHealthScore(packet, owner, score);
    if (compat::IsLegacyLunar()) return compat::lunar::ReadHealthScore(packet, owner, score);
    owner.clear();
    score = 0;
    if (packet == nullptr)
        return false;

    JniAttach attach;
    if (!attach || !ResolveScore(attach.env))
        return false;
    JNIEnv* env = attach.env;

    if (env->IsInstanceOf(packet, g_score.packet) != JNI_TRUE) {
        if (env->ExceptionCheck())
            env->ExceptionClear();
        return false;
    }

    jobject kind = env->CallObjectMethod(packet, g_score.getMethod);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        kind = nullptr;
    }
    if (kind == nullptr)
        return false;
    const jint ordinal = env->CallIntMethod(kind, g_score.ordinal);
    env->DeleteLocalRef(kind);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    if (ordinal == kMethodRemove)
        return false;

    jobject objective = env->CallObjectMethod(packet, g_score.getObjective);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        objective = nullptr;
    }
    if (objective == nullptr)
        return false;

    std::string name;
    const bool readName = ReadString(env, objective, name);
    env->DeleteLocalRef(objective);
    if (!readName)
        return false;

    bool wanted = false;
    for (const char* candidate : kObjectives) {
        if (name == candidate) {
            wanted = true;
            break;
        }
    }
    if (!wanted)
        return false;

    jobject ownerName = env->CallObjectMethod(packet, g_score.getOwner);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        ownerName = nullptr;
    }
    if (ownerName == nullptr)
        return false;
    const bool readOwner = ReadString(env, ownerName, owner);
    env->DeleteLocalRef(ownerName);
    if (!readOwner || owner.empty())
        return false;

    const jint value = env->CallIntMethod(packet, g_score.getScore);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        owner.clear();
        return false;
    }
    score = static_cast<int>(value);
    return true;
}

bool ReadStoredHealthScores(std::unordered_map<std::string,int>& scores,std::string* objectives) {
    scores.clear();if(objectives)objectives->clear();
    if(compat::IsLunar12111())return compat::mc263::ReadStoredHealthScores(scores,objectives);
    return false;
}

}
