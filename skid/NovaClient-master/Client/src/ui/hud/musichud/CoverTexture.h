#pragma once
#include "imgui.h"
#include "smtc/MediaSnapshot.h"
namespace nova::ui::music {
// All entry points are render-thread only.
void RetainCovers(std::uint64_t current, std::uint64_t previous);
bool DrawCover(ImDrawList* draws, const std::shared_ptr<const smtc::Artwork>& artwork,
    ImVec2 min, ImVec2 max, float rounding, float opacity = 1.0f);
// Called after the renderer shutdown, before the ImGui context is destroyed.
void ReleaseCovers();
}
