#include "ui/hud/bedplates/BedPlateHud.h"
#include "ui/overlay/HudBackground.h"
#include "ui/clickgui/Fonts.h"
#include "ui/text/ImTextRenderer.h"
#include <algorithm>
#include <format>

namespace nova::ui {
namespace {
void Icon(ImDrawList* d,float x,float y,float s,bool bed,ImU32 ink) {
    if(bed) {
        d->AddRectFilled({x,y+5*s},{x+20*s,y+12*s},ink,2*s);
        d->AddRectFilled({x+2*s,y+2*s},{x+7*s,y+6*s},ink,1.5f*s);
        d->AddLine({x,y+3*s},{x,y+16*s},ink,2*s);
        d->AddLine({x+20*s,y+10*s},{x+20*s,y+16*s},ink,2*s);
    } else {
        const ImVec2 top{x+10*s,y},left{x,y+5*s},right{x+20*s,y+5*s},middle{x+10*s,y+10*s},bottom{x+10*s,y+20*s};
        d->AddQuadFilled(top,right,middle,left,ink);
        d->AddQuad(left,middle,bottom,{x,y+15*s},ink,1.2f*s);
        d->AddQuad(middle,right,{x+20*s,y+15*s},bottom,ink,1.2f*s);
    }
}
}
void DrawBedPlate(const BedPlateSnapshot& plate,const BedPlateStyle& style) {
    if(!ImGui::GetCurrentContext() || !ImGui::GetFont())return;
    const auto display=ImGui::GetIO().DisplaySize;
    auto* draws=ImGui::GetBackgroundDrawList();
    const float scale=style.size*(style.distanceScale?std::clamp(1.f-static_cast<float>(std::max(0.,plate.distance-10))*.003f,.65f,1.f):1.f);
    const float s=scale*ImGui::GetStyle().FontScaleMain*ImGui::GetStyle().FontScaleDpi;
    ImGui::PushFont(Medium12,13*scale);
    const auto title=style.minimal?plate.material:"Bed · "+plate.material;
    const auto meters=std::format("{:.0f}m",plate.distance);
    const char* status=plate.covered==0?"Exposed":plate.covered<8?"Partial":"Covered";
    const std::string details=style.distance?std::string(status)+" · "+meters:status;
    float textWidth=text::TextWidth(title.c_str());
    ImGui::PushFont(Medium12,11*scale);
    if(!style.minimal)textWidth=std::max(textWidth,text::TextWidth(details.c_str()));
    ImGui::PopFont();
    if(style.minimal && style.distance)textWidth+=text::TextWidth(meters.c_str())+8*s;
    const float width=textWidth+46*s,height=(style.minimal?32:48)*s;
    const float x=plate.anchor.x-width*.5f,y=plate.anchor.y-height;
    if(x+width<0 || x>display.x || y+height<0 || y>display.y) {ImGui::PopFont();return;}
    hud::Background(draws,{x,y},{x+width,y+height},0xCC331914,9*s);
    Icon(draws,x+10*s,y+(height-20*s)*.5f,s,plate.covered==0,0xFFFFEAE5);
    text::DrawText(draws,title.c_str(),x+38*s,y+8*s,0xFFFFEAE5,style.shadow);
    if(style.minimal) {
        if(style.distance)text::DrawText(draws,meters.c_str(),x+width-text::TextWidth(meters.c_str())-8*s,y+8*s,0xBFFFEAE5,style.shadow);
    } else {
        ImGui::PushFont(Medium12,11*scale);
        text::DrawText(draws,details.c_str(),x+38*s,y+27*s,0xBFFFEAE5,style.shadow);
        ImGui::PopFont();
    }
    ImGui::PopFont();
}
}
