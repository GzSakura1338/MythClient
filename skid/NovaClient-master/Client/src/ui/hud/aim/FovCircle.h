#pragma once
#include "imgui.h"
#include <cmath>
#include <numbers>

namespace nova::ui::aim {
// verticalScale = cot(vertical FOV / 2), supplied by the game's projection.
inline float Radius(float halfAngle, float verticalScale, float height) {
    if (!std::isfinite(halfAngle) || halfAngle <= 0 || halfAngle >= 89 ||
        !std::isfinite(verticalScale) || verticalScale <= 0 || !std::isfinite(height) || height <= 0) return 0;
    const float radius = height * .5f * verticalScale * std::tan(halfAngle * std::numbers::pi_v<float> / 180.f);
    return std::isfinite(radius) && radius < 100000 ? radius : 0;
}
inline void Draw(float radius) {
    if (!ImGui::GetCurrentContext() || !std::isfinite(radius) || radius <= 0) return;
    const auto display = ImGui::GetIO().DisplaySize;
    if (display.x <= 0 || display.y <= 0) return;
    auto* draw = ImGui::GetBackgroundDrawList();
    // One thin, unfilled white ring. Keep client-space geometry circular even
    // when the game renders to a stretched back buffer.
    const auto flags = draw->Flags;
    draw->Flags |= ImDrawListFlags_AntiAliasedLines;
    draw->AddCircle({display.x * .5f, display.y * .5f}, radius, IM_COL32(255, 255, 255, 210), 0, 1.f);
    draw->Flags = flags;
}
}
