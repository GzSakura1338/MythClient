#pragma once

#include "minecraft/java/api/hook_bridge.h"

namespace nova::hook {

bool Prepare(JNIEnv* env, jobject mcLoader);

void Release(JNIEnv* env);

}
