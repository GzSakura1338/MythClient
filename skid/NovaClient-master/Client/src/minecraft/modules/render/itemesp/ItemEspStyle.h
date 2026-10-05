#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace nova::module::itemesp {
inline double GroupKey(double x, double y, double z, int kind) {
    if (x == 0) x = 1;
    if (y == 0) y = 1;
    if (z == 0) z = 1;
    constexpr double suffix[] = {.155, .255, .355, .455};
    // Java Math.round rounds negative ties toward positive infinity too.
    return std::floor((x + 1) * std::floor(y) * (z + 2) + .5) + suffix[kind];
}
inline double Radius(double distance) { return std::clamp(.01 * distance, .2, .4); }
inline double TextScale(double distance) { return std::clamp(.0015 * distance, .02266667, .07); }
constexpr std::int32_t boxColors[] = {-1, -331703, -10362113, -15216030};
constexpr std::int32_t textColors[] = {-1, -152, -7667713, -14614644};
}
