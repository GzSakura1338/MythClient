#pragma once

#include <array>
#include <cmath>
#include <vector>
#include "minecraft/utils/rotation/Vec3.h"

namespace nova::minecraft::projection {

// Project's visibility predicate includes the exact camera transform (including
// bob/model-view). Find the near-plane intersection without duplicating it here.
template <typename Project, typename Point>
Point ClipBoundary(rotation::Vec3 visible, rotation::Vec3 hidden,
                   Point point, Project&& project) {
    for (int i = 0; i < 32; ++i) {
        const rotation::Vec3 middle{visible.x + (hidden.x - visible.x) * 0.5,
                                    visible.y + (hidden.y - visible.y) * 0.5,
                                    visible.z + (hidden.z - visible.z) * 0.5};
        Point candidate;
        if (project(middle, candidate)) { visible = middle; point = candidate; }
        else hidden = middle;
    }
    return point;
}

template <typename Point, typename Project, typename Edge>
void DrawClippedLine(const rotation::Vec3& a, const rotation::Vec3& b, Project&& project, Edge&& edge) {
    for (const auto& v : {a, b})
        if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z)) return;
    Point start{}, end{};
    const bool av = project(a, start), bv = project(b, end);
    if (!av && !bv) return;
    if (!av) start = ClipBoundary(b, a, end, project);
    if (!bv) end = ClipBoundary(a, b, start, project);
    edge(start, end);
}

template <typename Point, typename Project, typename Face, typename Edge>
void DrawClippedBox(const std::array<rotation::Vec3, 8>& corners,
                    Project&& project, Face&& face, Edge&& edge) {
    for (const auto& v : corners)
        if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z)) return;
    std::array<Point, 8> points{};
    std::array<bool, 8> visible{};
    for (int i = 0; i < 8; ++i) visible[i] = project(corners[i], points[i]);
    constexpr int edges[][2]{{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
    std::array<Point, 12> intersections{};
    for (int i = 0; i < 12; ++i) {
        const int a = edges[i][0], b = edges[i][1];
        if (visible[a] != visible[b]) {
            const int front = visible[a] ? a : b, back = visible[a] ? b : a;
            intersections[i] = ClipBoundary(corners[front], corners[back], points[front], project);
        }
    }
    constexpr int faces[][4]{{0,1,2,3},{4,5,6,7},{0,1,5,4},{1,2,6,5},{2,3,7,6},{3,0,4,7}};
    for (const auto& indices : faces) {
        std::vector<Point> polygon;
        polygon.reserve(5);
        for (int j = 0; j < 4; ++j) {
            const int a = indices[j], b = indices[(j + 1) % 4];
            if (visible[a]) polygon.push_back(points[a]);
            if (visible[a] != visible[b]) {
                for (int k = 0; k < 12; ++k) {
                    if ((edges[k][0] == a && edges[k][1] == b) ||
                        (edges[k][0] == b && edges[k][1] == a)) {
                        polygon.push_back(intersections[k]);
                        break;
                    }
                }
            }
        }
        if (polygon.size() >= 3) face(polygon);
    }
    for (int i = 0; i < 12; ++i) {
        const int a = edges[i][0], b = edges[i][1];
        if (!visible[a] && !visible[b]) continue;
        edge(visible[a] ? points[a] : intersections[i],
             visible[b] ? points[b] : intersections[i]);
    }
}

}
