#pragma once
#include "imgui.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace nova::ui::targetesp {
inline constexpr int Segments = 64;
struct Point { double x = 0, y = 0, z = 0; };
struct Ring {
    std::array<Point, Segments + 1> top{}, bottom{};
    float pulse = 0;
};

// Starain 1.20.1 TargetESP's clock, radius, oscillation, skirt and alpha formula.
// No game state or JNI dependencies: the host supplies position and projection.
inline Ring Animate(std::int64_t milliseconds, double radius, float alphaFactor) {
    Ring ring;
    if (!std::isfinite(radius) || !std::isfinite(alphaFactor)) return ring;
    const float ticks = static_cast<float>(milliseconds % 1000000) * .004f;
    // Mth.sin uses a 65536-entry table; compute its selected entry directly.
    const int sineIndex = static_cast<int>((ticks * 1.8f) * 10430.378f) & 65535;
    const float sine = static_cast<float>(std::sin(sineIndex * 6.283185307179586 / 65536.));
    ring.pulse = .35f + .65f * ((sine + 1.f) * .5f) * alphaFactor;
    const double y = std::sin(static_cast<double>(ticks)) + 1.;
    const float skirt = static_cast<float>(-std::sin(static_cast<double>(ticks + 1.f)) / 2.7f);
    for (int i = 0; i <= Segments; ++i) {
        const float angle = static_cast<float>(6.283185307179586 * i / Segments);
        const float x = static_cast<float>(radius * std::cos(static_cast<double>(angle)));
        const float z = static_cast<float>(radius * std::sin(static_cast<double>(angle)));
        ring.top[i] = {x, y, z};
        ring.bottom[i] = {x, y + skirt, z};
    }
    // Avoid a subpixel gap at the float-rounded 2pi seam.
    ring.top.back() = ring.top.front();
    ring.bottom.back() = ring.bottom.front();
    return ring;
}

struct Vertex { ImVec2 position; ImVec4 color; };
struct Mesh {
    std::vector<std::array<Vertex, 3>> triangles;
    std::vector<std::array<Vertex, 2>> lines;
};
// Clips to the viewport while interpolating color/alpha, then emits ImGui
// triangles. The host has already clipped against the camera's near plane.
void Draw(const Mesh& mesh, ImDrawList* draw, ImVec2 display);
}
