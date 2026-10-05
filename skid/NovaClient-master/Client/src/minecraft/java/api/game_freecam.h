#pragma once
#include "minecraft/java/api/base.h"
namespace nova::java {
struct FreeCamSettings {
    bool enabled=false;
    double speed=2.5;
    bool disableOnDamage=true,showArm=false,allowDigging=false,allowInteracting=false,allowPlacing=false;
};
void ConfigureFreeCam(const FreeCamSettings& settings);
bool UpdateFreeCam();
void RenderFreeCam();
void StopFreeCam();
bool FreeCamActive();
bool FreeCamCamera(void* entity);
bool FreeCamTurn(double dx,double dy);
enum class FreeCamAction { Dig, Interact, Place };
bool FreeCamBlocks(FreeCamAction action);
bool FreeCamBlocksMouse(bool right);
bool FreeCamBlocksPacket(JNIEnv* env,jobject packet);
bool CaptureCameraView(int& view);
bool SetCameraView(int view);
struct FreeCamKeys {float forward=0,strafe=0;bool up=false,down=false;};
bool ReadFreeCamKeys(FreeCamKeys& out);
void SuppressFreeCamKeys();
jboolean JNICALL FreeCamAttackHook(JNIEnv*,jclass,jobject client);
void JNICALL FreeCamClickHook(JNIEnv*,jclass,jobject client);
jboolean JNICALL FreeCamControlledHook(JNIEnv*,jclass,jobject player);
void JNICALL FreeCamAnglesHook(JNIEnv*,jclass,jobject player,jfloat dx,jfloat dy);
}
