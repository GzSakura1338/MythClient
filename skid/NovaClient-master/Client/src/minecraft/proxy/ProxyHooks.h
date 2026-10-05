#pragma once
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/hook_bridge.h"
#include "minecraft/hooks/Edits.h"
namespace nova::minecraft::proxyadapter {
enum class Version {Lunar189,Lunar1710,Lunar12111,Forge1201,EaseCation,NeoForge12110,Minecraft263,Unsupported};
const hook::ClassEdits* ExtendEdits(const hook::ClassEdits*,int& count,Version);
const bridge::HookDef* ExtendHooks(const bridge::HookDef*,int& count);
void Initialize();
void Shutdown(JNIEnv*);
}
