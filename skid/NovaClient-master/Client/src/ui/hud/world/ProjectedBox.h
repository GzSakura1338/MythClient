#pragma once
#include <array>
#include "imgui.h"

namespace nova::ui {
// Eight screen-space corners, indexed by x bit 0, y bit 1, z bit 2.
using ProjectedBox = std::array<ImVec2,8>;
inline void DrawFilledBox(ImDrawList* draws,const ProjectedBox& corners,ImU32 color) {
    constexpr int faces[6][4]={{0,1,5,4},{2,6,7,3},{0,2,3,1},{4,5,7,6},{0,4,6,2},{1,3,7,5}};
    for(const auto& face:faces) {
        const ImVec2 points[]{corners[face[0]],corners[face[1]],corners[face[2]],corners[face[3]]};
        draws->AddConvexPolyFilled(points,4,color);
    }
}
}
