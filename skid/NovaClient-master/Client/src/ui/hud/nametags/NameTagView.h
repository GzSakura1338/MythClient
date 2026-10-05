#pragma once
#include <string>
#include <vector>
#include "imgui.h"

namespace nova::ui::nametags {
struct Glyph {
    std::string text;
    ImU32 color=IM_COL32_WHITE;
    bool bold=false,italic=false,underline=false,strike=false;
};
struct Style {
    ImU32 background=0xCC331914,text=0xFFFFEAE5;
    bool shadow=false,status=true,health=true;
};
void DrawClassic(ImVec2 anchor,float scale,const std::vector<Glyph>& label,const Style& style);
void DrawDefault(ImVec2 anchor,float scale,const std::string& name,const std::string& health,bool team,const Style& style);
}
