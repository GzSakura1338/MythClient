#pragma once
#include "minecraft/java/api/game_entity.h"
namespace nova::java {
struct PearlPhaseState {
    rotation::Vec3 position;
    float yaw = 0, forward = 0, strafe = 0;
    bool ground = false, horizontalCollision = false, ladder = false;
    bool blocked = false, risingLowPose = false;
};
bool ReadPearlPhaseState(PearlPhaseState& out);
}
