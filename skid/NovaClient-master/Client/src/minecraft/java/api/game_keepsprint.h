#pragma once
#include "minecraft/java/api/base.h"

namespace nova::java {
// Attack callbacks are synchronous on the game thread; remote/server players
// never touch the scratch state. No static TLS or retained JNI references.
double BeginHitSlowdown(JNIEnv* env, jobject player);
double HitSlowdown();
bool EndHitSprint(bool original);
void JNICALL AttackVelocityHook(JNIEnv*, jclass, jobject player, jobject velocity);
void JNICALL AttackSprintHook(JNIEnv*, jclass, jobject player, jboolean sprint);
}
