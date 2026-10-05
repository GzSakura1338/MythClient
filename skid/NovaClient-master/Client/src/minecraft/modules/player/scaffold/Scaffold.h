#pragma once
#include "minecraft/java/api/base.h"

namespace nova::module::scaffold {

// While the scaffold is placing, its rotations and its own attack timing are the
// only ones that should be running: KillAura stands down completely and Velocity
// stops aiming on the client's behalf. Both ask here, mirroring Starain, where the
// same three call sites test Scaffold.isEnabled().
bool IsEnabled();
// Observations after legacy dispatch, including delayed replay. Never send or
// modify packets here; these callbacks only maintain placement/rotation history.
void ObserveLegacyScaffoldPacket(bool movement, bool hasLook, float yaw, float pitch);
jfloat JNICALL KeepFovFloatHook(JNIEnv*, jclass, jfloat original, jboolean world);
jdouble JNICALL KeepFovDoubleHook(JNIEnv*, jclass, jdouble original, jboolean world);
jfloat JNICALL KeepCameraFovHook(JNIEnv*, jclass, jfloat original);

}
