#pragma once
#include "minecraft/java/api/base.h"
#include <string_view>

namespace nova::module::imefix {

// Lunar 1.8.9 / 1.21.11 adapter. Calls run on the verified client thread,
// except Release after the host has stopped callbacks. No cached JNIEnv.
class ChatInput {
public:
    bool Resolve(JNIEnv* env);
    bool OnClientThread(JNIEnv* env);
    bool RefreshTarget(JNIEnv* env, bool& changed);
    bool Insert(JNIEnv* env, std::u16string_view text);
    void ClearTarget(JNIEnv* env);
    void Release(JNIEnv* env);
private:
    jclass minecraft_ = nullptr, chat_ = nullptr, edit_ = nullptr;
    jmethodID getInstance_ = nullptr, sameThread_ = nullptr;
    jmethodID canConsume_ = nullptr, insert_ = nullptr, getValue_ = nullptr;
    jfieldID screenField_ = nullptr, inputField_ = nullptr, worldField_ = nullptr;
    jfieldID enabledField_ = nullptr;
    jobject screen_ = nullptr, input_ = nullptr, world_ = nullptr;
    unsigned reports_ = 0;
};

}
