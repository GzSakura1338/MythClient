#pragma once
#include "imgui.h"
#include <cmath>
namespace nova::ui::browser {
inline void DrawChromeAppIcon(ImDrawList* draw,ImVec2 center,float radius){
    constexpr float pi=3.14159265358979323846f;
    const ImU32 colors[]={IM_COL32(234,67,53,255),IM_COL32(251,188,5,255),IM_COL32(52,168,83,255)};
    for(int sector=0;sector<3;++sector){
        const float start=-5*pi/6+sector*2*pi/3;
        draw->PathClear();
        for(int i=0;i<=32;++i){const float a=start+i*(2*pi/3)/32;draw->PathLineTo({center.x+std::cos(a)*radius,center.y+std::sin(a)*radius});}
        for(int i=32;i>=0;--i){const float a=start+i*(2*pi/3)/32-pi/3;draw->PathLineTo({center.x+std::cos(a)*radius*.46f,center.y+std::sin(a)*radius*.46f});}
        draw->PathFillConcave(colors[sector]);
    }
    draw->AddCircleFilled(center,radius*.49f,IM_COL32(245,248,255,255),32);
    draw->AddCircleFilled(center,radius*.39f,IM_COL32(66,133,244,255),32);
}
inline void DrawChromeIcon(ImDrawList* draw,ImVec2 center,float radius,ImU32 color){
    constexpr float pi=3.14159265358979323846f;
    constexpr int steps=24;
    auto point=[&](float angle,float r){return ImVec2(center.x+std::cos(angle)*r,center.y+std::sin(angle)*r);};
    for(int blade=0;blade<3;++blade){
        const float start=-pi/6.f+blade*(2.f*pi/3.f);draw->PathClear();
        for(int i=0;i<=steps;++i)draw->PathLineTo(point(start+(2.f*pi/3.f-.12f)*i/steps,radius));
        for(int i=steps;i>=0;--i)draw->PathLineTo(point(start+(2.f*pi/3.f-.12f)*i/steps-pi/3.f,radius*.46f));
        draw->PathFillConcave(color);
    }
    draw->AddCircleFilled(center,radius*.32f,color,24);
}
}
