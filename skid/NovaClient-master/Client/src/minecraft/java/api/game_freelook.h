#pragma once
#include "minecraft/java/api/base.h"
#include "minecraft/modules/render/freelook/FreeLookLogic.h"

namespace nova::java {
void ConfigureFreeLook(const module::freelook::Settings& settings);
void UpdateFreeLook();
void StopFreeLook();
bool FreeLookAngles(float& yaw, float& pitch);
jboolean JNICALL FreeLookMouseHook(JNIEnv*, jclass, jobject);
jfloat JNICALL FreeLookYawHook(JNIEnv*, jclass, jobject);
jfloat JNICALL FreeLookPitchHook(JNIEnv*, jclass, jobject);
jfloat JNICALL FreeLookPreviousYawHook(JNIEnv*, jclass, jobject);
jfloat JNICALL FreeLookPreviousPitchHook(JNIEnv*, jclass, jobject);
void JNICALL FreeLookBillboardHook(JNIEnv*, jclass, jobject);
void JNICALL FreeLookParticlesHook(JNIEnv*, jclass, jboolean);
void JNICALL FreeLookTurnHook(JNIEnv*, jclass, jobject, jdouble, jdouble);
jfloat JNICALL FreeLookViewYawHook(JNIEnv*, jclass, jobject, jfloat);
jfloat JNICALL FreeLookViewPitchHook(JNIEnv*, jclass, jobject, jfloat);
}
