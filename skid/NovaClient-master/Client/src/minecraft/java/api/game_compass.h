#pragma once

#include "minecraft/java/api/base.h"

namespace nova::java {

// --- Compass -----------------------------------------------------------------
//
// The three world reads a compass needs that nothing else in the client does: the
// player's previous-tick position and yaw, which the frame interpolates between,
// the world spawn, and the window's GUI scale.

struct CompassView {
    double x = 0.0;     // player X, interpolated to the frame
    double z = 0.0;     // player Z, interpolated to the frame
    float  yaw = 0.0f;  // player yaw, interpolated to the frame
};

// Player position and yaw, lerped from the previous tick to this one by
// `partialTick`. False when the client, level, or player is not there, in which
// case there is nothing to point at.
bool ReadCompassView(float partialTick, CompassView& out);

// Level.getSharedSpawnPos, but only for a natural dimension -- the original's
// getSpawnPosition returns null everywhere else, which is what stops a compass
// pointing anywhere in the Nether or the End.
bool WorldSpawn(int& x, int& z);

// Minecraft's window size in screen pixels and its effective GUI scale. The
// overlay needs both to turn the original's GUI-scaled offsets into draw list
// pixels, because those two spaces are not the same one.
bool WindowMetrics(int& width, int& height, double& guiScale);

}
