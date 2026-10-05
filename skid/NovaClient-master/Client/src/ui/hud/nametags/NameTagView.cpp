#include "ui/hud/nametags/NameTagView.h"
#include "ui/clickgui/Fonts.h"
#include "ui/overlay/HudBackground.h"
#include "ui/text/ImTextRenderer.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace nova::ui::nametags {
namespace {
void Heart(ImDrawList* draws,float cx,float cy,float size,ImU32 color) {
    const float lobe=size*.25f,shoulder=cy-lobe*.5f;
    draws->AddCircleFilled({cx-lobe,shoulder},lobe,color,16);
    draws->AddCircleFilled({cx+lobe,shoulder},lobe,color,16);
    draws->AddTriangleFilled({cx-lobe*2,shoulder},{cx+lobe*2,shoulder},{cx,cy+lobe*1.6f},color);
}
}
void DrawClassic(ImVec2 anchor,float scale,const std::vector<Glyph>& label,const Style& options) {
    ImGui::PushFont(Classic9,9*scale);
    auto* draws=ImGui::GetBackgroundDrawList();
    const auto display=ImGui::GetIO().DisplaySize;
    const auto measure=[&](const Glyph& glyph) {
        return ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(),std::numeric_limits<float>::max(),0,
            glyph.text.data(),glyph.text.data()+glyph.text.size()).x+(glyph.bold?scale:0);
    };
    float width=0;
    for(const auto& glyph:label)width+=measure(glyph);
    const float left=anchor.x-width/2-2*scale,top=anchor.y-13*scale;
    if(left+width+4*scale<0 || left>display.x || top+12*scale<0 || top>display.y) {ImGui::PopFont();return;}
    const ImU32 alpha=std::min<ImU32>((options.background&IM_COL32_A_MASK)>>IM_COL32_A_SHIFT,128);
    draws->AddRectFilled({left,top},{left+width+4*scale,top+12*scale},
        hud::BackgroundColor((options.background&~IM_COL32_A_MASK)|(alpha<<IM_COL32_A_SHIFT)));
    const float x=anchor.x-width/2+.5f*scale,y=anchor.y-11*scale;
    for(bool shadow:{true,false}) {
        if(shadow && !options.shadow)continue;
        const float offset=shadow?scale:0;
        float advance=0;
        for(const auto& glyph:label) {
            const auto color=shadow?text::Dimmed(glyph.color):glyph.color;
            const ImVec2 position{x+advance+offset,y+offset};
            const int start=draws->VtxBuffer.Size;
            draws->AddText(position,color,glyph.text.c_str());
            if(glyph.bold)draws->AddText({position.x+scale,position.y},color,glyph.text.c_str());
            if(glyph.italic)for(int i=start;i<draws->VtxBuffer.Size;++i)
                draws->VtxBuffer[i].pos.x+=(4.5f*scale-(draws->VtxBuffer[i].pos.y-position.y))*.25f;
            const float width=measure(glyph);
            if(glyph.strike)draws->AddRectFilled({position.x,position.y+4*scale},{position.x+width,position.y+5*scale},color);
            if(glyph.underline)draws->AddRectFilled({position.x,position.y+8*scale},{position.x+width,position.y+9*scale},color);
            advance+=width;
        }
    }
    ImGui::PopFont();
}
void DrawDefault(ImVec2 anchor,float scale,const std::string& name,const std::string& health,bool team,const Style& options) {
    ImGui::PushFont(Medium12,kOverlayTextSize*scale);
    auto* draws=ImGui::GetBackgroundDrawList();
    const float font=ImGui::GetFontSize(),lineHeight=text::TextHeight();
    const float padding=std::round(font*.30f),radius=std::round(font*.34f),gap=std::round(font*.24f);
    const float iconGap=std::round(font*.18f),iconSize=std::round(font*.62f),height=lineHeight+padding*2;
    const bool showStatus=options.status && team,showHealth=options.health && !health.empty();
    const float statusWidth=showStatus?text::TextWidth("Team")+padding*2:0;
    const float nameWidth=text::TextWidth(name.c_str())+padding*2;
    const float healthWidth=showHealth?text::TextWidth(health.c_str())+iconSize+iconGap+padding*2:0;
    const float total=nameWidth+(showStatus?statusWidth+gap:0)+(showHealth?healthWidth+gap:0);
    float x=anchor.x-total*.5f;
    const float y=anchor.y-height;
    const auto pill=[&](float width) {hud::Background(draws,{x,y},{x+width,y+height},options.background,radius);};
    if(showStatus) {
        pill(statusWidth);
        text::DrawText(draws,"Team",x+padding,y+(height-lineHeight)*.5f,text::Dimmed(options.text),options.shadow);
        x+=statusWidth+gap;
    }
    pill(nameWidth);
    text::DrawText(draws,name.c_str(),x+padding,y+(height-lineHeight)*.5f,options.text,options.shadow);
    x+=nameWidth+gap;
    if(showHealth) {
        pill(healthWidth);
        const float contentX=x+padding,middle=y+height*.5f;
        Heart(draws,contentX+iconSize*.5f,middle,iconSize,IM_COL32(255,85,85,255));
        text::DrawText(draws,health.c_str(),contentX+iconSize+iconGap,y+(height-lineHeight)*.5f,options.text,options.shadow);
    }
    ImGui::PopFont();
}
}
