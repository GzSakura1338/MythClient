#pragma once
#include <array>
#include <vector>
#include "imgui.h"

namespace nova::ui {
// Already clipped and projected by the host; no camera or world dependencies.
struct ScreenGeometry {
    std::vector<std::vector<ImVec2>> faces;
    std::vector<std::array<ImVec2,2>> edges;
};
inline void DrawGeometry(const ScreenGeometry& geometry,ImU32 fill,ImU32 outline,float thickness) {
    auto* draws=ImGui::GetBackgroundDrawList();
    for(const auto& polygon:geometry.faces)
        if(polygon.size()>=3)draws->AddConvexPolyFilled(polygon.data(),static_cast<int>(polygon.size()),fill);
    for(const auto& edge:geometry.edges)draws->AddLine(edge[0],edge[1],outline,thickness);
}
}
