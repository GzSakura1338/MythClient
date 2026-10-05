#pragma once

#include "minecraft/java/api/base.h"

namespace nova::hook {

bool EnableTransformer(jvmtiEnv* jvmti, JNIEnv* env);

bool DisableTransformer(jvmtiEnv* jvmti, JNIEnv* env);
bool WaitForQuiescence(jvmtiEnv* jvmti, JNIEnv* env, unsigned timeoutMs);

int TransformedCount();

}
