#include "ui/hud/compass/CompassHud.h"
#include <cmath>
#include "imgui.h"

namespace nova::ui {
void DrawCompass(float radians,float unit) {
    if(!ImGui::GetCurrentContext() || !std::isfinite(radians) || !std::isfinite(unit) || unit<=0)return;
    const auto display=ImGui::GetIO().DisplaySize;
    if(display.x<=1 || display.y<=1)return;
    const float centerX=display.x*.5f,centerY=display.y*.5f;
    const float cosine=std::cos(radians),sine=std::sin(radians);
    const auto rotate=[&](float px,float py) {
        const float dx=px-centerX,dy=py-centerY;
        return ImVec2(centerX+dx*cosine-dy*sine,centerY+dx*sine+dy*cosine);
    };
    const float apexY=centerY-45.f*unit,baseY=apexY+10.f*unit,halfWidth=5.f*unit;
    ImGui::GetBackgroundDrawList()->AddTriangleFilled(
        rotate(centerX,apexY),rotate(centerX-halfWidth,baseY),rotate(centerX+halfWidth,baseY),IM_COL32_WHITE);
}
}
