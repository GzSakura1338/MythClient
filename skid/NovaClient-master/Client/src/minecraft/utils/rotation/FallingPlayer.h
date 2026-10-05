#pragma once

namespace nova::rotation {

// A ballistics replay of the player's own motion, ported from Starain's
// FallingPlayer. Scaffold asks it where the player will be a few ticks from now
// so it can tell whether the block it picked is still reachable on the way down.
//
// Only the motion integration is here: the collision search the original also
// carries is not reached by anything on this side.
struct FallingPlayer {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double motionX = 0.0;
    double motionY = 0.0;
    double motionZ = 0.0;
    float yaw = 0.0f;
    float strafe = 0.0f;
    float forward = 0.0f;
    float jumpMovementFactor = 0.0f;

    void CalculateForTick();
    void Calculate(int ticks);
};

// Seeds from the local player's position, delta movement, yaw and movement
// speed. False when the player cannot be read.
bool MakeFallingPlayer(FallingPlayer& out);

}
