#pragma once

namespace nova::ui {
// Angle is clockwise in screen coordinates; zero points above the viewport.
// unit converts host GUI pixels to the current ImGui viewport units.
void DrawCompass(float radians, float unit = 1.f);
}
