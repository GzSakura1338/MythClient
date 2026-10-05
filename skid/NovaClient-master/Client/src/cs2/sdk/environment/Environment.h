#pragma once
#include "cs2/sdk/players/Players.h"
#include <algorithm>
#include <cmath>

namespace nova::cs2::sdk::environment {
// Local post-processing exposure, not a server clock or a replacement skybox.
enum class Period { Day, Night };
struct Settings { Period period = Period::Day; float day = 1.25f, night = .20f; };
inline float Scale(Settings value) {
    const bool night = value.period == Period::Night;
    const float scale = night ? value.night : value.day;
    return std::isfinite(scale) ? std::clamp(scale, night ? .05f : 1.f, night ? 1.f : 3.f) : (night ? .20f : 1.25f);
}
inline float Adjust(float native, float scale) {
    if (!std::isfinite(native) || native <= 0 || !std::isfinite(scale) || scale <= 0) return native;
    const float result = native * scale;
    return std::isfinite(result) && result > 0 ? result : native;
}
struct Binding {
    void* target = nullptr;
    void* sourceReturn = nullptr;
    bool (*inGame)() = nullptr;
};
bool InGame(const players::Memory&, std::uintptr_t client, std::uintptr_t engine);
Binding ResolveBinding();
}
