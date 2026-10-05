#pragma once

#include <cmath>

namespace nova::rotation {

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    [[nodiscard]] Vec3 Add(double dx, double dy, double dz) const {
        return Vec3{x + dx, y + dy, z + dz};
    }

    [[nodiscard]] Vec3 Subtract(const Vec3& other) const {
        return Vec3{x - other.x, y - other.y, z - other.z};
    }

    [[nodiscard]] Vec3 Scale(double factor) const {
        return Vec3{x * factor, y * factor, z * factor};
    }

    [[nodiscard]] double DistanceToSqr(const Vec3& other) const {
        const double dx = x - other.x;
        const double dy = y - other.y;
        const double dz = z - other.z;
        return dx * dx + dy * dy + dz * dz;
    }

    [[nodiscard]] double DistanceTo(const Vec3& other) const {
        return std::sqrt(DistanceToSqr(other));
    }

    [[nodiscard]] double DistanceToSqr(double ox, double oy, double oz) const {
        const double dx = x - ox;
        const double dy = y - oy;
        const double dz = z - oz;
        return dx * dx + dy * dy + dz * dz;
    }
};

}
