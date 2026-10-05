#include "ui/hud/cswin/CsWin.h"
#include "ui/clickgui/Fonts.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace nova::ui::cswin {
namespace {
struct Palette {ImU32 accent,highlight,banner,strip,light,dark,badgeText;};
constexpr Palette TColors{IM_COL32(228,190,88,255),IM_COL32(255,228,160,255),
    IM_COL32(57,42,23,255),IM_COL32(98,82,29,255),IM_COL32(255,222,117,255),
    IM_COL32(51,40,12,255),IM_COL32(43,33,8,255)};
constexpr Palette CTColors{IM_COL32(115,178,238,255),IM_COL32(193,222,255,255),
    IM_COL32(21,38,59,255),IM_COL32(32,67,105,255),IM_COL32(151,209,255,255),
    IM_COL32(12,30,50,255),IM_COL32(10,29,48,255)};
constexpr ImU32 White=IM_COL32_WHITE;
float Clamp(float v) { return std::clamp(v,0.f,1.f); }
float Smooth(float v) { v=Clamp(v);return v*v*(3-2*v); }
ImU32 Alpha(ImU32 c,float a) { return (c&~IM_COL32_A_MASK)|(ImU32(std::lround(((c>>IM_COL32_A_SHIFT)&255)*Clamp(a)))<<IM_COL32_A_SHIFT); }
ImU32 Mix(ImU32 a,ImU32 b,float t) {
    ImVec4 x=ImGui::ColorConvertU32ToFloat4(a),y=ImGui::ColorConvertU32ToFloat4(b);t=Clamp(t);
    return ImGui::ColorConvertFloat4ToU32({x.x+(y.x-x.x)*t,x.y+(y.y-x.y)*t,x.z+(y.z-x.z)*t,x.w+(y.w-x.w)*t});
}
float Flash(float t,float a,float b,float c,float v) {
    if(t<=a)return 0;
    if(t<=b)return (t-a)/(b-a)*v;
    if(t<=c)return v+(1-v)*(t-b)/(c-b);
    return 1;
}
void Rect(ImDrawList* d,float x,float y,float w,float h,ImU32 c) {
    if(w>0 && h>0)d->AddRectFilled({x,y},{x+w,y+h},c);
}
void CenterRect(ImDrawList* d,float cx,float y,float w,float h,ImU32 c) { Rect(d,cx-w*.5f,y,w,h,c); }
void StripRect(ImDrawList* d,float cx,float y,float w,float h,ImU32 c,float a) {
    if(w<=0)return;
    const float left=cx-w*.5f,fade=w*.18f;
    Rect(d,left+fade,y,w-fade*2,h,Alpha(c,a));
    for(int i=0;i<12;++i) {
        const float lo=i/12.f,hi=(i+1)/12.f;
        const auto c0=Alpha(c,a*Smooth(lo)),c1=Alpha(c,a*Smooth(hi));
        d->AddRectFilledMultiColor({left+fade*lo,y},{left+fade*hi,y+h},c0,c1,c1,c0);
        const float right=left+w-fade;
        const auto r0=Alpha(c,a*Smooth(1-lo)),r1=Alpha(c,a*Smooth(1-hi));
        d->AddRectFilledMultiColor({right+fade*lo,y},{right+fade*hi,y+h},r0,r1,r1,r0);
    }
}
ImFont* Bold() { return ExtraBold20?ExtraBold20:ImGui::GetFont(); }
void Text(ImDrawList* d,ImFont* f,const char* text,float size,float x,float y,ImU32 c,float maxWidth=1e6f) {
    auto bounds=f->CalcTextSizeA(size,1e6f,0,text);
    if(bounds.x>maxWidth){size*=maxWidth/bounds.x;bounds=f->CalcTextSizeA(size,1e6f,0,text);}
    d->AddText(f,size,{x-bounds.x*.5f,y-bounds.y*.5f},c,text);
}
std::uint32_t Seed(unsigned col,unsigned row) {
    std::uint32_t v=col*0x1F1F1F1Fu ^ row*0x45D9F3Bu;v^=v>>16;v*=0x7FEB352Du;return v^(v>>15);
}
}
void Draw(double elapsed,const Snapshot& player,double duration) {
    if(duration<=.6 || !std::isfinite(duration) || elapsed<0 || elapsed>=duration || !ImGui::GetCurrentContext() || !ImGui::GetFont())return;
    const auto& colors=player.style==Style::CT?CTColors:TColors;
    const auto Gold=colors.accent,Highlight=colors.highlight,Banner=colors.banner,Strip=colors.strip,Light=colors.light,Dark=colors.dark;
    const auto* viewport=ImGui::GetMainViewport();
    const float s=std::min(viewport->Size.x/1920.f,viewport->Size.y/1030.f)*.88f;
    if(s<=0)return;
    auto* d=ImGui::GetBackgroundDrawList();
    const float cx=viewport->Pos.x+viewport->Size.x*.5f;
    const float y=viewport->Pos.y+std::max(12.f,62*s),h=112*s;
    const float stripY=y+h+33*s,stripH=137*s;
    const float p=float(elapsed/duration),exit=elapsed>=duration-.6?float((elapsed-(duration-.6))/.6):-1;
    const float collapse=exit<0?0:Clamp((exit-.7f)/.3f);
    const float whiten=exit<0?0:Clamp((exit-.25f)/.25f);
    const float width=476*s*(1-collapse),stripWidth=928*s*(1-collapse);
    const float panelA=exit>=0?1:Clamp((p-.036f)/.006f);
    const float textA=exit>=0?1-Clamp(exit/.25f):Clamp((p-.034f)/.021f);
    if(exit<0 && p>=.012f && p<.036f)
        CenterRect(d,cx,y,476*s*Flash(p,0,.012f,.028f,.25f),h,White);
    if(panelA>0 && width>0) {
        CenterRect(d,cx,y,width,h,Alpha(Mix(Banner,White,whiten),panelA));
        const float line=std::min(width*.5f,std::max(1.f,3*s));
        const auto edge=Alpha(Mix(Gold,White,whiten),panelA);
        Rect(d,cx-width*.5f,y,line,h,edge);Rect(d,cx+width*.5f-line,y,line,h,edge);
        if(textA>0) {
            const auto color=Alpha(Gold,textA*panelA);
            Text(d,Bold(),"ROUND WON",38*s,cx,y+h*.5f+3*s,color);
            const float move=52*s*(1-(exit>=0?1:Clamp((p-.04f)/.04f)));
            Text(d,Bold(),"\xC2\xBB",42*s,cx-width*.5f+28*s+move,y+h*.5f,color);
            Text(d,Bold(),"\xC2\xAB",42*s,cx+width*.5f-28*s-move,y+h*.5f,color);
        }
    }
    if(exit<0 && p>=.094f && p<.12f)
        StripRect(d,cx,stripY,928*s*Flash(p,.086f,.094f,.108f,.3f),stripH,White,1);
    const float stripA=exit>=0?1:Clamp((p-.105f)/.015f);
    const float contentA=exit>=0?1-Clamp(exit/.25f):Clamp((p-.11f)/.025f);
    if(stripA<=0 || stripWidth<=0)return;
    StripRect(d,cx,stripY,stripWidth,stripH,Mix(Strip,White,whiten),stripA);
    if(contentA<=0)return;
    const float alpha=contentA*stripA,left=cx-stripWidth*.5f,right=cx+stripWidth*.5f;
    const float cell=20*s,gap=5*s;
    const int rows=std::max(1,int(stripH/cell));
    const float rowGap=rows>1?(stripH-cell*rows)/(rows-1):0;
    // Reference grid starts when the strip's contents first appear.
    const float time=std::max(0.f,float(elapsed)-float(duration)*.11f);
    unsigned col=0;
    for(float x=left;x+cell<=right;x+=cell+gap,++col)for(int row=0;row<rows;++row) {
        const auto seed=Seed(col,row);const float mid=x+cell*.5f,fade=stripWidth*.18f;
        const float edge=std::min(Smooth((mid-left)/fade),Smooth((right-mid)/fade));
        const float shimmer=.5f+.5f*std::sin(time*(1.08f+((seed>>8)&15)*.078f)+(seed&255)*.041f);
        const float slow=.5f+.5f*std::sin(time*(.285f+(seed&7)*.0255f)+((seed>>16)&255)*.031f);
        Rect(d,x,stripY+row*(cell+rowGap),cell,cell,
            Alpha(Mix(Dark,Light,.18f+(shimmer*.72f+slow*.28f)*.52f),(.08f+shimmer*.2f)*alpha*edge));
    }
    const float head=104*s,contentGap=22*s,tagH=27*s;
    auto* regular=PanelRegular?PanelRegular:ImGui::GetFont();
    const float tagWidth=regular->CalcTextSizeA(18*s,1e6f,0,"Round-winning MVP").x+20*s;
    const float nameWidth=Bold()->CalcTextSizeA(42*s,1e6f,0,player.name.c_str()).x;
    const float column=std::min(std::max(tagWidth,nameWidth),stripWidth-head-contentGap-48*s);
    const float avatarX=cx-(head+contentGap+column)*.5f,avatarY=stripY+(stripH-head)*.5f;
    if(player.portrait.texture)DrawPortrait(d,player.portrait,{avatarX,avatarY},{avatarX+head,avatarY+head},Alpha(White,alpha),0);
    else {Rect(d,avatarX,avatarY,head,head,Alpha(Dark,alpha));Text(d,Bold(),"?",42*s,avatarX+head*.5f,avatarY+head*.5f,Alpha(Gold,alpha));}
    const float textX=avatarX+head+contentGap;
    Rect(d,textX,stripY+31*s,tagWidth,tagH,Alpha(Gold,alpha));
    Text(d,regular,"Round-winning MVP",18*s,textX+tagWidth*.5f,stripY+31*s+tagH*.5f,Alpha(colors.badgeText,alpha));
    Text(d,Bold(),player.name.c_str(),42*s,textX+column*.5f,stripY+91*s,Alpha(Highlight,alpha),column);
}
}
