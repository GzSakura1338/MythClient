#pragma once
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_entity.h"

namespace nova::java {
struct BacktrackPacket {
    bool flush = false;
    bool resetWorld = false;
    bool deadHealth = false;
    bool movedTarget = false;
    rotation::Vec3 position;
};
bool ReadBacktrackPacket(jobject packet, int targetId, int selfId, const rotation::Vec3& base, BacktrackPacket& out);
bool BacktrackBase(const EntityHandle& target, rotation::Vec3& out);
void ReleaseBacktrackAccess(JNIEnv* env);
// The same head-of-controller-attack event on all three supported profiles.
void JNICALL BacktrackAttackHook(JNIEnv*, jclass, jobject target);
}
