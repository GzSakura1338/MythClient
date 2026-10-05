#pragma once
#include "minecraft/java/api/base.h"
namespace nova::module::displace {
void BeginBlink();
void EndBlink(bool send = true);
bool HoldOutput(JNIEnv*, jobject manager, jobject packet, jobject callbacks);
}
