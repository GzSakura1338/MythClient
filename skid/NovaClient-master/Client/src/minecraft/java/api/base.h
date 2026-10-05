
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "third_party/jni/jni.h"
#include "third_party/jni/jvmti.h"

#include <string>
#include "core/RuntimeControl.h"

extern JavaVM*    g_vm;
extern jvmtiEnv*  g_jvmti;

struct JniAttach {
    JNIEnv* env = nullptr;
    bool    attached = false;

    JniAttach();
    ~JniAttach();
    JniAttach(const JniAttach&) = delete;
    JniAttach& operator=(const JniAttach&) = delete;

    JNIEnv* operator->() const { return env; }
    explicit operator bool() const { return env != nullptr; }
};

void LogAndClearException(JNIEnv* env, const char* where);

jvmtiEnv* CurrentJvmti();
// Only after all callbacks/users of the runtime environment have drained.
bool DisposeJvmti();

jobject GetMinecraftClassLoader(JNIEnv* env);

jclass LoadClassInLoader(JNIEnv* env, jobject classLoader, const char* dotName);

extern "C" void InitializeBase(void* parameters);
