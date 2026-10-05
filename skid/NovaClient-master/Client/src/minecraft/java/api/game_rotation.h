#pragma once

namespace nova::java {

struct PosRot {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool onGround = false;
    bool horizontalCollision = false;
};

bool PlayerRotation(float& yaw, float& pitch);

bool PlayerPreviousRotation(float& yaw, float& pitch);

bool SetPlayerRotation(float yaw, float pitch);
// Camera-only write: also shift previous angles used for render interpolation.
bool SetPlayerCameraRotation(float yaw, float pitch, double smoothAmount);

bool PlayerEyePosition(double& x, double& y, double& z);

bool PlayerSensitivity(double& out);

int ClientFps();

bool RenderDistance(int& out);

bool Fov(int& out);

void PickCursor(float partialTicks);

bool IsPosRot(void* packet);

bool ReadPosRot(void* packet, PosRot& out);

void* MakePosRot(const PosRot& value);

}
