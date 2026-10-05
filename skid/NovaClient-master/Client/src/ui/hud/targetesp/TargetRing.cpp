#include "ui/hud/targetesp/TargetRing.h"
#include "imgui_internal.h"
#include <algorithm>

namespace nova::ui::targetesp {
namespace {
Vertex Mix(const Vertex& a, const Vertex& b, float t) {
    return {{a.position.x + (b.position.x-a.position.x)*t, a.position.y + (b.position.y-a.position.y)*t},
        {a.color.x+(b.color.x-a.color.x)*t, a.color.y+(b.color.y-a.color.y)*t,
         a.color.z+(b.color.z-a.color.z)*t, a.color.w+(b.color.w-a.color.w)*t}};
}
float Distance(const Vertex& v, int plane, ImVec2 display) {
    switch (plane) {
    case 0: return v.position.x;
    case 1: return display.x-v.position.x;
    case 2: return v.position.y;
    default: return display.y-v.position.y;
    }
}
bool Finite(const Vertex& v) {
    return std::isfinite(v.position.x) && std::isfinite(v.position.y) &&
        std::isfinite(v.color.x) && std::isfinite(v.color.y) &&
        std::isfinite(v.color.z) && std::isfinite(v.color.w);
}
void Triangle(ImDrawList* draw, const Vertex& a, const Vertex& b, const Vertex& c) {
    draw->PrimReserve(3,3);
    const auto index = draw->_VtxCurrentIdx;
    draw->PrimWriteIdx(static_cast<ImDrawIdx>(index));
    draw->PrimWriteIdx(static_cast<ImDrawIdx>(index+1));
    draw->PrimWriteIdx(static_cast<ImDrawIdx>(index+2));
    const auto uv = draw->_Data->TexUvWhitePixel;
    for (const auto* v : {&a,&b,&c})
        draw->PrimWriteVtx(v->position, uv, ImGui::ColorConvertFloat4ToU32(v->color));
}
}

void Draw(const Mesh& mesh, ImDrawList* draw, ImVec2 display) {
    if (!draw || !(display.x > 1 && display.y > 1) ||
        !std::isfinite(display.x) || !std::isfinite(display.y)) return;
    draw->PushClipRect({0,0}, display, true);
    std::vector<Vertex> polygon, clipped;
    polygon.reserve(8); clipped.reserve(8);
    for (const auto& triangle : mesh.triangles) {
        if (!std::all_of(triangle.begin(), triangle.end(), Finite)) continue;
        polygon.assign(triangle.begin(), triangle.end());
        for (int plane=0; plane<4 && polygon.size()>=3; ++plane) {
            clipped.clear();
            for (size_t i=0; i<polygon.size(); ++i) {
                const auto& a=polygon[i]; const auto& b=polygon[(i+1)%polygon.size()];
                const float da=Distance(a,plane,display), db=Distance(b,plane,display);
                if (da>=0) clipped.push_back(a);
                if ((da>=0)!=(db>=0)) clipped.push_back(Mix(a,b,da/(da-db)));
            }
            polygon.swap(clipped);
        }
        for (size_t i=1; i+1<polygon.size(); ++i) Triangle(draw,polygon[0],polygon[i],polygon[i+1]);
    }
    for (auto line : mesh.lines) {
        if (!Finite(line[0]) || !Finite(line[1])) continue;
        bool visible=true;
        for (int plane=0; plane<4; ++plane) {
            const float a=Distance(line[0],plane,display), b=Distance(line[1],plane,display);
            if (a<0 && b<0) { visible=false; break; }
            if ((a>=0)!=(b>=0)) line[a<0?0:1]=Mix(line[0],line[1],a/(a-b));
        }
        if (visible) draw->AddLine(line[0].position,line[1].position,
            ImGui::ColorConvertFloat4ToU32(Mix(line[0],line[1],.5f).color),2.f);
    }
    draw->PopClipRect();
}
}
