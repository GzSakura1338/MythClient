#include "ui/workspace/ForegroundWorkspace.h"
#include "ui/workspace/PanelSnapshot.h"
#include "ui/clickgui/browser/BrowserPage.h"
#include "ui/clickgui/browser/BrowserIcon.h"
#include "ui/clickgui/ClickGui.h"
#include "ui/clickgui/panel/PanelClickGui.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/clickgui/panel/Widgets.h"
#include "ui/clickgui/panel/ReferenceStyle.h"
#include "ui/backend/blur/Blur.h"
#include "chrome/BrowserService.h"
#include "config/BrowserSettings.h"
#include "bind/BrowserCapture.h"
#include "core/Instance.h"
#include "imgui_internal.h"
#include <atomic>
#include <cmath>

namespace nova::ui::workspace {
namespace {
using namespace menu::panel;
struct Rect {ImVec2 pos{},size{};};
struct Workspace {
    std::atomic<State> state{State::Embedded};
    std::atomic<bool> capture{false},escape{false},lostFocus{false},escapeDown{false};
    bool initialized=false,maximized=false,transition=false,dirty=false,focusChrome=false;
    bool panelShelved=false,panelTransition=false,wasPanelOpen=false,focusPanel=false;
    std::atomic<bool> panelCapture{false};
    Rect panelNormal{},panelShown{},panelFrom{},panelTarget{};
    float panelElapsed=0,panelTilt=0,panelFromTilt=0,panelDecoration=0,panelFromDecoration=0;
    PanelSnapshot panelPreview,chromePreview;
    Rect normal{},shown{},from{},target{};
    float elapsed=0,duration=.26f,tilt=0,fromTilt=0,decoration=0,fromDecoration=0;
    double saveAt=0;
    ImVec2 lastDisplay{};
    PopupAnimations popups;
    Rect shelfRects[2]{};
    int shelfFrames[2]{-1,-1};
};
Workspace& W(){return Instance<Workspace>();}
Rect ShelfTarget(bool panel=false){const auto* v=ImGui::GetMainViewport();const float width=std::min(190.f,v->WorkSize.x*.22f);
    const bool both=W().panelShelved && Current()==State::Shelved;
    auto height=[&](ImVec2 size){const float aspect=size.y>0?size.x/size.y:1.4f;return std::clamp(width/(.86f*aspect),width*.55f,width*1.2f);};
    float chromeHeight=height(W().normal.size),panelHeight=height(W().panelNormal.size);
    const float gap=38.f,total=both?chromeHeight+panelHeight+gap:panel?panelHeight:chromeHeight;
    const float fit=std::min(1.f,std::max(1.f,v->WorkSize.y-48)/total);
    chromeHeight*=fit;panelHeight*=fit;
    return {v->WorkPos+ImVec2(14,std::max(16.f,(v->WorkSize.y-total*fit)*.5f)+(both&&panel?chromeHeight+gap*fit:0)),{width*fit,panel?panelHeight:chromeHeight}};}
Rect Shelf(bool panel=false){
    auto& w=W();const int index=panel?1:0,frame=ImGui::GetFrameCount();
    auto& rect=w.shelfRects[index];auto& last=w.shelfFrames[index];
    if(last!=frame){
        const auto target=ShelfTarget(panel);
        if(last<0)rect=target;
        else {const float mix=1.f-std::exp(-18.f*ImGui::GetIO().DeltaTime);rect={ImLerp(rect.pos,target.pos,mix),ImLerp(rect.size,target.size,mix)};}
        last=frame;
    }
    return rect;
}
Rect Bounds(Rect r){const auto* v=ImGui::GetMainViewport();const auto max=v->WorkSize-ImVec2(16,16);
    r.size.x=std::clamp(r.size.x,std::min(480.f,max.x),max.x);r.size.y=std::clamp(r.size.y,std::min(320.f,max.y),max.y);
    r.pos.x=std::clamp(r.pos.x,v->WorkPos.x+8,v->WorkPos.x+v->WorkSize.x-r.size.x-8);
    r.pos.y=std::clamp(r.pos.y,v->WorkPos.y+8,v->WorkPos.y+v->WorkSize.y-r.size.y-8);return r;}
Rect Maximized(){const auto* v=ImGui::GetMainViewport();const float left=Shelf().size.x+24.f;
    return Bounds({v->WorkPos+ImVec2(left,8),v->WorkSize-ImVec2(left+8,16)});}
void Initialize(){auto& w=W();if(w.initialized)return;const auto s=config::LoadBrowserPlacement();const auto* v=ImGui::GetMainViewport();
    w.normal=Bounds({v->WorkPos+ImVec2(v->WorkSize.x*s.x/10000,v->WorkSize.y*s.y/10000),{v->WorkSize.x*s.width/10000,v->WorkSize.y*s.height/10000}});
    w.lastDisplay=v->WorkSize;w.initialized=true;}
void Save(){auto& w=W();if(!w.initialized)return;const auto* v=ImGui::GetMainViewport();
    config::SaveBrowserPlacement({int((w.normal.pos.x-v->WorkPos.x)*10000/v->WorkSize.x),int((w.normal.pos.y-v->WorkPos.y)*10000/v->WorkSize.y),int(w.normal.size.x*10000/v->WorkSize.x),int(w.normal.size.y*10000/v->WorkSize.y)});w.dirty=false;}
void Animate(Rect target,float duration){auto& w=W();w.from=w.shown;w.fromTilt=w.tilt;w.fromDecoration=w.decoration;w.target=target;w.elapsed=0;w.duration=duration;w.transition=true;w.popups.reset();
    ImGui::ClosePopupsOverWindow(nullptr,false);ImGui::ClearActiveID();chrome::Focus(false);}
bool Button(const char* id,const char8_t* icon,const char* tip,const ColorTheme& theme,ImVec2 at){
    ImGui::SetCursorScreenPos(at);const bool hit=ImGui::InvisibleButton(id,{Scale(28),Scale(28)});const bool hover=ImGui::IsItemHovered();
    const float a=reference::Motion(ImGui::GetItemID(),hover?1.f:0.f);auto* d=ImGui::GetWindowDrawList();
    d->AddRectFilled(at,at+ImVec2(Scale(28),Scale(28)),color::ReAlpha(theme.buttonHoveredBgColor,int(140*a)),Scale(7));
    PushPanelIcon(12);d->AddText(CenterText(Icon(icon),at.x+Scale(14),at.y+Scale(14),true,true),theme.mainColor,Icon(icon));PopFont();
    if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))ImGui::SetTooltip("%s",tip);return hit;
}
// Use the same geometry during travel and on the shelf: changing renderers at
// the endpoint must not introduce a separate perspective/decoration step.
void DrawShelfPreview(ImDrawList* d,Rect r,const ColorTheme& theme,bool panel,float tilt,float decoration,float hover=0){
    const auto pos=r.pos;
    d->AddDrawCmd();const int firstCommand=d->CmdBuffer.Size-1,first=d->VtxBuffer.Size;
    for(int i=5;i>0;--i)d->AddRectFilled(pos-ImVec2(float(i),float(i))+ImVec2(2,4),pos+r.size+ImVec2(float(i),float(i))+ImVec2(2,4),IM_COL32(0,0,0,int(8*decoration)),9.f+i);
    if(panel)W().panelPreview.draw(d,pos,r.size+ImVec2(0,4*hover),theme.windowBgColor);
    else if(!W().chromePreview.empty())W().chromePreview.draw(d,pos,r.size+ImVec2(0,4*hover),theme.windowBgColor);
    else {blur::LayerBackground(d,pos,pos+r.size,theme.windowBgColor,8);browser::DrawPreview(d,pos,r.size+ImVec2(0,4*hover));}
    d->AddRect(pos,pos+r.size,IM_COL32(255,255,255,int(32*decoration)),7.f);
    // Project towards the right. The shelf's upper/lower windows lean towards
    // the display center, as in Stage Manager, instead of a symmetric trapezoid.
    const auto* viewport=ImGui::GetMainViewport();
    const float lean=std::clamp((viewport->WorkPos.y+viewport->WorkSize.y*.5f-pos.y-r.size.y*.5f)/std::max(1.f,viewport->WorkSize.y*.25f),-1.f,1.f)*.18f;
    auto project=[&](ImVec2 p){
        const float x=(p.x-pos.x)/r.size.x,perspective=1.f+.18f*x*tilt;
        return ImVec2(pos.x+(p.x-pos.x)*(1.f+.18f*tilt)/perspective,
            pos.y+r.size.y*.5f+(p.y-pos.y-r.size.y*.5f)/perspective+lean*r.size.x*x*tilt);
    };
    for(int i=first;i<d->VtxBuffer.Size;++i)d->VtxBuffer[i].pos=project(d->VtxBuffer[i].pos);
    for(int i=firstCommand;i<d->CmdBuffer.Size;++i){
        auto& clip=d->CmdBuffer[i].ClipRect;
        const auto& cmd=d->CmdBuffer[i];
        if(!cmd.TexRef._TexData&&blur::BackdropTexture()!=ImTextureID_Invalid&&cmd.TexRef._TexID==blur::BackdropTexture()){
            for(unsigned j=0;j<cmd.ElemCount;++j){
                auto& vertex=d->VtxBuffer[cmd.VtxOffset+d->IdxBuffer[cmd.IdxOffset+j]];
                vertex.uv=blur::BackdropUv(vertex.pos);
            }
        }
        // Full-screen clip bounds can cross the perspective plane while the
        // card is small. Only project the region containing this card.
        const auto clipMin=ImMax(ImVec2(clip.x,clip.y),pos-ImVec2(16,16));
        const auto clipMax=ImMin(ImVec2(clip.z,clip.w),pos+r.size+ImVec2(16,16));
        const auto a=project(clipMin),b=project({clipMax.x,clipMin.y}),c=project({clipMin.x,clipMax.y}),e=project(clipMax);
        const auto low=ImMin(ImMin(a,b),ImMin(c,e)),high=ImMax(ImMax(a,b),ImMax(c,e));clip={low.x,low.y,high.x,high.y};
    }
    d->AddDrawCmd();
    const int iconFirst=d->VtxBuffer.Size;
    const ImVec2 iconCenter=pos+ImVec2(12,r.size.y-13);
    const float iconSize=std::min(42.f,r.size.x*.24f),half=iconSize*.5f;
    d->AddRectFilled(iconCenter-ImVec2(half+1,half-2),iconCenter+ImVec2(half+1,half+3),IM_COL32(0,0,0,65),iconSize*.25f);
    d->AddRectFilled(iconCenter-ImVec2(half,half),iconCenter+ImVec2(half,half),panel?IM_COL32(27,30,43,255):IM_COL32(250,250,252,255),iconSize*.24f);
    if(panel){PushText(23);d->AddText(CenterText("N",iconCenter.x,iconCenter.y,true,true),theme.iconHighlightColor,"N");PopFont();}
    else browser::DrawChromeAppIcon(d,iconCenter,iconSize*.4f);
    for(int i=iconFirst;i<d->VtxBuffer.Size;++i){
        auto& color=d->VtxBuffer[i].col;
        color=(color&~IM_COL32_A_MASK)|(ImU32(float((color&IM_COL32_A_MASK)>>IM_COL32_A_SHIFT)*decoration)<<IM_COL32_A_SHIFT);
    }
}
void Strip(const ColorTheme& theme,bool panel=false){
    const auto r=Shelf(panel);ImGui::SetNextWindowPos(r.pos-ImVec2(12,8));ImGui::SetNextWindowSize(r.size+ImVec2(32,32));
    const bool interactive=clickgui::IsOpen()||CapturesInput();
    ImGui::Begin(panel?"##PanelStageStrip":"##ChromeStageStrip",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|(interactive?0:ImGuiWindowFlags_NoInputs));
    ImGui::SetCursorScreenPos(r.pos-ImVec2(10,0));const bool clicked=ImGui::InvisibleButton("restore",r.size+ImVec2(18,18));
    const float hover=reference::Motion(ImGui::GetItemID(),interactive&&ImGui::IsItemHovered()?1.f:0.f);
    const Rect shown{r.pos+ImVec2(7*hover,-2*hover),r.size};
    if(panel){W().panelShown=shown;W().panelTilt=1-hover;W().panelDecoration=1;}
    else {W().shown=shown;W().tilt=1-hover;W().decoration=1;}
    auto* draws=ImGui::GetWindowDrawList();draws->PushClipRectFullScreen();
    DrawShelfPreview(draws,shown,theme,panel,1-hover,1,hover);
    draws->PopClipRect();
    if(interactive&&ImGui::IsItemHovered())ImGui::SetTooltip("%s - restore window",panel?"ClickGUI":"Chrome");
    if(clicked&&interactive){if(panel)RestorePanel();else Restore();}
    ImGui::End();
}
}
State Current(){return W().state.load();}
bool Detached(){return Current()!=State::Embedded;}
bool CapturesInput(){return W().capture.load()||W().panelCapture.load();}
bool PanelShelved(){return W().panelShelved;}
bool PanelHidden(){return W().panelShelved||W().panelTransition;}
void ShelvePanel(){
    auto& w=W();if(w.panelShelved||!ClientSettings::Get().stageManagerEnabled())return;
    if(auto* panel=ImGui::FindWindowByName("PanelClickGui")){
        w.panelNormal={panel->Pos,panel->Size};
        // A title-bar click occurs before the page has been submitted. Keep
        // the last complete frame, not this frame's half-built draw lists.
        if(w.panelPreview.empty())w.panelPreview.capture(panel);
    }
    else return;
    w.panelPreview.freeze();
    if(!w.panelTransition)w.panelShown=w.panelNormal;
    w.panelShelved=true;w.panelTransition=true;w.panelCapture=true;
    w.panelFrom=w.panelShown;w.panelFromTilt=w.panelTilt;w.panelFromDecoration=w.panelDecoration;w.panelTarget=Shelf(true);w.panelElapsed=0;
    ImGui::ClosePopupsOverWindow(nullptr,false);ImGui::ClearActiveID();
    clickgui::Close();w.wasPanelOpen=false;
}
void RestorePanel(){
    auto& w=W();if(Current()==State::Foreground)Shelve();
    if(w.panelShelved){
        w.panelShelved=false;w.panelTransition=true;w.panelCapture=true;
        w.panelFrom=w.panelShown;w.panelFromTilt=w.panelTilt;w.panelFromDecoration=w.panelDecoration;w.panelTarget=w.panelNormal;w.panelElapsed=0;
    }
    clickgui::OpenPanel();w.wasPanelOpen=true;w.focusPanel=true;
}
void PrepareFrame(){
    auto& w=W();const bool open=clickgui::IsOpen();
    if(!ClientSettings::Get().stageManagerEnabled()){
        const bool restoreBrowser=Current()==State::Foreground&&!open;
        if(Detached()||w.panelShelved||w.panelTransition){Save();Reset();chrome::Focus(false);if(restoreBrowser)clickgui::OpenBrowser();}
        w.wasPanelOpen=clickgui::IsOpen();return;
    }
    // Host key handlers only change the open flag. Perform ImGui transitions on
    // the render thread, also covering Escape and the module's bound shortcut.
    if(!open&&w.wasPanelOpen&&clickgui::GetMode()==clickgui::Mode::Panel&&!w.panelShelved)ShelvePanel();
    if(open&&!w.wasPanelOpen){
        if(Current()==State::Foreground)Shelve();
        if(w.panelShelved&&clickgui::GetMode()==clickgui::Mode::Panel)RestorePanel();
    }
    w.wasPanelOpen=clickgui::IsOpen();
    if(w.focusPanel&&!PanelHidden()&&clickgui::IsOpen()){
        ImGui::SetWindowFocus("PanelClickGui");w.focusPanel=false;
    }
}
void CapturePanel(){
    if(auto* panel=ImGui::FindWindowByName("PanelClickGui")){
        auto& w=W();
        if(!w.panelShelved){w.panelPreview.capture(panel);w.panelNormal={panel->Pos,panel->Size};}
        // A shelf button can close the panel midway through its render pass.
        if(w.panelShelved)for(auto* window:GImGui->Windows)
            if(window->Active&&window->RootWindow==panel)window->DrawList->_ResetForNewFrame();
    }
}
void Detach(){if(!ClientSettings::Get().stageManagerEnabled())return;Initialize();auto& w=W();w.state=State::Foreground;w.capture=true;
    // Preserve the current page texture while moving into its independent frame.
    if(auto* panel=ImGui::FindWindowByName("PanelClickGui")){w.shown={panel->Pos+ImVec2(Scale(168),Scale(10)),{Scale(570),Scale(556)}};w.tilt=w.decoration=0;}
    else {w.shown=Shelf();w.tilt=w.decoration=1;}
    Animate(w.maximized?Maximized():w.normal,.26f);w.focusChrome=true;ShelvePanel();
}
void Restore(){if(!Detached())return;Initialize();auto& w=W();w.focusChrome=true;if(Current()==State::Foreground&&!w.transition){w.capture=true;if(clickgui::IsOpen())ShelvePanel();return;}
    w.state=State::Foreground;w.capture=true;Animate(w.maximized?Maximized():w.normal,.26f);if(clickgui::IsOpen())ShelvePanel();}
void Shelve(){if(!Detached())return;auto& w=W();
    if(Current()==State::Foreground&&!w.transition&&w.chromePreview.empty())if(auto* window=ImGui::FindWindowByName("##ChromeForeground"))w.chromePreview.capture(window);
    if(!w.transition)w.chromePreview.freeze();
    w.state=State::Shelved;Animate(Shelf(),.22f);chrome::Visible(false);Save();}
void Dock(){auto& w=W();Save();w.state=State::Embedded;w.capture=false;w.transition=false;w.popups.reset();chrome::Focus(false);bind::browserCapture=false;clickgui::OpenBrowser();}
bool WindowMessage(UINT message,WPARAM key,LPARAM flags){
    auto& w=W();if(!Detached())return false;
    if(message==WM_KILLFOCUS){w.lostFocus=true;return false;}
    if(key==VK_ESCAPE&&(message==WM_KEYUP||message==WM_SYSKEYUP)&&w.escapeDown.exchange(false))return true;
    if(CapturesInput()&&key==VK_ESCAPE&&(message==WM_KEYDOWN||message==WM_SYSKEYDOWN)){
        if(!(flags&(1LL<<30)))w.escape=true;w.escapeDown=true;return true;
    }
    return false;
}
void Reset(){auto& w=W();w.state=State::Embedded;w.capture=false;w.escape=false;w.escapeDown=false;w.transition=false;w.popups.reset();
    w.shelfFrames[0]=w.shelfFrames[1]=-1;
    w.tilt=w.fromTilt=w.panelTilt=w.panelFromTilt=0;
    w.decoration=w.fromDecoration=w.panelDecoration=w.panelFromDecoration=0;
    w.panelShelved=w.panelTransition=w.wasPanelOpen=w.focusPanel=w.focusChrome=false;w.panelCapture=false;w.panelPreview.clear();w.chromePreview.clear();w.initialized=false;w.lostFocus=false;bind::browserCapture=false;}
void Draw(menu::panel::PanelClickGui& gui){
    auto& w=W();w.panelPreview.collect();w.chromePreview.collect();
    if(!Detached()&&!w.panelShelved&&!w.panelTransition&&!w.panelCapture)return;Initialize();const auto* viewport=ImGui::GetMainViewport();
    const auto display=viewport->WorkSize;
    if(display.x!=w.lastDisplay.x||display.y!=w.lastDisplay.y){
        if(w.lastDisplay.x>0&&w.lastDisplay.y>0){w.normal.pos=viewport->WorkPos+(w.normal.pos-viewport->WorkPos)*ImVec2(display.x/w.lastDisplay.x,display.y/w.lastDisplay.y);}
        w.normal=Bounds(w.normal);w.lastDisplay=display;if(w.transition){w.target=Current()==State::Shelved?Shelf():w.maximized?Maximized():w.normal;}
    }
    if(w.lostFocus.exchange(false)){ImGui::ClearActiveID();if(Current()==State::Foreground)Shelve();}
    if(w.escape.exchange(false)){
        if(ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId|ImGuiPopupFlags_AnyPopupLevel))ImGui::ClosePopupsOverWindow(nullptr,false);
        else if(ImGui::GetActiveID()){ImGui::ClearActiveID();chrome::Focus(false);}
        else if(Current()==State::Foreground)Shelve();
    }
    if(w.transition){w.elapsed+=ImGui::GetIO().DeltaTime;const float t=std::min(1.f,w.elapsed/w.duration),e=1-std::pow(1-t,3.f);
        if(Current()==State::Shelved)w.target=Shelf();
        w.shown={ImLerp(w.from.pos,w.target.pos,e),ImLerp(w.from.size,w.target.size,e)};
        w.tilt=ImLerp(w.fromTilt,Current()==State::Shelved?1.f:0.f,e);
        w.decoration=ImLerp(w.fromDecoration,Current()==State::Shelved?1.f:0.f,e);
        if(t>=1){w.transition=false;if(Current()==State::Foreground)chrome::Visible(true);}
    }
    const auto& theme=gui.colors();reference::SetTheme(&theme);
    const auto oldStyle=ImGui::GetStyle();
    const float scale=std::min(ClientSettings::Get().guiScale(),std::max(.5f,display.x/640.f));SetPanelScale(scale);
    ImGui::GetStyle().ScaleAllSizes(scale);ImGui::GetStyle().FontScaleMain*=scale;PushText(kTextBody);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,Scale(12));
    ImGui::PushStyleColor(ImGuiCol_Text,theme.mainColor);ImGui::PushStyleColor(ImGuiCol_Border,theme.windowBorderColor);
    const bool drewPanelTransition=w.panelTransition;
    if(w.panelTransition){
        w.panelElapsed+=ImGui::GetIO().DeltaTime;
        const float t=std::min(1.f,w.panelElapsed/.24f),e=1-std::pow(1-t,3.f);
        // ClickGUI preserves its dragged position, including off-screen areas.
        // Match that exact rectangle so handing back to the live window cannot jump.
        w.panelTarget=w.panelShelved?Shelf(true):w.panelNormal;
        w.panelShown={ImLerp(w.panelFrom.pos,w.panelTarget.pos,e),ImLerp(w.panelFrom.size,w.panelTarget.size,e)};
        w.panelTilt=ImLerp(w.panelFromTilt,w.panelShelved?1.f:0.f,e);
        w.panelDecoration=ImLerp(w.panelFromDecoration,w.panelShelved?1.f:0.f,e);
        DrawShelfPreview(ImGui::GetForegroundDrawList(),w.panelShown,theme,true,w.panelTilt,w.panelDecoration);
        if(t>=1){w.panelTransition=false;if(!w.panelShelved)gui.resumeFromWorkspace();}
    }
    if(!w.panelTransition&&!ImGui::IsMouseDown(0)&&!ImGui::IsMouseDown(1))w.panelCapture=false;
    if(w.transition){
        DrawShelfPreview(ImGui::GetForegroundDrawList(),w.shown,theme,false,w.tilt,w.decoration);
    }
    else if(Current()==State::Shelved){
        if(!w.escapeDown&&!ImGui::IsMouseDown(0)&&!ImGui::IsMouseDown(1)){w.capture=false;bind::browserCapture=false;}
        Strip(theme);
    }else if(Current()==State::Foreground){
        Rect rect=w.maximized?Maximized():Bounds(w.normal);
        ImGui::SetNextWindowPos(rect.pos);ImGui::SetNextWindowSize(rect.size);
        const auto minSize=ImVec2(std::min(480.f,display.x-16),std::min(320.f,display.y-16));
        ImGui::SetNextWindowSizeConstraints(minSize,display-ImVec2(16,16));
        ImGui::GetIO().ConfigWindowsResizeFromEdges=true;
        if(w.focusChrome){ImGui::SetNextWindowFocus();w.focusChrome=false;}
        ImGui::Begin("##ChromeForeground",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoBackground|(w.maximized?ImGuiWindowFlags_NoResize:0));
        const ImVec2 pos=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();w.shown={pos,size};
        if(!w.maximized&&(size.x!=w.normal.size.x||size.y!=w.normal.size.y||pos.x!=w.normal.pos.x||pos.y!=w.normal.pos.y)){w.normal={pos,size};w.dirty=true;w.saveAt=ImGui::GetTime()+.4;}
        auto* d=ImGui::GetWindowDrawList();blur::LayerBackground(d,pos,pos+size,theme.windowBgColor,Scale(12));d->AddRect(pos,pos+size,theme.windowBorderColor,Scale(12));
        ImGui::SetCursorScreenPos(pos+ImVec2(Scale(8),Scale(2)));
        ImGui::InvisibleButton("window-drag",{std::max(1.f,size.x-Scale(136)),Scale(30)});
        const bool drag=ImGui::IsItemActive();
        if(ImGui::IsItemHovered()&&ImGui::IsMouseDoubleClicked(0)){w.maximized=!w.maximized;ImGui::ClearActiveID();}
        else if(drag&&ImGui::IsMouseDragging(0)){
            if(w.maximized){const float grab=std::clamp((ImGui::GetIO().MousePos.x-pos.x)/size.x,0.f,1.f);w.maximized=false;w.normal.pos=ImGui::GetIO().MousePos-ImVec2(w.normal.size.x*grab,Scale(16));}
            else w.normal.pos+=ImGui::GetIO().MouseDelta;
            w.normal=Bounds(w.normal);w.dirty=true;w.saveAt=ImGui::GetTime()+.4;
        }
        reference::TextY(d,pos.x+Scale(14),pos.y+Scale(2),30,theme.mainColor,"Chrome",14);
        const auto at=pos+ImVec2(size.x-Scale(128),Scale(2));
        if(Button("dock",u8"\uf24d","Return to ClickGUI",theme,at))Dock();
        if(Button("shelve",u8"\uf068","Move to left shelf",theme,at+ImVec2(Scale(30),0)))Shelve();
        if(Button("maximize",u8"\uf2d0",w.maximized?"Restore size":"Maximize",theme,at+ImVec2(Scale(60),0)))w.maximized=!w.maximized;
        if(Button("close",u8"\uf00d","Close window (keep tabs)",theme,at+ImVec2(Scale(90),0))){Dock();clickgui::Close();chrome::Visible(false);}
        SetPopupAnimations(&w.popups);
        if(Current()==State::Foreground&&!w.transition)browser::DrawView(gui,pos+ImVec2(Scale(6),Scale(36)),size-ImVec2(Scale(12),Scale(42)),true);
        w.popups.drawClosing();SetPopupAnimations(nullptr);
        if(Current()==State::Foreground&&!w.transition&&ImGui::IsMouseClicked(0)&&
           !(w.panelShelved&&ImGui::IsMouseHoveringRect(Shelf(true).pos-ImVec2(12,8),Shelf(true).pos+Shelf(true).size+ImVec2(20,24),false))&&
           !ImGui::IsMouseHoveringRect(pos,pos+size,false)&&!ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId|ImGuiPopupFlags_AnyPopupLevel))Shelve();
        if(Current()==State::Foreground&&!w.transition)w.chromePreview.capture(ImGui::GetCurrentWindow());
        ImGui::End();
        if(w.dirty&&!ImGui::IsMouseDown(0)&&ImGui::GetTime()>w.saveAt)Save();
    }
    if(w.panelShelved&&!w.panelTransition&&!drewPanelTransition)Strip(theme,true);
    ImGui::PopStyleColor(2);ImGui::PopStyleVar(2);PopFont();ImGui::GetStyle()=oldStyle;
    SetPanelScale(1.f);reference::SetTheme(nullptr);
    if(Detached())bind::browserCapture=CapturesInput();
}
void ReleaseTextures(){Reset();W().panelPreview.release();W().chromePreview.release();}
}
