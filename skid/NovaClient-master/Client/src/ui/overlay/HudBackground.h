#pragma once
#include "imgui.h"

namespace nova::ui::hud {
// Master alpha controls tint, including zero. Visibility fades the whole glass
// layer separately, keeping blur coverage independent of background color.
ImU32 BackgroundColor(ImU32 color);
void Background(ImDrawList* draws, ImVec2 min, ImVec2 max, ImU32 color,
                float rounding = 0.0f, ImDrawFlags flags = 0, float visibility = 1.0f);
}
