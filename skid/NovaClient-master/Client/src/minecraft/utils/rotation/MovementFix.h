#pragma once

namespace nova::rotation {

struct MoveInput {
    float forward = 0.0f;
    float strafe = 0.0f;
};

float Direction(float forward, float strafe, float playerYaw);

void FixMovement(MoveInput& input, float yaw, float playerYaw);

}
