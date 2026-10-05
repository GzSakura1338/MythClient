#pragma once
#include "minecraft/java/api/game_entity.h"
namespace nova::java {
struct ElytraFlight {
    bool gliding = false, boosting = false;
    rotation::Vec3 velocity, look;
};
// Read only; false means unknown, never permission to launch another rocket.
bool ReadElytraFlight(ElytraFlight& out);
bool FireworkFlightDuration(int hotbarSlot, int& duration);
}
