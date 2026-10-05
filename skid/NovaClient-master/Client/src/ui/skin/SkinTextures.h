#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "imgui.h"

namespace nova::ui::skins {
std::uint64_t Find(const std::string& key);
std::uint64_t Store(const std::string& key,int width,int height,std::vector<std::uint32_t> pixels);
// Resolve only on the ImGui render thread. Uploaded by the active GL/Vulkan backend.
ImTextureRef Resolve(std::uint64_t handle);
// Called after the renderer has destroyed its GPU resources, before the context dies.
void Release();
}
