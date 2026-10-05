#include "ui/hud/itemesp/ItemLabel.h"
#include "ui/clickgui/Fonts.h"
#include "ui/text/ImTextRenderer.h"
#include <string>

namespace nova::ui {
void DrawItemLabel(ImVec2 anchor,float size,double scale,std::int64_t count,ImU32 color,bool shadow) {
    ImGui::PushFont(Medium12,size);
    const auto label=std::to_string(count);
    const float x=anchor.x-text::TextWidth(label.c_str())*.5f;
    text::DrawText(ImGui::GetBackgroundDrawList(),label.c_str(),x,
        anchor.y-float(123.805*scale-2.47494)*size/9,color,shadow);
    ImGui::PopFont();
}
}
