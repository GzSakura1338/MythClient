#pragma once

#include "minecraft/java/api/base.h"
#include "../../../../shared/Bootstrap.h"

#include <cstdint>

namespace nova::compat {

enum class RenderBackend { Glfw, Lwjgl2, Sdl };
enum class Capability : std::uint32_t {
    Forge1201Events = 1u << 0,
    Settings = 1u << 1,
    PlayerEffects = 1u << 2,
    ClickInput = 1u << 3,
    RotationEvents = 1u << 4,
    NetworkEvents = 1u << 5,
    BasicPlace = 1u << 6
};

struct GameMapping {
    const char* instance;
    const char* player;
    const char* playerType;
    const char* world;
    const char* worldType;
    const char* screen;
    const char* screenType;
};

struct Profile {
    unsigned id;
    const char* key;
    const char* label;
    RenderBackend renderer;
    const GameMapping* mapping;
    std::uint32_t capabilities;
};

const Profile& Current();
bool Has(Capability capability);
bool Select(unsigned id);
bool IsLunar189();
bool IsLunar1710();
bool IsLegacyLunar();
bool UsesModernRuntime();
bool IsMinecraft263();
bool IsEaseCation();
bool IsNeoForge12110();
bool IsLunar12111();
// Modern Minecraft before the 26.3 SDL/input and rendering ABI changes.
bool UsesGlfwModernRuntime();
const char* InitializationError();
bool Initialize(JNIEnv* env);
void Shutdown(JNIEnv* env);
jobject ClassLoader(JNIEnv* env);
jobject Minecraft(JNIEnv* env);
jobject LocalPlayer(JNIEnv* env);
bool ScreenOpen();
bool ChatScreenOpen();
bool CaptureMouse(bool capture);
void RestoreMouse();
bool ValidatePlayer();
// Read-only startup probe, used before any event or rendering hooks are installed.
bool RemoteWorldReady(JNIEnv* env);

}
