#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include "imgui.h"
#include "ui/skin/SkinTextures.h"

namespace nova::ui {
struct PortraitLayer { ImVec2 min{0,0}, max{1,1}; };
struct Portrait {
    std::uint64_t texture = 0;
    std::array<PortraitLayer,4> layers{};
    unsigned layerCount = 1;
    bool nearest = false;
};

inline void DrawPortrait(ImDrawList* draws,const Portrait& portrait,ImVec2 min,ImVec2 max,ImU32 tint,float rounding) {
    const auto resolved=skins::Resolve(portrait.texture);
    const ImVec2 scale=ImGui::GetIO().DisplayFramebufferScale;
    if(scale.x>0 && scale.y>0) {
        const ImVec2 size(max.x-min.x,max.y-min.y);
        min.x=std::round(min.x*scale.x)/scale.x;min.y=std::round(min.y*scale.y)/scale.y;
        max.x=min.x+std::round(size.x*scale.x)/scale.x;max.y=min.y+std::round(size.y*scale.y)/scale.y;
    }
    const auto& platform=ImGui::GetPlatformIO();
    const bool sampling=portrait.nearest && platform.DrawCallback_SetSamplerNearest && platform.DrawCallback_SetSamplerLinear;
    if(sampling)draws->AddCallback(platform.DrawCallback_SetSamplerNearest,nullptr);
    for(unsigned i=0;i<std::min<unsigned>(portrait.layerCount,portrait.layers.size());++i)
        draws->AddImageRounded(resolved,min,max,portrait.layers[i].min,portrait.layers[i].max,tint,rounding);
    if(sampling)draws->AddCallback(platform.DrawCallback_SetSamplerLinear,nullptr);
}
}
