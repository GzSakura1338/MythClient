#pragma once
#include <algorithm>
#include <cmath>
#include <numbers>

namespace nova::cs2::fov {
struct Settings { float degrees = 90; bool preserveZoom = true; bool removeShake = true; };
inline Settings Normalize(Settings settings) {
    settings.degrees = std::isfinite(settings.degrees) ? std::clamp(std::round(settings.degrees * 10) / 10, 30.f, 150.f) : 90.f;
    return settings;
}
inline float Adjust(float original, float base, bool scoped, Settings settings) {
    // Invalid native camera data must pass through, including NaN/Inf.
    if (!std::isfinite(original) || original <= 0 || original >= 180 ||
        !std::isfinite(base) || base <= 0 || base >= 180) return original;
    settings = Normalize(settings);
    // Weapon changes and lingering native interpolation are not proof of scope.
    // Always restore the configured world FOV as soon as the Pawn is unscoped.
    if (!scoped || !settings.preserveZoom) return settings.degrees;
    if (settings.degrees == base) return original;
    // Preserve optical magnification and the scoped zoom transition. Scaling
    // degrees directly distorts the magnification at wider fields of view.
    constexpr double halfRadians = std::numbers::pi / 360.;
    const double tangent = std::tan(settings.degrees * halfRadians) *
        std::tan(original * halfRadians) / std::tan(base * halfRadians);
    return static_cast<float>(std::clamp(std::atan(tangent) / halfRadians, 1., 179.));
}
}
