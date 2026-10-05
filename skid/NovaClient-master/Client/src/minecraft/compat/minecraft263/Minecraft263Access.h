#pragma once
#include "minecraft/compat/minecraft263/Minecraft263.h"
#include "minecraft/compat/Profile.h"
#include <type_traits>

namespace nova::compat::mc263::access {
using names::Member;
constexpr bool ValidMember(Member m) {
    return m.owner && *m.owner && m.name && *m.name && m.descriptor && *m.descriptor;
}
jclass Class(JNIEnv*, const char*);
jmethodID Method(JNIEnv*, Member, bool isStatic = false);
jfieldID Field(JNIEnv*, Member, bool isStatic = false);
void Release(JNIEnv*);

// An operation owns its JNI locals. No cached JNIEnv or game object crosses threads.
struct Frame {
    JniAttach attach;
    JNIEnv* env;
    bool valid;
    bool ok = true;
    explicit Frame(JNIEnv* supplied = nullptr) : env(supplied ? supplied : attach.env),
        valid(env && env->PushLocalFrame(128) == JNI_OK) {}
    ~Frame() { if (valid) { check(); env->PopLocalFrame(nullptr); } }
    explicit operator bool() const { return valid && ok; }
    bool check() {
        if (!valid) return false;
        if (env->ExceptionCheck()) { env->ExceptionClear(); ok = false; }
        return ok;
    }
    jobject keep(jobject obj) {
        if (!check()) return nullptr;
        valid = false;
        return env->PopLocalFrame(obj);
    }
    jobject client() { return valid ? compat::Minecraft(env) : nullptr; }
    jobject player() { return valid ? compat::LocalPlayer(env) : nullptr; }
    jobject level() { return valid ? get(client(), names::Minecraft_Field_level) : nullptr; }
    bool instance(jobject obj, const char* name) {
        if (!obj || !check()) return false;
        auto type = Class(env, name);
        return type && env->IsInstanceOf(obj, type) && check();
    }
    template<typename T = jobject, typename... A> T call(jobject obj, Member m, A... args) {
        if (!obj || !check()) return T{};
        auto id = Method(env, m);
        if (!id) { ok = false; return T{}; }
        T result{};
        if constexpr (std::is_same_v<T, bool>) result = env->CallBooleanMethod(obj, id, args...) != JNI_FALSE;
        else if constexpr (std::is_same_v<T, int>) result = env->CallIntMethod(obj, id, args...);
        else if constexpr (std::is_same_v<T, float>) result = env->CallFloatMethod(obj, id, args...);
        else if constexpr (std::is_same_v<T, double>) result = env->CallDoubleMethod(obj, id, args...);
        else if constexpr (std::is_same_v<T, jlong>) result = env->CallLongMethod(obj, id, args...);
        else result = static_cast<T>(env->CallObjectMethod(obj, id, args...));
        check(); return result;
    }
    template<typename... A> bool invoke(jobject obj, Member m, A... args) {
        if (!obj || !check()) return false;
        auto id = Method(env, m);
        if (!id) { ok = false; return false; }
        env->CallVoidMethod(obj, id, args...); return check();
    }
    template<typename... A> jobject create(Member m, A... args) {
        if (!check()) return nullptr;
        auto id = Method(env, m);
        if (!id) { ok = false; return nullptr; }
        auto result = env->NewObject(Class(env, m.owner), id, args...);
        return check() ? result : nullptr;
    }
    template<typename... A> jobject staticCall(Member m, A... args) {
        if (!check()) return nullptr;
        auto id = Method(env, m, true);
        if (!id) { ok = false; return nullptr; }
        auto result = env->CallStaticObjectMethod(Class(env, m.owner), id, args...);
        return check() ? result : nullptr;
    }
    template<typename T = jobject> T get(jobject obj, Member m) {
        if (!obj || !check()) return T{};
        auto id = Field(env, m);
        if (!id) { ok = false; return T{}; }
        T result{};
        if constexpr (std::is_same_v<T, bool>) result = env->GetBooleanField(obj, id) != JNI_FALSE;
        else if constexpr (std::is_same_v<T, int>) result = env->GetIntField(obj, id);
        else if constexpr (std::is_same_v<T, float>) result = env->GetFloatField(obj, id);
        else if constexpr (std::is_same_v<T, double>) result = env->GetDoubleField(obj, id);
        else if constexpr (std::is_same_v<T, jlong>) result = env->GetLongField(obj, id);
        else result = static_cast<T>(env->GetObjectField(obj, id));
        check(); return result;
    }
    template<typename T> bool set(jobject obj, Member m, T value) {
        if (!obj || !check()) return false;
        auto id = Field(env, m);
        if (!id) { ok = false; return false; }
        if constexpr (std::is_same_v<T, bool>) env->SetBooleanField(obj, id, value ? JNI_TRUE : JNI_FALSE);
        else if constexpr (std::is_same_v<T, int>) env->SetIntField(obj, id, value);
        else if constexpr (std::is_same_v<T, float>) env->SetFloatField(obj, id, value);
        else if constexpr (std::is_same_v<T, double>) env->SetDoubleField(obj, id, value);
        else env->SetObjectField(obj, id, value);
        return check();
    }
    jobject stat(Member m) {
        if (!check()) return nullptr;
        auto id = Field(env, m, true);
        if (!id) { ok = false; return nullptr; }
        auto result = env->GetStaticObjectField(Class(env, m.owner), id);
        return check() ? result : nullptr;
    }
    std::string text(jobject obj) {
        if (!obj || !check()) return {};
        const char* chars = env->GetStringUTFChars(static_cast<jstring>(obj), nullptr);
        if (!chars) { check(); return {}; }
        std::string result(chars);
        env->ReleaseStringUTFChars(static_cast<jstring>(obj), chars);
        return result;
    }
};
}
