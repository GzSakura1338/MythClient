#pragma once
#include "cs2/sdk/players/Players.h"
#include "ui/hud/esp2d/Esp2DView.h"
#include <optional>
#include <limits>

namespace nova::cs2::esp2d {
inline std::optional<ui::esp2d::Bounds> Project(const sdk::players::Player& player,
        const sdk::players::Matrix& matrix, float width, float height) {
    if (!(width > 1 && height > 1) || !std::isfinite(width) || !std::isfinite(height)) return {};
    struct Point { double x, y, w; } points[8];
    for (int i = 0; i < 8; ++i) {
        double world[]{player.origin.x + (i & 1 ? player.maxs.x : player.mins.x),
            player.origin.y + (i & 2 ? player.maxs.y : player.mins.y),
            player.origin.z + (i & 4 ? player.maxs.z : player.mins.z), 1};
        if (player.transformed) {
            world[0] = player.worldCorners[i].x; world[1] = player.worldCorners[i].y; world[2] = player.worldCorners[i].z;
        }
        const auto dot = [&](int row) { double value = 0; for (int c = 0; c < 4; ++c) value += matrix[row][c] * world[c]; return value; };
        points[i] = {dot(0), dot(1), dot(3)};
        if (!std::isfinite(points[i].x) || !std::isfinite(points[i].y) || !std::isfinite(points[i].w)) return {};
    }
    constexpr double nearPlane = .01;
    ui::esp2d::Bounds bounds{std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};
    bool visible = false;
    auto add = [&](Point point) {
        const double x = width * .5 * (1 + point.x / point.w), y = height * .5 * (1 - point.y / point.w);
        bounds.left = std::min(bounds.left, x); bounds.right = std::max(bounds.right, x);
        bounds.top = std::min(bounds.top, y); bounds.bottom = std::max(bounds.bottom, y); visible = true;
    };
    for (const auto& point : points) if (point.w >= nearPlane) add(point);
    // Clip the twelve box edges at the camera plane instead of dropping the
    // entire box when a single corner is behind the camera.
    for (int i = 0; i < 8; ++i) for (int bit : {1, 2, 4}) if (!(i & bit)) {
        const auto a = points[i], b = points[i | bit];
        if ((a.w >= nearPlane) == (b.w >= nearPlane)) continue;
        const double t = (nearPlane - a.w) / (b.w - a.w);
        add({a.x + t * (b.x - a.x), a.y + t * (b.y - a.y), nearPlane});
    }
    if (!visible || bounds.right <= 0 || bounds.bottom <= 0 || bounds.left >= width || bounds.top >= height) return {};
    // Preserve geometry at screen edges. Draw-list clipping hides off-screen
    // portions; clamping here shrinks the box and changes health-bar ratios.
    if (bounds.right - bounds.left < 1 || bounds.bottom - bounds.top < 1) return {};
    return bounds;
}
}
