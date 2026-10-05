#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

#include "core/Instance.h"

namespace nova::rotation::mth {

inline constexpr int kSinTableSize = 65536;
inline constexpr float kSinScale = 10430.378f;

struct SinTableStorage {
    SinTableStorage() {
        for (int i = 0; i < kSinTableSize; ++i)
            values[i] = static_cast<float>(std::sin(static_cast<double>(i) * 3.141592653589793 * 2.0 / 65536.0));
    }

    float values[kSinTableSize];
};

inline const float* SinTable() {
    return Instance<SinTableStorage>().values;
}

inline float Sin(float value) {
    return SinTable()[static_cast<int>(value * kSinScale) & 65535];
}

inline float Cos(float value) {
    return SinTable()[static_cast<int>(value * kSinScale + 16384.0f) & 65535];
}

inline float WrapDegrees(float angle) {
    float result = std::fmod(angle, 360.0f);
    if (result >= 180.0f)
        result -= 360.0f;
    if (result < -180.0f)
        result += 360.0f;
    return result;
}

inline double WrapDegrees(double angle) {
    double result = std::fmod(angle, 360.0);
    if (result >= 180.0)
        result -= 360.0;
    if (result < -180.0)
        result += 360.0;
    return result;
}

template <typename T>
inline T Clamp(T value, T min, T max) {
    return value < min ? min : (value > max ? max : value);
}

inline double JavaMin(double a, double b) {
    if (a != a || b != b)
        return std::numeric_limits<double>::quiet_NaN();
    return a <= b ? a : b;
}

inline double JavaMax(double a, double b) {
    if (a != a || b != b)
        return std::numeric_limits<double>::quiet_NaN();
    if (a == 0.0 && b == 0.0 && std::signbit(a))
        return b;
    return a >= b ? a : b;
}

inline double JavaRound(double value) {
    if (value != value)
        return 0.0;
    return static_cast<double>(static_cast<long long>(std::floor(value + 0.5)));
}

}
