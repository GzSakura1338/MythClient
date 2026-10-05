#pragma once
#include <algorithm>
#include "ui/overlay/HudBackground.h"

namespace nova::ui {
class BlockInHud {
public:
    void Draw(int fill,bool allowed,double nowMillis) {
        if(!ImGui::GetCurrentContext())return;
        if(fill<=0 || !allowed) {progress_=0;lastFill_=-1;return;}
        if(fill!=lastFill_) {from_=progress_;target_=std::clamp(fill/9.f,0.f,1.f);start_=nowMillis;lastFill_=fill;}
        const float t=std::min(1.f,float(nowMillis-start_)/50.f),ease=t<.5f?2*t*t:-1+(4-2*t)*t;
        progress_=from_+(target_-from_)*ease;
        const auto display=ImGui::GetIO().DisplaySize;
        const float scale=std::max(.5f,ImGui::GetFontSize()/14.f),radius=10*scale;
        const ImVec2 center{display.x*.5f-scale,display.y*.5f};
        auto* draw=ImGui::GetBackgroundDrawList();
        draw->AddCircle(center,radius,hud::BackgroundColor(0xCC331914),64,3*scale);
        if(progress_>.001f) {
            draw->PathArcTo(center,radius,1.5707963f,1.5707963f+progress_*6.2831853f,64);
            draw->PathStroke(0xFFFFEAE5,0,3*scale);
        }
    }
private:
    int lastFill_=-1;
    float progress_=0,from_=0,target_=0;
    double start_=0;
};
}
