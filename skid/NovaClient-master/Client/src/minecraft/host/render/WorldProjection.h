#pragma once

#include "imgui.h"
#include "minecraft/java/api/game_chest.h"
#include "minecraft/utils/rotation/Vec3.h"

// --- world to screen ---------------------------------------------------------
//
// World overlays project with the origin and complete matrices captured during
// the world pass. The yaw/pitch and bob path remains available for explicitly
// constructed views, but must not reapply effects already in captured matrices.
//
// A point at or behind the near plane cannot be projected at all, so Project says so
// rather than returning a number: the divide by a clip w at or under zero is not a
// screen position, and treating it as one puts the drawing on the far side of the
// screen.

namespace nova::minecraft::projection {

struct ScreenPoint {
    float x = 0.0f;
    float y = 0.0f;
};

[[nodiscard]] double Dot(const rotation::Vec3& a, const rotation::Vec3& b);

[[nodiscard]] rotation::Vec3 Cross(const rotation::Vec3& a, const rotation::Vec3& b);

// View bobbing, as GameRenderer.bobView builds it: a translate, a roll about Z and
// a nudge about X, applied in view space and driven by how far and how fast the
// player has walked. It is the one thing the camera's own angles do not carry.
struct BobTransform {
    double tx = 0.0;
    double ty = 0.0;
    double cosZ = 1.0, sinZ = 0.0;
    double cosX = 1.0, sinX = 0.0;
    bool   active = false;
};

// The camera's basis, built the way the game builds it: look comes straight from
// yaw and pitch in Minecraft's own convention (yaw 0 faces +Z, +90 faces -X, and
// pitch is downward-positive).
//
// Right is taken analytically rather than as look crossed with world up: that
// cross product collapses to zero when the view is straight up or down, and the
// basis has to be exact at every angle because everything drawn is placed by it.
struct CameraBasis {
    rotation::Vec3 look;
    rotation::Vec3 right;
    rotation::Vec3 up;
    rotation::Vec3 eye;
    float projection[16] = {};
    float modelView[16] = {};
    bool hasModelView = false;
    int   windowWidth = 0;
    int   windowHeight = 0;
    BobTransform bob;
};

[[nodiscard]] BobTransform MakeBob(const java::CameraView& view, float partialTick);

[[nodiscard]] CameraBasis MakeBasis(const java::CameraView& view, float partialTick);

// World point to screen point, through the frame's own projection matrix, so the
// field of view, the aspect and any zoom are whatever the frame was drawn with.
// False when the point is at or behind the near plane.
bool Project(const CameraBasis& basis, const rotation::Vec3& world, const ImVec2& display,
             ScreenPoint& out);

}
