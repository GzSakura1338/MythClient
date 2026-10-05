#include "ui/clickgui/proxy/ProxyPage.h"
#include "proxy/ProxyService.h"
#include "ui/clickgui/panel/PanelClickGui.h"
#include "ui/clickgui/panel/Widgets.h"
#include "ui/clickgui/panel/ReferenceStyle.h"
#include "core/Instance.h"
#include "imgui_internal.h"
#include <array>
#include <cstring>

namespace nova::ui::proxy {
using namespace menu::panel;
namespace {
struct State {
    std::array<char,256> host{},username{},password{};
    int port=1080,protocol=1;
    bool enabled=false,auth=false,dirty=false;
    std::uint64_t revision=~0ull;
    std::string error;
    ImGuiID passwordId=0;
};
State& Data(){return Instance<State>();}
void Copy(auto& buffer,const std::string& text){buffer.fill(0);std::memcpy(buffer.data(),text.data(),std::min(buffer.size()-1,text.size()));}
bool Button(const ColorTheme& t,const char* id,const char* label,float width){
    const auto p=ImGui::GetCursorScreenPos();const ImVec2 size{width,Scale(30)};
    const bool clicked=ImGui::InvisibleButton(id,size);
    const float hover=reference::Motion(ImGui::GetItemID()^0x4511u,ImGui::IsItemHovered()?1.f:0.f);
    auto* draw=ImGui::GetWindowDrawList();draw->AddRectFilled(p,p+size,ImGui::GetColorU32(reference::Mix(t.selectorBgColor,t.buttonHoveredBgColor,hover*.45f)),Scale(5));
    PushText(kTextControl);draw->AddText(CenterText(label,p.x+width*.5f,p.y+size.y*.5f,true,true),ImGui::GetColorU32(t.mainColor),label);PopFont();return clicked;
}
void Toggle(const ColorTheme& t,const char* label,int id,bool& value){
    const auto p=ImGui::GetCursorScreenPos();PushText(kTextControl);
    ImGui::GetWindowDrawList()->AddText(CenterText(label,p.x,p.y+Scale(15),false,true),ImGui::GetColorU32(t.mainColor),label);PopFont();
    reference::Toggle(ImGui::GetWindowDrawList(),id,p+ImVec2(ImGui::GetContentRegionAvail().x-Scale(31),Scale(15)),value);
    ImGui::SetCursorScreenPos(p);ImGui::Dummy({0,Scale(32)});
}
void InputStyle(const ColorTheme& t){
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,Scale(5));ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,Scale(1));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{Scale(9),Scale(6)});
    ImGui::PushStyleColor(ImGuiCol_FrameBg,t.selectorBgColor);ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,reference::Mix(t.selectorBgColor,t.buttonHoveredBgColor,.25f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,reference::Mix(t.selectorBgColor,t.buttonHoveredBgColor,.3f));
    ImGui::PushStyleColor(ImGuiCol_Border,t.splitColor);ImGui::PushStyleColor(ImGuiCol_TextSelectedBg,ImGui::GetColorU32(t.iconHighlightColor,.18f));
    PushText(kTextControl);ImGui::PushItemWidth(-1);
}
void EndInputStyle(){ImGui::PopItemWidth();PopFont();ImGui::PopStyleColor(5);ImGui::PopStyleVar(3);}
void Label(const ColorTheme& t,const char* label){PushText(kTextSmall);ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(t.secondaryColor),"%s",label);PopFont();}
}
void Hide(){
    auto& s=Data();if(s.revision==~0ull)return;
    std::fill(s.password.begin(),s.password.end(),0);s.revision=~0ull;
    if(ImGui::GetCurrentContext()&&s.passwordId&&GImGui->InputTextState.ID==s.passwordId){
        auto& text=GImGui->InputTextState;std::fill(text.TextA.begin(),text.TextA.end(),0);
        std::fill(text.TextToRevertTo.begin(),text.TextToRevertTo.end(),0);std::fill(text.CallbackTextBackup.begin(),text.CallbackTextBackup.end(),0);
        if(GImGui->ActiveId==s.passwordId)ImGui::ClearActiveID();text.ClearFreeMemory();text.ID=0;text.TextLen=0;text.TextSrc=nullptr;
    }s.passwordId=0;
}
void Draw(PanelClickGui& gui,ImVec2 origin){
    const auto snapshot=nova::proxy::ReadStatus();auto& s=Data();const auto& t=gui.colors();
    if(s.revision!=snapshot.revision){const auto c=nova::proxy::ReadConfig();Copy(s.host,c.host);Copy(s.username,c.username);Copy(s.password,c.password);
        s.port=c.port;s.protocol=c.protocol==nova::proxy::Protocol::Http?0:1;s.enabled=c.enabled;s.auth=c.authentication;s.dirty=false;s.revision=snapshot.revision;
    }
    ImGui::SetCursorScreenPos(origin+ImVec2(Scale(174),Scale(70)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});ImGui::PushStyleColor(ImGuiCol_Text,t.mainColor);
    ImGui::BeginChild("proxy-page",{Scale(558),Scale(490)},ImGuiChildFlags_None,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar);
    PushText(kTextTitle);ImGui::TextUnformatted("Proxy");PopFont();ImGui::Dummy({0,Scale(8)});
    const bool enabled=s.enabled;Toggle(t,"Enabled",71,s.enabled);s.dirty|=enabled!=s.enabled;
    if(!snapshot.supported){Label(t,"Not available on this client version");}
    InputStyle(t);Label(t,"Protocol");
    const char* choices[]{"HTTP (CONNECT)","SOCKS5"};
    const auto dropdown=ImGui::GetCursorScreenPos();const float dropdownWidth=ImGui::GetContentRegionAvail().x;
    if(reference::Select(ImGui::GetWindowDrawList(),73,dropdown,dropdownWidth,choices[s.protocol]))ImGui::OpenPopup("proxy-protocol");
    ImGui::SetCursorScreenPos(dropdown);ImGui::Dummy({dropdownWidth,Scale(28)});
    ImGui::SetNextWindowPos(dropdown+ImVec2(0,Scale(29)));ImGui::SetNextWindowSize({dropdownWidth,0});
    if(BeginPanelPopup(t,"proxy-protocol")){for(int i=0;i<2;++i)if(Choice(t,choices[i],choices[i],s.protocol==i,dropdownWidth-Scale(8))){s.protocol=i;s.dirty=true;ImGui::CloseCurrentPopup();}EndPanelPopup();}
    Label(t,"Host");s.dirty|=ImGui::InputText("##proxy-host",s.host.data(),s.host.size());
    Label(t,"Port");s.dirty|=ImGui::InputInt("##proxy-port",&s.port,0,0);EndInputStyle();
    const bool auth=s.auth;Toggle(t,"Authentication",72,s.auth);s.dirty|=s.auth!=auth;
    if(s.auth){InputStyle(t);Label(t,"Username");s.dirty|=ImGui::InputText("##proxy-user",s.username.data(),s.username.size());
        Label(t,"Password");s.dirty|=ImGui::InputText("##proxy-password",s.password.data(),s.password.size(),ImGuiInputTextFlags_Password);
        s.passwordId=ImGui::GetItemID();EndInputStyle();}
    ImGui::Dummy({0,Scale(4)});
    if(Button(t,"proxy-apply",s.dirty?"Apply changes":"Apply",Scale(125))){
        nova::proxy::Config c;c.enabled=s.enabled;c.authentication=s.auth;c.host=s.host.data();c.port=s.port;
        c.protocol=s.protocol==0?nova::proxy::Protocol::Http:nova::proxy::Protocol::Socks5;c.username=s.username.data();c.password=s.password.data();
        if(c.enabled&&!snapshot.supported)s.error="This client does not support proxy connections";
        else if(nova::proxy::Apply(std::move(c),s.error))s.dirty=false;
    }
    if(!s.error.empty())Label(t,s.error.c_str());
    reference::PageReveal(gui.pageReveal());ImGui::EndChild();ImGui::PopStyleColor();ImGui::PopStyleVar();
}
}
