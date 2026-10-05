#pragma once
#include "imgui.h"
namespace nova::menu::panel { class PanelClickGui; }
namespace nova::ui::browser {
void Draw(menu::panel::PanelClickGui& gui,ImVec2 origin);
void DrawView(menu::panel::PanelClickGui& gui,ImVec2 position,ImVec2 size,bool detached);
// Cached page pixels with a compact frame for shelf/transition previews.
void DrawPreview(ImDrawList*,ImVec2 position,ImVec2 size);
void Hide();
void ReleaseTextures(); // after renderer shutdown, before ImGui context destroy
}
