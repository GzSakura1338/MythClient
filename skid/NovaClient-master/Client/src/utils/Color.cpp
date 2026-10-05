#include "utils/Color.h"

#include <chrono>
#include <cmath>

namespace nova::color {
namespace {

std::int64_t Millis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

}

Value HsbToBgr(float hue, float saturation, float brightness) {
    int r = 0, g = 0, b = 0;
    if (saturation == 0.0f) {
        r = g = b = static_cast<int>(brightness * 255.0f + 0.5f);
    } else {
        const float h = (hue - std::floor(hue)) * 6.0f;
        const float f = h - std::floor(h);
        const float p = brightness * (1.0f - saturation);
        const float q = brightness * (1.0f - saturation * f);
        const float t = brightness * (1.0f - (saturation * (1.0f - f)));
        switch (static_cast<int>(h)) {
            case 0: r = static_cast<int>(brightness * 255.0f + 0.5f); g = static_cast<int>(t * 255.0f + 0.5f); b = static_cast<int>(p * 255.0f + 0.5f); break;
            case 1: r = static_cast<int>(q * 255.0f + 0.5f); g = static_cast<int>(brightness * 255.0f + 0.5f); b = static_cast<int>(p * 255.0f + 0.5f); break;
            case 2: r = static_cast<int>(p * 255.0f + 0.5f); g = static_cast<int>(brightness * 255.0f + 0.5f); b = static_cast<int>(t * 255.0f + 0.5f); break;
            case 3: r = static_cast<int>(p * 255.0f + 0.5f); g = static_cast<int>(q * 255.0f + 0.5f); b = static_cast<int>(brightness * 255.0f + 0.5f); break;
            case 4: r = static_cast<int>(t * 255.0f + 0.5f); g = static_cast<int>(p * 255.0f + 0.5f); b = static_cast<int>(brightness * 255.0f + 0.5f); break;
            case 5: r = static_cast<int>(brightness * 255.0f + 0.5f); g = static_cast<int>(p * 255.0f + 0.5f); b = static_cast<int>(q * 255.0f + 0.5f); break;
            default: break;
        }
    }
    return 0xFF000000u | (static_cast<Value>(b) << 16) | (static_cast<Value>(g) << 8) |
           static_cast<Value>(r);
}

void BgrToHsb(Value color, float out[3]) {
    const float r = static_cast<float>(Red(color)) / 255.0f;
    const float g = static_cast<float>(Green(color)) / 255.0f;
    const float b = static_cast<float>(Blue(color)) / 255.0f;

    const float max = (r > g ? r : g) > b ? (r > g ? r : g) : b;
    const float min = (r < g ? r : g) < b ? (r < g ? r : g) : b;

    out[2] = max;
    out[1] = max != 0.0f ? (max - min) / max : 0.0f;
    if (out[1] == 0.0f) {
        out[0] = 0.0f;
        return;
    }
    const float delta = max - min;
    float hue;
    if (r == max)      hue = (g - b) / delta;
    else if (g == max) hue = 2.0f + (b - r) / delta;
    else               hue = 4.0f + (r - g) / delta;
    hue *= 60.0f;
    if (hue < 0.0f) hue += 360.0f;
    out[0] = hue / 360.0f;
}

Value Interpolate(Value from, Value to, float ratio) {
    const auto channel = [ratio](int a, int b) {
        return static_cast<Value>(static_cast<int>(a + (b - a) * ratio));
    };
    return (channel(Alpha(from), Alpha(to)) << 24) |
           (channel(Blue(from), Blue(to)) << 16) |
           (channel(Green(from), Green(to)) << 8) |
           channel(Red(from), Red(to));
}

Value MultiGradient(int index, int speed, int range, std::span<const Value> colors) {
    if (colors.empty()) return 0;
    if (colors.size() == 1) return colors[0];

    if (range < 1) range = 1;
    if (speed < 1) speed = 1;
    if (speed > range) speed = range;

    const std::int64_t period = static_cast<std::int64_t>(speed) * 16 * range;
    const float timeProgress =
        static_cast<float>(Millis() % period) / static_cast<float>(period);

    const float progress = std::fmod(timeProgress + static_cast<float>(index) / static_cast<float>(range), 1.0f);

    const float scaled = progress * static_cast<float>(colors.size());
    const auto segment = static_cast<std::size_t>(std::floor(scaled)) % colors.size();
    const std::size_t next = (segment + 1) % colors.size();

    return Interpolate(colors[segment], colors[next], SmoothStep(scaled - std::floor(scaled)));
}

}
