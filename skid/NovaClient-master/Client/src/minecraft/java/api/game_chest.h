#pragma once

#include <vector>

#include "minecraft/java/api/base.h"

namespace nova::java {

// --- chest esp ---------------------------------------------------------------
//
// Where the chests are, and where the camera is looking from.
//
// The chests come from the chunks the player has loaded, which is a walk over
// every loaded chunk's block entity map -- the same list Naven builds each tick.
// The camera read returns the world pass snapshot used by screen-space overlays.

struct ChestBox {
    double minX = 0.0;
    double minY = 0.0;
    double minZ = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
    double maxZ = 0.0;

    // The block the box was built from, so the caller can ask whether it has been
    // seen opened.
    int x = 0;
    int y = 0;
    int z = 0;
};

// Every chest in the loaded chunks, as a world-space box. A double chest reports
// one box spanning both halves; the left half on its own is skipped, exactly as
// the original does. False when the level cannot be read, in which case `out` is
// emptied.
bool LoadedChestBoxes(std::vector<ChestBox>& out);

struct CameraView {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    float  yaw = 0.0f;
    float  pitch = 0.0f;
    double fov = 70.0;

    // The game's own projection matrix for the frame, column-major as JOML's
    // Matrix4f stores it, so entry [row][col] is projection[col * 4 + row]. Taken
    // from the world render pass, including its FOV, zoom and camera effects.
    float projection[16] = {};
    float modelView[16] = {};
    bool hasModelView = false;

    // The window the frame was drawn into. The overlay draws in its own display
    // space, so it needs both to map a projected point correctly.
    int windowWidth = 0;
    int windowHeight = 0;

    // Whether the world pass is applying view bobbing, and the player walk state it
    // is driven by. Bobbing is a transform the camera angles do not carry, so a
    // projection built only from them is short by it whenever the player is
    // walking -- which is a vertical slide, the axis bobbing mostly moves on.
    bool  bobView = false;
    float walkDist = 0.0f;
    float walkDistO = 0.0f;
    float bob = 0.0f;
    float oBob = 0.0f;
};

// The camera position, its yaw and pitch, and the field of view the frame was
// rendered with. False when there is no camera to read.
bool ReadCameraView(float partialTick, CameraView& out);

// Value-only snapshot captured at the world pass, before hand/GUI rendering can
// replace its matrices. Invalidated at frame start, including menu-only frames.
void InvalidateWorldCamera();
void PublishWorldCamera(const CameraView& view);
bool ReadWorldCamera(CameraView& out);
void JNICALL ResetWorldCameraHook(JNIEnv*, jclass);
void JNICALL ForgeWorldCameraHook(JNIEnv*, jclass, jobject camera, jobject poseStack, jobject projection);

// ClientboundBlockEventPacket, when it is the "a chest was opened" event: the
// block is a chest or a trapped chest and its two bytes are 1 and 1. Fills in the
// position it names.
bool ReadChestOpenEvent(jobject packet, int& x, int& y, int& z);

}
