#pragma once
#include "minecraft/java/api/base.h"
#include "ui/backend/VulkanOverlay.h"

namespace nova::compat::mc263 {
// Names are emitted from the single version catalog; handles are owned by the runtime.
namespace names {
struct Member { const char* owner; const char* name; const char* descriptor; };
#define NOVA_CLASS(id, name) inline constexpr const char* id = name;
#define NOVA_FIELD(id, owner, name, desc) inline constexpr Member id{owner, name, desc};
#define NOVA_STATIC_FIELD NOVA_FIELD
#define NOVA_METHOD NOVA_FIELD
#define NOVA_STATIC_METHOD NOVA_FIELD
#include "minecraft/compat/mappings/minecraft263.inc"
#undef NOVA_CLASS
#undef NOVA_FIELD
#undef NOVA_STATIC_FIELD
#undef NOVA_METHOD
#undef NOVA_STATIC_METHOD
}
bool Initialize(JNIEnv* env);
void* WindowHandle();
bool IsVulkan();
bool VulkanHandles(vkoverlay::Handles* out);
void Shutdown(JNIEnv* env);
const char* Error();
jobject Screen(JNIEnv* env, jobject minecraft);
bool CaptureMouse(bool capture);
bool SetSprintKeyDown(bool down);
bool GetGamma(double* out);
bool SetGamma(double value);
}
