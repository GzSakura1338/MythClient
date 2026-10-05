#pragma once
#include "minecraft/compat/minecraft263/Minecraft263.h"

namespace nova::compat::neoforge12110 {
namespace names {
#define NOVA_NEOFORGE12110_MEMBER(id,owner,name,desc) inline constexpr mc263::names::Member id{owner,name,desc};
#include "minecraft/compat/mappings/neoforge12110.inc"
#undef NOVA_NEOFORGE12110_MEMBER
}
const char* ClassName(const char* name);
mc263::names::Member Translate(mc263::names::Member member);
bool Initialize(JNIEnv* env);
void Shutdown(JNIEnv* env);
jobject Screen(JNIEnv* env,jobject minecraft);
bool SetTimerSpeed(float speed);
float TimerMillis(float vanilla);
}
