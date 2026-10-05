#pragma once
#include <algorithm>
#include <string>
#include "ui/text/ImTextRenderer.h"

namespace nova::text {
// Shared Watermark digit-cell animation. Labels and units stay stationary.
struct RollingText {
    std::string previous, current;
    float progress = 1;
    void Update(const std::string& next, float delta, bool fresh) {
        progress = std::min(1.f, progress + delta * 4.5f);
        if (fresh || current.empty()) { previous = current = next; progress = 1; }
        else if (progress >= 1 && next != current) { previous = current; current = next; progress = 0; }
    }
    void Draw(ImDrawList* draws, float x, float y, float cell, ImU32 color, bool shadow = false) const {
        const float height = TextHeight(), inverse = 1-progress;
        const float eased = 1-inverse*inverse*inverse;
        draws->PushClipRect({x,y},{x+cell*current.size(),y+height},true);
        for (std::size_t i=0; i<current.size(); ++i) {
            const char next[]{current[i],0}, before[]{i<previous.size()?previous[i]:' ',0};
            const float left = x+cell*static_cast<float>(i);
            const float nextX = left+(cell-TextWidth(next))*.5f;
            if (before[0]==next[0] || progress>=1) DrawText(draws,next,nextX,y,color,shadow);
            else {
                DrawText(draws,before,left+(cell-TextWidth(before))*.5f,y-height*eased,color,shadow);
                DrawText(draws,next,nextX,y+height*(1-eased),color,shadow);
            }
        }
        draws->PopClipRect();
    }
};
}
