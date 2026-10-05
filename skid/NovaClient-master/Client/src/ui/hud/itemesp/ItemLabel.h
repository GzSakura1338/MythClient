#pragma once
#include <cstdint>
#include "imgui.h"

namespace nova::ui {
// Position and font size are projected by the host for this frame.
void DrawItemLabel(ImVec2 anchor, float fontSize, double worldTextScale,
    std::int64_t count, ImU32 color, bool shadow);
}
