#pragma once
#include "minecraft/compat/Profile.h"
#include "minecraft/compat/lunar12111/Mapping.h"
namespace nova::compat::lunar12111 {
bool Initialize(JNIEnv* env);
jobject Screen(JNIEnv* env,jobject client);
void Shutdown(JNIEnv* env);
}
