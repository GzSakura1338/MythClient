#pragma once
#include <array>
#include "ui/hud/world/ProjectedBox.h"

namespace nova::ui {
class BreakerHud {
public:
    float Advance(float progress,float delta);
    // Host supplies the bottom game's occupied area in pixels; zero uses UI scale.
    void DrawProgress(float bottomInset=0) const;
    static void DrawBox(const ProjectedBox& points,const std::array<bool,8>& valid,bool fill);
private:
    float value_=0,from_=0,target_=0,time_=0;
};
}
