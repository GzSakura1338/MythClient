#include "ui/clickgui/accounts/AccountPage.h"
#include "contracts/AccountService.h"
#include "core/Instance.h"
#include "ui/clickgui/panel/PanelClickGui.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/clickgui/panel/Widgets.h"
#include "ui/clickgui/panel/ReferenceStyle.h"
#include "ui/overlay/Portrait.h"
#include "imgui_internal.h"
#include <algorithm>
#include <array>
#include <cctype>

namespace nova::ui::accounts {
using namespace menu::panel;
namespace {
struct State { std::array<char,8193> input{}; std::string removeId; int dialog=0; bool focus=false,openPopup=false; ImGuiID inputId=0; };
State& Data() { return Instance<State>(); }
void Clear() {
    auto& s=Data();std::fill(s.input.begin(),s.input.end(),0);
    if(ImGui::GetCurrentContext() && s.inputId && GImGui->InputTextState.ID==s.inputId) {
        auto& text=GImGui->InputTextState;
        std::fill(text.TextA.begin(),text.TextA.end(),0);
        std::fill(text.TextToRevertTo.begin(),text.TextToRevertTo.end(),0);
        std::fill(text.CallbackTextBackup.begin(),text.CallbackTextBackup.end(),0);
        if(GImGui->ActiveId==s.inputId)ImGui::ClearActiveID();
        text.ClearFreeMemory();
        text.ID=0;text.TextLen=0;text.TextSrc=nullptr;
    }
    s.dialog=0;s.focus=false;s.openPopup=false;s.inputId=0;s.removeId.clear();
}
bool Button(const ColorTheme& t,const char* id,const char* label,ImVec2 size,bool trash=false) {
    const auto p=ImGui::GetCursorScreenPos(); const bool click=ImGui::InvisibleButton(id,size);
    const float hover=reference::Motion(ImGui::GetItemID()^0x8711,ImGui::IsItemHovered()?1.f:0.f);
    const auto paint=[](ImU32 color){return ImGui::GetColorU32(color);};
    auto* d=ImGui::GetWindowDrawList();
    if(trash) {
        d->AddRectFilled(p,p+size,ImGui::GetColorU32(t.buttonHoveredBgColor,hover*.35f),Scale(5));
    } else {
        d->AddRectFilled(p,p+size,paint(reference::Mix(t.selectorBgColor,t.buttonHoveredBgColor,hover*reference::kFieldHoverFill)),Scale(5));
        d->AddRect(p,p+size,paint(reference::Mix(t.splitColor,t.iconHighlightColor,hover*reference::kFieldHoverBorder)),Scale(5));
    }
    if(trash) {
        const auto c=p+size*.5f;const auto ink=paint(reference::Mix(t.secondaryColor,t.mainColor,hover));
        d->AddLine(c+ImVec2(Scale(-6),Scale(-4)),c+ImVec2(Scale(6),Scale(-4)),ink,Scale(1.4f));
        d->AddRect(c+ImVec2(Scale(-2),Scale(-7)),c+ImVec2(Scale(2),Scale(-4)),ink,Scale(1),0,Scale(1.4f));
        d->AddRect(c+ImVec2(Scale(-4.5f),Scale(-4)),c+ImVec2(Scale(4.5f),Scale(7)),ink,Scale(1.5f),0,Scale(1.4f));
        for(float x:{-1.5f,1.5f})d->AddLine(c+ImVec2(Scale(x),Scale(-1)),c+ImVec2(Scale(x),Scale(4)),ink,Scale(1));
    } else {
        PushText(kTextControl);d->AddText(CenterText(label,p.x+size.x*.5f,p.y+size.y*.5f,true,true),paint(t.mainColor),label);PopFont();
    }
    return click;
}
void Input(const ColorTheme& t,bool token) {
    auto& s=Data(); PushText(kTextControl);
    const auto p=ImGui::GetCursorScreenPos();
    const bool hover=ImGui::IsWindowHovered()&&ImGui::IsMouseHoveringRect(p,p+ImVec2(ImGui::GetContentRegionAvail().x,Scale(28)));
    const float response=reference::Motion(ImGui::GetID("account-input-hover"),hover||GImGui->ActiveId==ImGui::GetID("##account-value")?1.f:0.f);
    const auto background=reference::Mix(t.selectorBgColor,t.buttonHoveredBgColor,response*reference::kFieldHoverFill);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,Scale(5));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{Scale(8),std::max(0.f,(Scale(28)-TextHeight())*.5f)});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,background);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,background);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,background);
    ImGui::PushStyleColor(ImGuiCol_Text,t.mainColor);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg,color::ReAlpha(t.iconHighlightColor,reference::kFieldSelectionAlpha));
    ImGui::PushStyleColor(ImGuiCol_NavCursor,0u);
    ImGui::SetNextItemWidth(-1);
    if(s.focus){ImGui::SetKeyboardFocusHere();s.focus=false;}
    ImGui::InputTextWithHint("##account-value",token?"Minecraft access token":"Player name",s.input.data(),token?s.input.size():17,
        token?(ImGuiInputTextFlags_Password|ImGuiInputTextFlags_NoUndoRedo):ImGuiInputTextFlags_CharsNoBlank);
    s.inputId=ImGui::GetItemID();
    ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),ImGui::GetColorU32(reference::Mix(t.splitColor,t.iconHighlightColor,response*reference::kFieldHoverBorder)),Scale(5));
    ImGui::PopStyleColor(6);ImGui::PopStyleVar(3);PopFont();
}
}
void Hide() { Clear(); }
void Draw(PanelClickGui& gui,ImVec2 origin) {
    const auto snapshot=nova::accounts::Read(); const auto& t=gui.colors(); auto& state=Data();
    ImGui::SetCursorScreenPos(origin+ImVec2(Scale(174),Scale(70)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    ImGui::BeginChild("accounts-page",{Scale(558),Scale(490)},ImGuiChildFlags_None,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text,t.mainColor); PushText(kTextTitle);
    ImGui::TextUnformatted("Account Manager"); PopFont();
    ImGui::Dummy({0,Scale(8)});
    const float footer=Scale(108);
    ImGui::BeginChild("account-list",{0,std::max(Scale(80),ImGui::GetContentRegionAvail().y-footer)},ImGuiChildFlags_None,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar);
    size_t visible=0;
    for(const auto& account:snapshot.rows) {
        const auto& filter=gui.search();
        if(!filter.empty() && std::search(account.name.begin(),account.name.end(),filter.begin(),filter.end(),[](unsigned char a,unsigned char b){return std::tolower(a)==std::tolower(b);})==account.name.end())continue;
        ++visible;ImGui::PushID(account.id.c_str());const auto p=ImGui::GetCursorScreenPos();const float width=ImGui::GetContentRegionAvail().x;
        ImGui::BeginDisabled(!snapshot.supported||snapshot.busy||snapshot.inWorld||account.current);
        const bool select=ImGui::InvisibleButton("select",{width-Scale(44),Scale(42)});
        const float response=reference::Motion(ImGui::GetItemID()^0x811,ImGui::IsItemHovered()?1.f:0.f);
        const float selected=reference::Motion(ImGui::GetItemID()^0x812,account.current?1.f:0.f);
        ImGui::EndDisabled();
        auto* d=ImGui::GetWindowDrawList();
        d->AddRectFilled(p,p+ImVec2(width,Scale(42)),ImGui::GetColorU32(reference::Mix(t.modulePanelBgColor,t.buttonHoveredBgColor,response*.18f+selected*.08f)),Scale(5));
        const auto head=p+ImVec2(Scale(9),Scale(7));
        if(account.portrait) {
            Portrait portrait;portrait.texture=account.portrait;portrait.nearest=true;
            DrawPortrait(d,portrait,head,head+ImVec2(Scale(28),Scale(28)),ImGui::GetColorU32(IM_COL32_WHITE),Scale(3));
        } else {
            d->AddRectFilled(head,head+ImVec2(Scale(28),Scale(28)),ImGui::GetColorU32(t.selectorBgColor),Scale(3));
        }
        d->PushClipRect(p+ImVec2(Scale(47),0),p+ImVec2(width-Scale(68),Scale(42)),true);
        PushText(kTextBody);d->AddText(CenterText(account.name.c_str(),p.x+Scale(47),p.y+Scale(21),false,true),ImGui::GetColorU32(t.mainColor),account.name.c_str());PopFont();d->PopClipRect();
        if(selected>.001f)reference::Check(d,p+ImVec2(width-Scale(64),Scale(21)),ImGui::GetColorU32(t.iconHighlightColor,selected));
        if(select)nova::accounts::Submit(nova::accounts::Action::Select,account.id);
        ImGui::SetCursorScreenPos(p+ImVec2(width-Scale(35),Scale(7)));
        ImGui::BeginDisabled(snapshot.busy||account.source==nova::accounts::Source::Current);
        if(Button(t,"remove","",{Scale(28),Scale(28)},true)){Clear();state.removeId=account.id;state.dialog=3;state.openPopup=true;}
        ImGui::EndDisabled();ImGui::SetCursorScreenPos(p+ImVec2(0,Scale(48)));ImGui::Dummy({0,0});ImGui::PopID();
    }
    if(!visible){PushText(kTextControl);ImGui::TextDisabled("No accounts to display");PopFont();}
    ImGui::EndChild();
    if(!snapshot.status.empty()) {
        PushText(kTextSmall);ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+ImGui::GetContentRegionAvail().x);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(t.secondaryColor),"%s",snapshot.status.c_str());
        ImGui::PopTextWrapPos();PopFont();
    }
    if(snapshot.authorizing&&Button(t,"cancel-auth","Cancel sign-in",{Scale(132),Scale(26)}))nova::accounts::Submit(nova::accounts::Action::Cancel);
    ImGui::SetCursorPosY(ImGui::GetWindowHeight()-Scale(60));
    ImGui::BeginDisabled(!snapshot.supported||snapshot.busy);
    const float w=(ImGui::GetContentRegionAvail().x-Scale(16))/3;
    if(Button(t,"microsoft","Add Microsoft",{w,Scale(32)})) {
        if(snapshot.inWorld){Clear();state.dialog=4;state.openPopup=true;}
        else nova::accounts::Submit(nova::accounts::Action::Microsoft);
    }
    ImGui::SameLine(0,Scale(8));if(Button(t,"offline","Add Offline",{w,Scale(32)})){Clear();state.dialog=1;state.focus=true;state.openPopup=true;}
    ImGui::SameLine(0,Scale(8));if(Button(t,"token","Token Login",{w,Scale(32)})){Clear();state.dialog=2;state.focus=true;state.openPopup=true;}
    ImGui::EndDisabled();
    reference::PageReveal(gui.pageReveal());
    if(state.openPopup){ImGui::OpenPopup("account-dialog");state.openPopup=false;}
    ImGui::SetNextWindowPos(origin+ImVec2(Scale(274),Scale(193)),ImGuiCond_Appearing);
    ImGui::SetNextWindowSize({Scale(356),0},ImGuiCond_Always);
    if(BeginPanelPopup(t,"account-dialog",ImGuiWindowFlags_AlwaysAutoResize,PopupMotion::Account,{12,14})) {
        const auto microsoftTitle=ClientSettings::Get().clientName()+" Microsoft";
        PushText(kTextTitle);ImGui::TextUnformatted(state.dialog==1?"Add Offline":state.dialog==2?"Token Login":state.dialog==3?"Remove account":microsoftTitle.c_str());PopFont();
        // Panel popups set ItemSpacing to zero; Spacing() would add no gap.
        ImGui::Dummy({0,Scale(10)});
        if(state.dialog<=2)Input(t,state.dialog==2);
        if(snapshot.inWorld&&state.dialog!=3){
            if(state.dialog<=2)ImGui::Dummy({0,Scale(8)});
            PushText(kTextControl);ImGui::TextUnformatted("Return to the main menu to sign in.");PopFont();
        }
        if(state.dialog<=2||state.dialog>=4)ImGui::Dummy({0,Scale(12)});
        const float actionWidth=(ImGui::GetContentRegionAvail().x-Scale(10))*.5f;
        ImGui::BeginDisabled(snapshot.busy || state.dialog==4 || (state.dialog<=2&&(snapshot.inWorld||!state.input[0])));
        if(Button(t,"confirm",state.dialog==3?"Remove":"Continue",{actionWidth,Scale(30)})) {
            using enum nova::accounts::Action;
            if(nova::accounts::Submit(state.dialog==1?Offline:state.dialog==2?Token:Remove,state.dialog==3?state.removeId:state.input.data())){Clear();ImGui::CloseCurrentPopup();}
        }
        ImGui::EndDisabled();ImGui::SameLine(0,Scale(10));
        if(Button(t,"cancel","Cancel",{actionWidth,Scale(30)})){Clear();ImGui::CloseCurrentPopup();}
        if(ImGui::IsKeyPressed(ImGuiKey_Escape,false)){Clear();ImGui::CloseCurrentPopup();}
        EndPanelPopup();
    } else if(state.dialog)Clear();
    ImGui::PopStyleColor();ImGui::EndChild();ImGui::PopStyleVar();
}
}
