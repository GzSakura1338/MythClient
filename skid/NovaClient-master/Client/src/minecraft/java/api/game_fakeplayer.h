#pragma once
#include "minecraft/java/api/base.h"
namespace nova::java {
struct FakePlayerSettings {bool enabled=false,copyEquipment=true,damage=true,autoReset=true;float health=20;};
enum class FakePlayerStatus { Off,Waiting,Active,Failed,Finished };
bool FakePlayerSupported();
void ConfigureFakePlayer(const FakePlayerSettings& settings);
FakePlayerStatus GetFakePlayerStatus();
// Runs on the client game tick, including circuit-open cleanup.
void TickFakePlayer();
bool IsFakePlayer(void* entity);
// True means the local attack has been handled; do not send it to the server.
bool AttackFakePlayer(JNIEnv* env,jobject target);
bool BlocksFakePlayerPacket(JNIEnv* env,jobject packet);
void ObserveFakePlayerExplosion(JNIEnv* env,jobject packet);
bool WaitFakePlayerStopped(unsigned milliseconds);
}
