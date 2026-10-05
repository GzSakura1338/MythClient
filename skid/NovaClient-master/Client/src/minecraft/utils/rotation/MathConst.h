#pragma once

namespace nova::rotation {

inline constexpr double kPi = 3.14159265358979323846;
inline constexpr float kPiF = static_cast<float>(kPi);
inline constexpr float kToRadians = kPiF / 180.0f;
inline constexpr float kToDegrees = static_cast<float>(180.0 / kPi);

inline float ToRadians(float degrees) {
    return static_cast<float>(static_cast<double>(degrees) / 180.0 * kPi);
}

inline double ToDegrees(double radians) {
    return radians / kPi * 180.0;
}

}
