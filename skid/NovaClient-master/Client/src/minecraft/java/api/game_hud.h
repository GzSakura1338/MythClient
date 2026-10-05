#pragma once
#include "base.h"

namespace nova::java {
struct HudInfo {
    std::string server = "--";
    int ping = -1;
};
HudInfo ReadHudInfo();
bool ReadHudServerTime(jobject packet, long long& ticks);
void ReleaseHudAccess(JNIEnv* env);
}
