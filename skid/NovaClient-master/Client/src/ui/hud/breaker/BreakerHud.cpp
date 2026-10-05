#include "ui/hud/breaker/BreakerHud.h"
#include "ui/overlay/HudBackground.h"
#include "ui/clickgui/Fonts.h"
#include <algorithm>
#include <cmath>
#include <format>

namespace nova::ui {
float BreakerHud::Advance(float progress,float delta) {
    if(progress!=target_) {from_=value_;target_=progress;time_=0;}
    time_=std::min(.05f,time_+delta);
    value_=from_+(target_-from_)*(time_/.05f);
    return value_;
}
void BreakerHud::DrawBox(const ProjectedBox& points,const std::array<bool,8>& valid,bool fill) {
    auto* draw=ImGui::GetBackgroundDrawList();
    constexpr int edges[][2]={{0,1},{0,2},{0,4},{1,3},{1,5},{2,3},{2,6},{3,7},{4,5},{4,6},{5,7},{6,7}};
    if(fill) {
        constexpr int faces[][4]={{0,1,3,2},{4,5,7,6},{0,4,6,2},{1,5,7,3},{2,3,7,6}};
        for(auto& f:faces)if(valid[f[0]] && valid[f[1]] && valid[f[2]] && valid[f[3]])
            draw->AddQuadFilled(points[f[0]],points[f[1]],points[f[2]],points[f[3]],0x30331914);
    }
    for(auto& e:edges)if(valid[e[0]] && valid[e[1]])draw->AddLine(points[e[0]],points[e[1]],0xBFFFEAE5,1.5f);
}
void BreakerHud::DrawProgress(float bottomInset) const {
    const float value=std::clamp(1.21f*value_,0.f,1.f);
    const auto display=ImGui::GetIO().DisplaySize;
    auto* draw=ImGui::GetBackgroundDrawList();
    const auto& style=ImGui::GetStyle();
    const float uiScale=std::max(.01f,style.FontScaleMain*style.FontScaleDpi);
    const float fit=std::clamp(std::min((display.x-24.f)/(280.f*uiScale),
        (display.y*.5f-12.f)/(118.f*uiScale)),.1f,1.f);
    ImGui::PushFont(Medium12,kOverlayTextSize*fit);
    const float scale=ImGui::GetFontSize()/kOverlayTextSize;
    const float width=280.f*scale,height=70.f*scale;
    const float x=std::round((display.x-width)*.5f);
    if(bottomInset<=0)bottomInset=44.f*scale;
    const float y=std::round(std::max(12.f*scale,display.y-bottomInset-28.f*scale-height));
    constexpr ImU32 textColor=0xFFFFEAE5;
    hud::Background(draw,{x,y},{x+width,y+height},0xCC331914,12.f*scale);
    const auto point=[=](float dx,float dy) {return ImVec2(x+dx*scale,y+dy*scale);};
    draw->AddRectFilled(point(14,12),point(46,44),hud::BackgroundColor(IM_COL32(0,0,0,90)),8.f*scale);
    const float stroke=1.8f*scale;
    draw->AddLine(point(20,21),point(20,36),textColor,stroke);
    draw->AddLine(point(20,32),point(40,32),textColor,stroke);
    draw->AddLine(point(40,27),point(40,36),textColor,stroke);
    draw->AddRectFilled(point(22,24),point(27,29),textColor,1.5f*scale);
    draw->AddRectFilled(point(29,24),point(40,30),textColor,2.f*scale);
    const char* title="Bed Breaker";
    const float titleHeight=ImGui::CalcTextSize(title).y;
    draw->AddText({x+56.f*scale,y+28.f*scale-titleHeight*.5f},textColor,title);
    ImGui::PopFont();
    ImGui::PushFont(ExtraBold20?ExtraBold20:Medium12,kOverlayTitleSize*fit);
    const auto label=std::format("{:.0f}%",value*100.f);
    const ImVec2 labelSize=ImGui::CalcTextSize(label.c_str());
    draw->AddText({x+width-14.f*scale-labelSize.x,y+28.f*scale-labelSize.y*.5f},textColor,label.c_str());
    ImGui::PopFont();
    const float barWidth=width-28.f*scale,barHeight=7.f*scale;
    const ImVec2 bar=point(14,51);
    draw->AddRectFilled(bar,{bar.x+barWidth,bar.y+barHeight},IM_COL32(0,0,0,110),barHeight*.5f);
    const float filled=barWidth*value;
    if(filled>0)draw->AddRectFilled(bar,{bar.x+filled,bar.y+barHeight},textColor,std::min(barHeight,filled)*.5f);
}
}
