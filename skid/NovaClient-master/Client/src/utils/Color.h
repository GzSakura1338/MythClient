#pragma once

#include <cstdint>
#include <span>

namespace nova::color {

using Value = std::uint32_t;

constexpr Value ReAlpha(Value color, int alpha) {
    if (alpha < 0) alpha = 0;
    if (alpha > 255) alpha = 255;
    return (color & 0x00FFFFFFu) | (static_cast<Value>(alpha) << 24);
}

constexpr int Red(Value color) { return static_cast<int>(color & 0xFFu); }
constexpr int Green(Value color) { return static_cast<int>((color >> 8) & 0xFFu); }
constexpr int Blue(Value color) { return static_cast<int>((color >> 16) & 0xFFu); }
constexpr int Alpha(Value color) { return static_cast<int>((color >> 24) & 0xFFu); }

Value HsbToBgr(float hue, float saturation, float brightness);

void BgrToHsb(Value color, float out[3]);

constexpr float Clamp(float value, float low, float high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

constexpr float SmoothStep(float t) {
    const float c = Clamp(t, 0.0f, 1.0f);
    return c * c * (3.0f - 2.0f * c);
}

Value Interpolate(Value from, Value to, float ratio);

Value MultiGradient(int index, int speed, int range, std::span<const Value> colors);

}
