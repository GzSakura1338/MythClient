#pragma once
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_entity.h"

namespace nova::java {
struct BridgePlayer {
    bool flying = false, manualSneak = false;
    float forward = 0;
    double reach = 4.5;
};
bool ReadBridgePlayer(BridgePlayer& out);
// Predict one Raven simulation tick with sneak disabled, then query its 0.01-block foot slice.
bool BridgeEdgeOffset(float forward, float strafe, bool jump, double& out);
void ReleaseBridgeAssistAccess(JNIEnv* env);
}
