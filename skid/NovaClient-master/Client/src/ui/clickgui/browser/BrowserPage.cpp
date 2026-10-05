#include "ui/clickgui/browser/BrowserPage.h"
#include "ui/clickgui/browser/BrowserBackdrop.h"
#include "ui/workspace/ForegroundWorkspace.h"
#include "bind/BrowserCapture.h"
#include "chrome/BrowserService.h"
#include "config/BrowserSettings.h"
#include "core/Instance.h"
#include "ui/clickgui/panel/PanelClickGui.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/clickgui/panel/Widgets.h"
#include "ui/clickgui/panel/ReferenceStyle.h"
#include "ui/clickgui/panel/PropertyComponent.h"
#include "ui/clickgui/Fonts.h"
#include "imgui_internal.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace nova::ui::browser {
namespace {
using namespace menu::panel;
struct Texture {std::unique_ptr<ImTextureData> data;uint64_t sequence{};int retired=-1;};
struct Page {
    std::vector<Texture> textures;
    std::vector<Texture> backdropTextures;
    BrowserBackdrop backdrop;
    ImVec2 webMin{},webMax{};
    char address[4096]{};
    bool initialized=false,ready=false,addressActive=false,hovered=false,focusAddress=false;
    bool entered=false,dirty=false;
    Properties::StringProperty home;
    Properties::NumberProp fps{15,60,1,30,"{:.0f} FPS"};
    Properties::NumberProp zoom{50,200,1,100,"{:.0f}%"};
    Properties::BoolProperty audio{false};
    std::vector<std::unique_ptr<PropertyComponent>> settings;
};
Page& P(){return Instance<Page>();}
void Op(chrome::Op op,int a=0){chrome::Command c;c.op=op;c.a=a;chrome::Send(c);}
void Save(){auto& p=P();config::SaveBrowserSettings({p.home.get(),int(p.fps.get()),int(p.zoom.get()),p.audio.get()});p.dirty=false;}
void Collect(std::vector<Texture>& v){for(auto it=v.begin();it!=v.end();){auto& t=*it->data;if(it->retired>=0){t.WantDestroyNextFrame=true;t.SetStatus(ImTextureStatus_WantDestroy);t.UnusedFrames=ImGui::GetFrameCount()-it->retired;if(t.TexID==ImTextureID_Invalid&&!t.BackendUserData){ImGui::UnregisterUserTexture(&t);it=v.erase(it);continue;}}++it;}}
ImTextureData* TextureFor(const chrome::Frame& frame,std::vector<Texture>& v) {
    Collect(v);
    auto it=std::find_if(v.begin(),v.end(),[](auto& t){return t.retired<0;});
    if(it!=v.end()&&(it->data->Width!=frame.width||it->data->Height!=frame.height)){it->retired=ImGui::GetFrameCount();it->data->WantDestroyNextFrame=true;it->data->SetStatus(ImTextureStatus_WantDestroy);it=v.end();}
    if(it==v.end()){
        if(v.size()>=8)return nullptr;
        Texture t;t.data=std::make_unique<ImTextureData>();t.data->Create(ImTextureFormat_RGBA32,frame.width,frame.height);ImGui::RegisterUserTexture(t.data.get());v.push_back(std::move(t));it=std::prev(v.end());
    }
    auto& tex=*it->data;
    if(it->sequence!=frame.sequence){memcpy(tex.GetPixels(),frame.rgba.data(),frame.rgba.size());it->sequence=frame.sequence;if(tex.Status!=ImTextureStatus_WantCreate){tex.Updates.clear();tex.UpdateRect={0,0,(unsigned short)frame.width,(unsigned short)frame.height};tex.Updates.push_back(tex.UpdateRect);tex.SetStatus(ImTextureStatus_WantUpdates);}}
    return &tex;
}
bool IconButton(const char* id,const char8_t* icon,const ColorTheme& t,bool enabled=true,const char* tooltip=nullptr,float extent=28.f) {
    const auto pos=ImGui::GetCursorScreenPos();ImGui::PushID(id);ImGui::BeginDisabled(!enabled);
    const ImVec2 bounds(Scale(extent),Scale(extent));
    const bool clicked=ImGui::InvisibleButton("button",bounds);
    const bool hovered=ImGui::IsItemHovered();
    const float blend=reference::Motion(ImGui::GetItemID(),hovered?1.f:0.f);
    auto* d=ImGui::GetWindowDrawList();d->AddRectFilled(pos,pos+bounds,color::ReAlpha(t.buttonHoveredBgColor,int(blend*color::Alpha(t.buttonHoveredBgColor))),Scale(extent*.5f));
    PushPanelIcon(12);const auto glyph=Icon(icon);const auto size=ImGui::CalcTextSize(glyph);
    const ImU32 ink=enabled?reference::Mix(t.mainColor,t.textHighlightColor,blend):color::ReAlpha(t.secondaryColor,80);
    d->AddText(pos+(bounds-size)*.5f,ink,glyph);PopFont();
    if(tooltip&&hovered&&ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))ImGui::SetTooltip("%s",tooltip);
    ImGui::EndDisabled();ImGui::PopID();return clicked;
}
bool AddressField(const ColorTheme& t,float width,bool enabled) {
    auto& p=P();const auto pos=ImGui::GetCursorScreenPos();auto* d=ImGui::GetWindowDrawList();
    const float height=Scale(30);
    const ImGuiID id=ImGui::GetID("##address");
    const float response=reference::Motion(id^0xf31u,enabled&&(p.addressActive||ImGui::IsMouseHoveringRect(pos,pos+ImVec2(width,height)))?1.f:0.f);
    d->AddRectFilled(pos,pos+ImVec2(width,height),color::ReAlpha(t.windowBgColor,255),height*.5f);
    d->AddRectFilled(pos,pos+ImVec2(width,height),color::ReAlpha(t.buttonHoveredBgColor,int(color::Alpha(t.buttonHoveredBgColor)*response*.45f)),height*.5f);
    PushPanelIcon(11);d->AddText(CenterText(Icon(u8"\uf0ac"),pos.x+Scale(16),pos.y+height*.5f,true,true),t.secondaryColor,Icon(u8"\uf0ac"));PopFont();
    PushText(kTextControl);
    // Match the search field's optical centering, including non-Latin URLs.
    auto* baked=ImGui::GetFontBaked();float top=TextHeight(),bottom=0;
    const char* hint="Search or enter address";
    for(const char* at=p.address[0]?p.address:hint;*at;){unsigned int cp=0;const int bytes=ImTextCharFromUtf8(&cp,at,nullptr);if(bytes<=0)break;at+=bytes;
        const auto* glyph=baked->FindGlyph(cp);if(!glyph||!glyph->Visible)continue;
        const float scale=ImGui::GetFontSize()/baked->Size;top=std::min(top,glyph->Y0*scale);bottom=std::max(bottom,glyph->Y1*scale);}
    const float padding=std::max(0.f,(height-TextHeight())*.5f);
    const float offset=bottom>top?(height-top-bottom)*.5f-padding:0;
    ImGui::PushClipRect(pos,pos+ImVec2(width,height),true);
    ImGui::SetCursorScreenPos({pos.x+Scale(28),std::round(pos.y+offset)});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{Scale(10),padding});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,0u);ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,0u);ImGui::PushStyleColor(ImGuiCol_FrameBgActive,0u);
    ImGui::PushStyleColor(ImGuiCol_Text,t.mainColor);ImGui::PushStyleColor(ImGuiCol_TextDisabled,t.secondaryColor);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg,color::ReAlpha(t.iconHighlightColor,reference::kFieldSelectionAlpha));ImGui::PushStyleColor(ImGuiCol_NavCursor,0u);
    ImGui::BeginDisabled(!enabled);ImGui::SetNextItemWidth(width-Scale(36));
    if(p.focusAddress&&enabled){ImGui::SetKeyboardFocusHere();p.focusAddress=false;}
    const bool submit=ImGui::InputTextWithHint("##address",hint,p.address,sizeof(p.address),ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_AutoSelectAll);
    p.addressActive=ImGui::IsItemActive();if(p.addressActive)chrome::Focus(false);
    ImGui::EndDisabled();ImGui::PopStyleColor(7);ImGui::PopStyleVar(2);ImGui::PopClipRect();PopFont();
    d->AddRect(pos,pos+ImVec2(width,height),t.splitColor,height*.5f);
    return submit;
}
void Tabs(const chrome::Snapshot& state,const ColorTheme& t,ImVec2 pos,float width) {
    const int count=std::clamp(state.status.count,0,chrome::MaxTabs);
    const float tabWidth=std::min(Scale(184),(width-Scale(34)-Scale(4)*std::max(0,count-1))/std::max(1,count));
    for(int i=0;i<count;++i){
        const auto& tab=state.status.tabs[i];const bool selected=tab.id==state.status.selected;
        ImGui::PushID(tab.id);const ImVec2 p=pos+ImVec2(i*(tabWidth+Scale(4)),0);
        ImGui::SetCursorScreenPos(p);
        if(ImGui::InvisibleButton("tab",{tabWidth-Scale(27),Scale(34)})){Op(chrome::Op::SelectTab,tab.id);chrome::Focus(false);}
        const bool hovered=ImGui::IsItemHovered();
        if(ImGui::IsItemClicked(ImGuiMouseButton_Middle))Op(chrome::Op::CloseTab,tab.id);
        const float blend=reference::Motion(ImGui::GetItemID(),selected?1.f:hovered?.45f:0.f);
        auto* d=ImGui::GetWindowDrawList();
        const ImU32 surface=reference::Mix(t.modulePanelBgColor,t.categoryHighlightColor,.65f);
        d->AddRectFilled(p,p+ImVec2(tabWidth,Scale(34)),color::ReAlpha(surface,int(255*blend)),Scale(10),ImDrawFlags_RoundCornersTop);
        if(!selected&&i+1<count)d->AddLine(p+ImVec2(tabWidth,Scale(9)),p+ImVec2(tabWidth,Scale(25)),t.splitColor);
        PushPanelIcon(11);d->AddText(CenterText(Icon(u8"\uf0ac"),p.x+Scale(15),p.y+Scale(17),true,true),selected?t.mainColor:t.secondaryColor,Icon(u8"\uf0ac"));PopFont();
        const auto title=chrome::Utf8(tab.title);const char* label=title.empty()?"New tab":title.c_str();
        PushText(kTextSmall);const auto textMin=p+ImVec2(Scale(29),(Scale(34)-TextHeight())*.5f);
        ImGui::PushStyleColor(ImGuiCol_Text,selected?t.mainColor:t.secondaryColor);
        ImGui::RenderTextEllipsis(d,textMin,p+ImVec2(tabWidth-Scale(29),Scale(30)),tabWidth+p.x-Scale(29),label,nullptr,nullptr);
        ImGui::PopStyleColor();PopFont();
        if(hovered&&ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))ImGui::SetTooltip("%s",label);
        ImGui::SetCursorScreenPos(p+ImVec2(tabWidth-Scale(26),Scale(5)));
        if(IconButton("close-tab",u8"\uf00d",t,true,"Close tab",24))Op(chrome::Op::CloseTab,tab.id);
        ImGui::PopID();
    }
    ImGui::SetCursorScreenPos(pos+ImVec2(count*(tabWidth+Scale(4)),Scale(3)));
    if(IconButton("new-tab",u8"\uf067",t,state.status.ready&&count<chrome::MaxTabs,"New tab"))Op(chrome::Op::NewTab);
}
void Settings(PanelClickGui& gui,ImVec2 anchor) {
    auto& p=P();
    ImGui::SetNextWindowSize({Scale(360),Scale(196)});
    ImGui::SetNextWindowPos(anchor,ImGuiCond_Appearing,{1,0});
    if(!BeginPanelPopup(gui.colors(),"browser-settings",ImGuiWindowFlags_NoScrollbar,PopupMotion::Dropdown,{12,8}))return;
    chrome::Focus(false);
    PushText(kTextSmall);ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(gui.colors().secondaryColor),"Chrome settings");PopFont();
    const auto home=p.home.get();const int fps=int(p.fps.get()),zoom=int(p.zoom.get());const bool audio=p.audio.get();
    for(auto& row:p.settings)row->render();
    if(fps!=int(p.fps.get())){Op(chrome::Op::Fps,int(p.fps.get()));p.dirty=true;}
    if(zoom!=int(p.zoom.get())){Op(chrome::Op::Zoom,int(p.zoom.get()));p.dirty=true;}
    if(audio!=p.audio.get()){Op(chrome::Op::BackgroundAudio,p.audio.get());p.dirty=true;}
    p.dirty|=home!=p.home.get();
    EndPanelPopup();
}
void Mouse(ImVec2 pos,ImVec2 size,int w,int h) {
    auto& p=P();auto& io=ImGui::GetIO();const bool hover=ImGui::IsItemHovered()&&!ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId|ImGuiPopupFlags_AnyPopupLevel);
    if(ImGui::IsMouseClicked(0)&&!hover)chrome::Focus(false);
    if(hover&&ImGui::IsMouseClicked(0))chrome::Focus(true);
    chrome::InputRegion(pos.x,pos.y,size.x,size.y,w,h);
    chrome::Command c;c.a=int((io.MousePos.x-pos.x)*w/size.x);c.b=int((io.MousePos.y-pos.y)*h/size.y);
    c.modifiers=((GetAsyncKeyState(VK_SHIFT)&0x8000)?2:0)|((GetAsyncKeyState(VK_CONTROL)&0x8000)?4:0)|((GetAsyncKeyState(VK_MENU)&0x8000)?8:0)|(io.MouseDown[0]?16:0)|(io.MouseDown[1]?64:0)|(io.MouseDown[2]?32:0);
    if(hover||p.hovered||chrome::Focused()) {c.op=chrome::Op::MouseMove;c.c=!hover;chrome::Send(c);}
    for(int i=0;i<3;++i)if((hover&&ImGui::IsMouseClicked(i))||(chrome::Focused()&&ImGui::IsMouseReleased(i))){c.op=chrome::Op::MouseButton;c.c=i==1?2:i==2?1:0;c.d=(ImGui::IsMouseReleased(i)?1:0)|(ImGui::IsMouseDoubleClicked(i)?2:0);chrome::Send(c);}
    if(hover&&(io.MouseWheel||io.MouseWheelH)){c.op=chrome::Op::Wheel;c.c=int(io.MouseWheelH*120);c.d=int(io.MouseWheel*120);chrome::Send(c);}
    if(hover&&ImGui::IsMouseClicked(3))Op(chrome::Op::Back);
    if(hover&&ImGui::IsMouseClicked(4))Op(chrome::Op::Forward);
    if(hover&&ImGui::IsMouseReleased(1))ImGui::OpenPopup("web-context");
    p.hovered=hover;
}
}
void Draw(menu::panel::PanelClickGui& gui,ImVec2 origin) {
    if(workspace::Detached())return;
    DrawView(gui,origin+ImVec2(Scale(168),Scale(10)),{Scale(570),Scale(556)},false);
}
void DrawView(menu::panel::PanelClickGui& gui,ImVec2 position,ImVec2 viewSize,bool detached) {
    bind::browserCapture.store(true);
    auto& p=P();const auto& t=gui.colors();
    p.focusAddress|=chrome::TakeAddressFocus();
    if(!p.initialized){
        const auto settings=config::LoadBrowserSettings();p.home.set(settings.home);p.fps.set(settings.fps);p.zoom.set(settings.zoom);p.audio.set(settings.audio);
        for(const auto& row:{Properties::PropDesc("Home",&p.home),Properties::PropDesc("Browser FPS",&p.fps),Properties::PropDesc("Page zoom",&p.zoom),Properties::PropDesc("Background audio",&p.audio)})
            p.settings.push_back(std::make_unique<PropertyComponent>(gui,row));
        p.initialized=true;
    }
    auto state=chrome::Read();
    if(!p.entered){
        p.entered=true;
        if(!state.running&&!state.starting){chrome::Start();chrome::Navigate(chrome::Wide(p.home.get()));state=chrome::Read();}
    }
    ImGui::SetCursorScreenPos(position);
    ImGui::BeginChild("browser-page",viewSize,ImGuiChildFlags_None,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    PushText(kTextControl);
    ImGui::PushStyleColor(ImGuiCol_Text,t.mainColor);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{Scale(6),Scale(6)});
    const auto base=ImGui::GetCursorScreenPos();const float width=ImGui::GetContentRegionAvail().x;
    auto* d=ImGui::GetWindowDrawList();
    d->AddRectFilled(base,base+ImVec2(width,Scale(82)),color::ReAlpha(t.modulePanelBgColor,255),Scale(10));
    const ImU32 toolbarColor=color::ReAlpha(reference::Mix(t.modulePanelBgColor,t.categoryHighlightColor,.65f),255);
    d->AddRectFilled(base+ImVec2(0,Scale(40)),base+ImVec2(width,Scale(82)),toolbarColor,Scale(10),ImDrawFlags_RoundCornersBottom);
    // Popups above a webpage need its pixels, not only the game's blur cache.
    if(state.frame&&blur::Available()&&ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId|ImGuiPopupFlags_AnyPopupLevel)){
        const auto& blurred=p.backdrop.Update(*state.frame,blur::GetOptions());
        if(auto* texture=TextureFor(blurred,p.backdropTextures))blur::SetOverlayBackdrop(texture->GetTexRef(),p.webMin,p.webMax);
    }
    ImGui::SetCursorScreenPos(base+ImVec2(Scale(6),Scale(7)));
    if(IconButton("tab-list",u8"\uf078",t,state.status.ready,"Tabs")){chrome::Focus(false);ImGui::OpenPopup("tab-list");}
    if(BeginPanelPopup(t,"tab-list")){
        for(int i=0;i<std::clamp(state.status.count,0,chrome::MaxTabs);++i){const auto& tab=state.status.tabs[i];const auto title=chrome::Utf8(tab.title);ImGui::PushID(tab.id);
            if(Choice(t,"tab",title.empty()?"New tab":title.c_str(),tab.id==state.status.selected,Scale(240))){Op(chrome::Op::SelectTab,tab.id);ImGui::CloseCurrentPopup();}ImGui::PopID();}
        EndPanelPopup();
    }
    Tabs(state,t,base+ImVec2(Scale(40),Scale(6)),width-Scale(detached?48:86));
    if(!detached){
        ImGui::SetCursorScreenPos(base+ImVec2(width-Scale(36),Scale(7)));
        if(ClientSettings::Get().stageManagerEnabled()&&IconButton("foreground-workspace",u8"\uf24d",t,true,"Move to foreground workspace"))workspace::Detach();
    }
    const auto toolbar=base+ImVec2(Scale(8),Scale(44));
    auto button=[&](float x,const char* id,const char8_t* icon,bool enabled,const char* tooltip){ImGui::SetCursorScreenPos(toolbar+ImVec2(Scale(x),Scale(1)));return IconButton(id,icon,t,enabled,tooltip);};
    if(button(0,"back",u8"\uf060",state.status.ready&&state.status.back,"Back"))Op(chrome::Op::Back);
    if(button(30,"forward",u8"\uf061",state.status.ready&&state.status.forward,"Forward"))Op(chrome::Op::Forward);
    if(button(60,"reload",u8"\uf021",state.status.ready,"Reload"))Op(chrome::Op::Reload);
    if(!p.addressActive&&!p.focusAddress&&state.status.ready)strncpy_s(p.address,chrome::Utf8(state.status.url).c_str(),_TRUNCATE);
    ImGui::SetCursorScreenPos(toolbar+ImVec2(Scale(94),0));
    if(AddressField(t,width-Scale(174),state.status.ready)){chrome::Navigate(chrome::Wide(p.address));ImGui::ClearActiveID();}
    ImGui::SetCursorScreenPos({base.x+width-Scale(68),toolbar.y+Scale(1)});
    if(IconButton("browser-settings",u8"\uf142",t,true,"Browser settings")){chrome::Focus(false);ImGui::OpenPopup("browser-settings");}
    Settings(gui,{base.x+width,base.y+Scale(88)});
    ImGui::SetCursorScreenPos({base.x+width-Scale(36),toolbar.y+Scale(1)});
    const bool active=state.running||state.starting;
    if(IconButton("browser-power",u8"\uf011",t,true,active?"Stop browser":"Restart browser")){
        chrome::Focus(false);p.ready=false;
        if(active)chrome::Stop();else{chrome::Start();chrome::Navigate(chrome::Wide(p.home.get()));}
    }
    if(p.dirty&&!ImGui::IsAnyItemActive())Save();
    ImGui::SetCursorScreenPos(base+ImVec2(0,Scale(90)));
    if(!state.status.ready){
        p.ready=false;p.hovered=false;chrome::Focus(false);
        const auto pos=ImGui::GetCursorScreenPos();const auto size=ImGui::GetContentRegionAvail();
        d->AddRectFilled(pos,pos+size,color::ReAlpha(t.modulePanelBgColor,230),Scale(10));
        ImGui::SetCursorScreenPos(pos+ImVec2(Scale(18),Scale(20)));
        ImGui::TextUnformatted(active?"Starting Chrome...":"Browser stopped");
        if(!state.error.empty()){ImGui::SetCursorScreenPos(pos+ImVec2(Scale(18),Scale(50)));ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+size.x-Scale(36));ImGui::TextWrapped("%s",state.error.c_str());ImGui::PopTextWrapPos();}
        ImGui::SetCursorScreenPos(pos+size);ImGui::Dummy({0,0});
    } else {
        chrome::Visible(true);
        if(!p.ready){p.ready=true;Op(chrome::Op::Fps,int(p.fps.get()));Op(chrome::Op::Zoom,int(p.zoom.get()));Op(chrome::Op::BackgroundAudio,p.audio.get());}
        if(state.status.error[0])ImGui::TextWrapped("%s",chrome::Utf8(state.status.error).c_str());
        if(!state.error.empty())ImGui::TextWrapped("%s",state.error.c_str());
        auto pos=ImGui::GetCursorScreenPos();auto size=ImGui::GetContentRegionAvail();size.y=std::max(1.f,size.y);size.x=std::max(1.f,size.x);
        p.webMin=pos;p.webMax=pos+size;
        const float ratio=std::min({1.f,float(chrome::MaxWidth)/size.x,float(chrome::MaxHeight)/size.y});int w=std::max(1,int(size.x*ratio)),h=std::max(1,int(size.y*ratio));chrome::Viewport(w,h);
        ImGui::InvisibleButton("web-view",size,ImGuiButtonFlags_MouseButtonLeft|ImGuiButtonFlags_MouseButtonRight|ImGuiButtonFlags_MouseButtonMiddle);
        auto* draw=ImGui::GetWindowDrawList();draw->AddRectFilled(pos,pos+size,t.modulePanelBgColor,Scale(5));
        if(state.frame)if(auto* texture=TextureFor(*state.frame,p.textures))draw->AddImageRounded(texture->GetTexRef(),pos,pos+size,{0,0},{1,1},IM_COL32_WHITE,Scale(5));
        if(state.status.loading)draw->AddCircleFilled(pos+ImVec2(size.x-Scale(10),Scale(10)),Scale(3),t.iconHighlightColor);
        if(!(GImGui->CurrentItemFlags&ImGuiItemFlags_Disabled))Mouse(pos,size,state.frame?state.frame->width:w,state.frame?state.frame->height:h);
        if(p.hovered)ImGui::SetMouseCursor(state.status.cursor==1?ImGuiMouseCursor_TextInput:state.status.cursor==2?ImGuiMouseCursor_Hand:ImGuiMouseCursor_Arrow);
        if(BeginPanelPopup(t,"web-context")){
            chrome::Focus(false);
            auto action=[&](const char* id,const char* name,chrome::Op op,int value=0){if(Choice(t,id,name,false,Scale(180))){Op(op,value);ImGui::CloseCurrentPopup();}};
            action("back","Back",chrome::Op::Back);action("reload","Reload",chrome::Op::Reload);
            SplitLine(t);action("copy","Copy",chrome::Op::Edit,0);action("paste","Paste",chrome::Op::Edit,1);action("select","Select all",chrome::Op::Edit,2);
            EndPanelPopup();
        }
    }
    blur::ClearOverlayBackdrop();
    ImGui::PopStyleVar();ImGui::PopStyleColor();PopFont();ImGui::EndChild();
}
void DrawPreview(ImDrawList* d,ImVec2 pos,ImVec2 size){
    auto& p=P();d->AddRectFilled(pos,pos+size,IM_COL32(22,25,34,255),8);
    const float bar=std::min(24.f,size.y*.15f);
    for(auto& tex:p.textures)if(tex.retired<0){d->AddImageRounded(tex.data->GetTexRef(),pos+ImVec2(3,bar),pos+size-ImVec2(3,3),{0,0},{1,1},IM_COL32_WHITE,5);break;}
    const float r=std::min(2.f,bar*.12f);
    for(int i=0;i<3;++i)d->AddCircleFilled(pos+ImVec2(8+i*7,bar*.5f),r,IM_COL32(124,134,155,255));
    d->AddRect(pos,pos+size,IM_COL32(76,83,102,200),8);
}
void Hide(){if(workspace::Detached())return;bind::browserCapture.store(false);chrome::Visible(false);auto& p=P();p.hovered=false;p.entered=false;p.addressActive=false;if(p.dirty)Save();Collect(p.textures);Collect(p.backdropTextures);}
void ReleaseTextures(){workspace::ReleaseTextures();blur::ClearOverlayBackdrop();for(auto* textures:{&P().textures,&P().backdropTextures}){for(auto& t:*textures)ImGui::UnregisterUserTexture(t.data.get());textures->clear();}}
}
