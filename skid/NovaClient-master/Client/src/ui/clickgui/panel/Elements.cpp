#include "ui/backend/blur/Blur.h"
#include "ui/clickgui/panel/Elements.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

#include "bind/BindCapture.h"
#include "bind/BindProperty.h"
#include "bind/KeyName.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "ui/clickgui/panel/PanelClickGui.h"
#include "ui/clickgui/panel/Widgets.h"
#include "ui/clickgui/panel/ReferenceStyle.h"
#include "utils/Color.h"
#include "ui/hud/musichud/SourcePicker.h"
#include "smtc/SmtcService.h"

namespace nova::menu::panel {
namespace {

using Properties::BoolProperty;
using Properties::ColorProperty;
using Properties::ModeProperty;
using Properties::NumberProp;

class MediaSourceElement final : public ElementRenderer {
public:
    using ElementRenderer::ElementRenderer;
    float width(const Properties::AbstractProperty&) const override { return Scale(134); }
    float height(const Properties::AbstractProperty&) const override { return Scale(23); }
    void draw(Properties::AbstractProperty& property) override {
        auto& prop=static_cast<ui::music::SourceProperty&>(property);
        const auto snapshot=smtc::ReadSnapshot();
        std::string label=prop.get().empty()?"Auto":prop.get();
        if(snapshot) for(const auto& item:snapshot->sources) if(item.id==prop.get()) label=item.name;
        const auto& t=gui_.colors(); const auto p=ImGui::GetCursorScreenPos();
        if(reference::Select(ImGui::GetWindowDrawList(),0,p,Scale(134),label.c_str())) ImGui::OpenPopup("source-popup");
        const float h=Scale(8+32*(1+(snapshot?snapshot->sources.size():0)));
        ImGui::SetNextWindowPos(p+ImVec2(0,(Scale(23)-h)*.5f),ImGuiCond_Appearing);
        ImGui::SetNextWindowSizeConstraints({Scale(220),0},{Scale(220),ImGui::GetIO().DisplaySize.y-Scale(20)});
        if(BeginPanelPopup(t,"source-popup")) {
            if(Choice(t,"auto","Auto",prop.get()=="Auto"||prop.get().empty(),Scale(212))) {prop.set("Auto");ImGui::CloseCurrentPopup();}
            if(snapshot) for(const auto& item:snapshot->sources) if(Choice(t,item.id.c_str(),item.name.c_str(),prop.get()==item.id,Scale(212))) {prop.set(item.id);ImGui::CloseCurrentPopup();}
            EndPanelPopup();
        }
    }
};

class TextElement final : public ElementRenderer {
public:
    explicit TextElement(PanelClickGui& gui) : ElementRenderer(gui) {}
    float width(const Properties::AbstractProperty&) const override { return Scale(134.0f); }
    float height(const Properties::AbstractProperty&) const override { return Scale(23.0f); }
    void draw(Properties::AbstractProperty& property) override {
        auto& prop = static_cast<Properties::StringProperty&>(property);
        std::array<char, 1024> buffer{};
        const auto text = prop.get();
        std::copy_n(text.data(), std::min(text.size(), buffer.size() - 1), buffer.data());
        const ColorTheme& theme = gui_.colors();

        // Match the panel's search/config inputs, with compact setting-row padding.
        PushText(kTextBody);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Scale(5.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(Scale(7.0f), Scale(4.0f)));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.selectorBgColor);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, theme.selectorBgColor);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, theme.selectorBgColor);
        ImGui::PushStyleColor(ImGuiCol_Text, theme.mainColor);
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, theme.secondaryColor);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, color::ReAlpha(theme.iconHighlightColor, reference::kFieldSelectionAlpha));
        ImGui::PushStyleColor(ImGuiCol_NavCursor, 0u);
        ImGui::SetNextItemWidth(width(prop));
        if (ImGui::InputText("##text", buffer.data(), buffer.size()))
            prop.set(buffer.data());

        const float response = reference::Motion(ImGui::GetItemID() ^ 0x6161u,
            ImGui::IsItemActive() || ImGui::IsItemHovered() ? 1.f : 0.f);
        const Color outline = reference::Mix(theme.splitColor, theme.iconHighlightColor,
            response * reference::kFieldHoverBorder);
        const ImVec2 inset(Scale(0.5f), Scale(0.5f));
        ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin() + inset,
            ImGui::GetItemRectMax() - inset, ImGui::GetColorU32(outline), Scale(5.5f));
        ImGui::PopStyleColor(7);
        ImGui::PopStyleVar(3);
        PopFont();
    }
};

using nova::bind::BindCapture;
using nova::bind::BindProperty;

class SwitchElement final : public ElementRenderer {
public:
    using ElementRenderer::ElementRenderer;
    float width(const Properties::AbstractProperty&) const override { return Scale(29); }
    float height(const Properties::AbstractProperty&) const override { return Scale(18); }
    void draw(Properties::AbstractProperty& input) override {
        auto& p=static_cast<BoolProperty&>(input);
        bool value=p.get();
        reference::Toggle(ImGui::GetWindowDrawList(),0,ImGui::GetCursorScreenPos(),value);
        if(value!=p.get())p.set(value);
    }
};

class SliderElement final : public ElementRenderer {
public:
    using ElementRenderer::ElementRenderer;
    float width(const Properties::AbstractProperty&) const override { return Scale(132); }
    float height(const Properties::AbstractProperty&) const override { return Scale(21); }
    void draw(Properties::AbstractProperty& input) override {
        auto& p=static_cast<NumberProp&>(input); const auto& t=gui_.colors();
        if (lastFrame_ != ImGui::GetFrameCount() - 1) {
            editing_ = focus_ = false;
            editReveal_ = 0.f;
        }
        lastFrame_ = ImGui::GetFrameCount();
        const float target=editing_?1.f:0.f;
        editReveal_ += (target-editReveal_)*(1.f-std::exp(-22.f*ImGui::GetIO().DeltaTime));
        if(std::abs(editReveal_-target)<.001f)editReveal_=target;
        PushText(kTextControl);
        // Reserve enough room for formatted values, including units and range endpoints.
        const float labelWidth=std::max({TextWidth(p.formatValue(p.get()).c_str()),
            TextWidth(p.formatValue(p.getMin()).c_str()),TextWidth(p.formatValue(p.getMax()).c_str())});
        const float restingWidth=std::clamp(labelWidth+Scale(8),Scale(42),Scale(90));
        const float valueWidth=std::lerp(restingWidth,std::max(restingWidth,Scale(58)),editReveal_);
        const float trackWidth=132.f-valueWidth/Scale(1.f)-10.f;
        const auto start=ImGui::GetCursorScreenPos(); auto* d=ImGui::GetWindowDrawList();
        const double span=p.getMax()-p.getMin();
        const float amount=span>0?float((p.get()-p.getMin())/span):0;
        const float changed=reference::Slider(d,0,start+ImVec2(0,Scale(10)),amount,amount,trackWidth);
        if(changed!=amount)p.set(p.getMin()+changed*span);
        ImGui::SetCursorScreenPos(start+ImVec2(Scale(132)-valueWidth,0));
        const auto valuePos = ImGui::GetCursorScreenPos();
        const auto valueEnd = valuePos + ImVec2(valueWidth, Scale(21));
        const bool hovered = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(valuePos,valueEnd);
        const float response = Motion(ImGui::GetID("##value-hover"),hovered || editing_ ? 1.f : 0.f,16.f);
        const auto background = reference::Mix(t.selectorBgColor,t.buttonHoveredBgColor,response * reference::kFieldHoverFill);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{Scale(4),std::max(0.f,(Scale(21)-TextHeight())*.5f)});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,Scale(5));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0.f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg,background);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,background);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,background);
        ImGui::PushStyleColor(ImGuiCol_Text,t.mainColor);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg,color::ReAlpha(t.iconHighlightColor,reference::kFieldSelectionAlpha));
        ImGui::PushStyleColor(ImGuiCol_NavCursor,0u);
        if(editing_) {
            ImGui::SetNextItemWidth(valueWidth);
            if(focus_) { ImGui::SetKeyboardFocusHere(); focus_=false; }
            const bool entered = ImGui::InputText("##value-edit",buffer_.data(),buffer_.size(),
                ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_AutoSelectAll|ImGuiInputTextFlags_CharsScientific);
            if(entered || ImGui::IsItemDeactivatedAfterEdit()) {
                char* end=nullptr; double v=std::strtod(buffer_.data(),&end);
                if(end!=buffer_.data() && *end=='\0' && std::isfinite(v)) p.set(v);
                editing_=false;
            }
            if(ImGui::IsItemDeactivated()) editing_=false;
        } else {
            const auto pos=ImGui::GetCursorScreenPos();
            if(ImGui::InvisibleButton("value",{valueWidth,Scale(21)})) {
                editing_=focus_=true; std::snprintf(buffer_.data(),buffer_.size(),"%.8g",p.get());
            }
            d->AddRectFilled(pos,pos+ImVec2(valueWidth,Scale(21)),background,Scale(5));
            const auto label=p.formatValue(p.get());
            d->PushClipRect(pos,pos+ImVec2(valueWidth,Scale(21)),true);
            const float fit=std::min(1.f,(valueWidth-Scale(8))/std::max(1.f,TextWidth(label.c_str())));
            const float fontSize=ImGui::GetFontSize()*fit;
            const float centered=(valueWidth-TextWidth(label.c_str())*fit)*.5f;
            d->AddText(ImGui::GetFont(),fontSize,pos+ImVec2(std::lerp(centered,Scale(4),editReveal_),
                (Scale(21)-fontSize)*.5f),t.mainColor,label.c_str());
            d->PopClipRect();
            if(ImGui::IsItemHovered()) {
                // Do not inherit the enclosing panel's large rounding/padding.
                PushText(kTextSmall);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{Scale(8),Scale(5)});
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,Scale(5));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,Scale(1));
                ImGui::PushStyleColor(ImGuiCol_PopupBg,t.selectorBgColor);
                ImGui::PushStyleColor(ImGuiCol_Border,t.splitColor);
                ImGui::PushStyleColor(ImGuiCol_Text,t.mainColor);
                if(ImGui::BeginTooltip()) {
                    ImGui::Text("%s (click to edit)",label.c_str());
                    ImGui::EndTooltip();
                }
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar(3);
                PopFont();
            }
        }
        d->AddRect(valuePos,valueEnd,reference::Mix(t.splitColor,t.iconHighlightColor,response * reference::kFieldHoverBorder),Scale(5),0,Scale(1));
        ImGui::PopStyleColor(6); ImGui::PopStyleVar(3); PopFont();
    }
private:
    bool editing_=false,focus_=false;
    float editReveal_=0.f;
    int lastFrame_=-2;
    std::array<char,64> buffer_{};
};

class SelectorElement final : public ElementRenderer {
public:
    using ElementRenderer::ElementRenderer;
    float width(const Properties::AbstractProperty&) const override { return Scale(134); }
    float height(const Properties::AbstractProperty&) const override { return Scale(23); }
    void draw(Properties::AbstractProperty& input) override {
        auto& p=static_cast<ModeProperty&>(input); const auto& t=gui_.colors();
        const auto start=ImGui::GetCursorScreenPos();
        if(reference::Select(ImGui::GetWindowDrawList(),0,start,Scale(134),p.getValueNameCStr(p.get()))) ImGui::OpenPopup("popup");
        const float popupHeight=std::min(Scale(32*p.getValueCount()+8),ImGui::GetIO().DisplaySize.y-Scale(20));
        ImVec2 at=start+ImVec2(0,(Scale(23)-popupHeight)*.5f);
        const auto display=ImGui::GetIO().DisplaySize;
        at.x=std::clamp(at.x,Scale(10),std::max(Scale(10),display.x-Scale(144)));
        at.y=std::clamp(at.y,Scale(10),std::max(Scale(10),display.y-popupHeight-Scale(10)));
        ImGui::SetNextWindowPos(at);
        ImGui::SetNextWindowSize({Scale(134),popupHeight});
        if(BeginPanelPopup(t,"popup")) {
            for(int i=0;i<p.getValueCount();++i) {
                ImGui::PushID(i);
                if(Choice(t,"option",p.getValueNameCStr(i),p.get()==i,ImGui::GetContentRegionAvail().x)) {p.set(i); ImGui::CloseCurrentPopup();}
                ImGui::PopID();
            }
            EndPanelPopup();
        }
    }
};

class ColorSelectorElement final : public ElementRenderer {
public:
    using ElementRenderer::ElementRenderer;
    float width(const Properties::AbstractProperty&) const override {return Scale(18);}
    float height(const Properties::AbstractProperty&) const override {return Scale(18);}
    void draw(Properties::AbstractProperty& input) override {
        auto& p=static_cast<ColorProperty&>(input); const auto& t=gui_.colors();
        if(ImGui::InvisibleButton("swatch",{Scale(18),Scale(18)})) {
            editingColor_=false; invalidColor_=false;
            ImGui::OpenPopup("picker");
        }
        const auto at=ImGui::GetItemRectMin(), end=ImGui::GetItemRectMax();
        auto* d=ImGui::GetWindowDrawList();
        d->AddRectFilled(at,end,color::ReAlpha(color::HsbToBgr(p.h(),p.s(),p.b()),int((p.allowsAlpha()?p.a():1.f)*255)),Scale(5));
        ImGui::SetNextWindowPos({at.x,end.y+Scale(4)},ImGuiCond_Appearing);
        if(BeginPanelPopup(t,"picker")) {
            ImGui::SetCursorPos({Scale(13),Scale(13)});
            DrawPicker(p);
            ImGui::Dummy({0,Scale(9)});
            EndPanelPopup();
        }
    }
private:
    template<class Paint> static void Gradient(ImDrawList* d,ImVec2 p,ImVec2 size,float rounding,Paint paint) {
        int first=d->VtxBuffer.Size; d->AddRectFilled(p,p+size,IM_COL32_WHITE,rounding);
        for(int i=first;i<d->VtxBuffer.Size;++i){auto& v=d->VtxBuffer[i];auto c=paint(std::clamp((v.pos.x-p.x)/size.x,0.f,1.f),std::clamp((v.pos.y-p.y)/size.y,0.f,1.f));v.col=color::ReAlpha(c,color::Alpha(c)*color::Alpha(v.col)/255);}
    }
    void DrawPicker(ColorProperty& p) {
        // The reference's horizontal hue/alpha picker (15px text, 4px bars).
        const float side=Scale(p.allowsAlpha()?186:198); float h=p.h(),s=p.s(),v=p.b(),a=p.a();
        auto* d=ImGui::GetWindowDrawList();auto at=ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("square",{side,side});const bool active=ImGui::IsItemActive();
        if(active){s=std::clamp((ImGui::GetIO().MousePos.x-at.x)/(side-Scale(1)),0.f,1.f);v=1-std::clamp((ImGui::GetIO().MousePos.y-at.y)/(side-Scale(1)),0.f,1.f);}
        const auto hue=color::HsbToBgr(h,1,1);
        Gradient(d,at,{side,side},Scale(4),[&](float x,float y){return color::Interpolate(color::Interpolate(IM_COL32_WHITE,hue,x),IM_COL32_BLACK,y);});
        d->AddRect(at,at+ImVec2(side,side),gui_.colors().splitColor,Scale(4));
        ImVec2 cursor(at.x+std::clamp(s*side,Scale(2),side-Scale(2)),at.y+std::clamp((1-v)*side,Scale(2),side-Scale(2)));
        d->AddCircle(cursor+ImVec2(Scale(1),Scale(1)),Scale(active?6:4),IM_COL32(0,0,0,64),30,Scale(1));
        d->AddCircle(cursor,Scale(active?6:4),IM_COL32_WHITE,30,Scale(1));
        auto bar=at+ImVec2(0,side+Scale(12));
        ImGui::SetCursorScreenPos(bar-ImVec2(0,Scale(4)));
        ImGui::InvisibleButton("hue",{side,Scale(12)});
        if(ImGui::IsItemActive()) h=std::clamp((ImGui::GetIO().MousePos.x-bar.x)/(side-Scale(1)),0.f,1.f);
        // Six exact RGB ramps; each ramp is a mesh so intermediate hues are preserved.
        constexpr ImU32 hues[]={IM_COL32(255,0,0,255),IM_COL32(255,255,0,255),IM_COL32(0,255,0,255),IM_COL32(0,255,255,255),IM_COL32(0,0,255,255),IM_COL32(255,0,255,255),IM_COL32(255,0,0,255)};
        for(int i=0;i<6;++i)d->AddRectFilledMultiColor(bar+ImVec2(side*i/6,0),bar+ImVec2(side*(i+1)/6,Scale(4)),hues[i],hues[i+1],hues[i+1],hues[i]);
        d->AddCircleFilled(bar+ImVec2(std::round(h*side)+Scale(1),Scale(2)),Scale(6),color::HsbToBgr(h,1,1));
        if(p.allowsAlpha()) {
            bar.y+=Scale(16);ImGui::SetCursorScreenPos(bar-ImVec2(0,Scale(4)));
            ImGui::InvisibleButton("alpha",{side,Scale(12)});
            if(ImGui::IsItemActive())a=1-std::clamp((ImGui::GetIO().MousePos.x-bar.x)/(side-Scale(1)),0.f,1.f);
            const auto rgb=color::HsbToBgr(h,s,v);
            d->AddRectFilled(bar,bar+ImVec2(side,Scale(4)),IM_COL32_BLACK,Scale(3));
            Gradient(d,bar,{side,Scale(4)},Scale(3),[&](float x,float){return color::ReAlpha(rgb,int(255*(1-x)));});
            d->AddCircleFilled(bar+ImVec2(std::round((1-a)*side)+Scale(1),Scale(2)),Scale(6),rgb);
        }
        ImGui::SetCursorScreenPos({at.x,bar.y+Scale(10)});ImGui::Dummy({side+Scale(9),0});
        const float hsba[]={h,s,v,a};p.setHsba(hsba);
        const auto rgb = color::HsbToBgr(h,s,v);
        const auto code = p.allowsAlpha()
            ? std::format("#{:02X}{:02X}{:02X}{:02X}", rgb & 255, (rgb >> 8) & 255,
                (rgb >> 16) & 255, int(std::lround(a*255)))
            : std::format("#{:02X}{:02X}{:02X}", rgb & 255, (rgb >> 8) & 255, (rgb >> 16) & 255);
        // Reference ColorEdit4 uses an inline HEX editor rather than action buttons.
        if (!editingColor_) std::snprintf(colorBuffer_.data(),colorBuffer_.size(),"%s",code.c_str());
        const auto& theme=gui_.colors();
        ImGui::SetCursorScreenPos({at.x,bar.y+Scale(17)});
        PushText(kTextControl);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,Scale(5));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{Scale(8),Scale(5)});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0.f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg,theme.selectorBgColor);
        ImGui::PushStyleColor(ImGuiCol_Text,theme.mainColor);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg,color::ReAlpha(theme.iconHighlightColor,reference::kFieldSelectionAlpha));
        ImGui::PushStyleColor(ImGuiCol_NavCursor,0u);
        ImGui::SetNextItemWidth(side);
        const bool entered=ImGui::InputText("##color-hex",colorBuffer_.data(),colorBuffer_.size(),
            ImGuiInputTextFlags_AutoSelectAll|ImGuiInputTextFlags_EnterReturnsTrue);
        editingColor_=ImGui::IsItemActive();
        const bool commit=entered || ImGui::IsItemDeactivatedAfterEdit();
        const float response=reference::Motion(ImGui::GetItemID() ^ 0x636fu,
            editingColor_ || ImGui::IsItemHovered() ? 1.f : 0.f);
        d->AddRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),
            reference::Mix(theme.splitColor,theme.iconHighlightColor,response*reference::kFieldHoverBorder),Scale(5));
        ImGui::PopStyleColor(4); ImGui::PopStyleVar(3); PopFont();
        if (commit) {
            std::string_view value = colorBuffer_.data();
            const auto first = value.find_first_not_of(" \t\r\n");
            value = first == std::string_view::npos ? std::string_view{} : value.substr(first);
            const auto last = value.find_last_not_of(" \t\r\n");
            if (last != std::string_view::npos) value = value.substr(0,last+1);
            if (value.starts_with('#')) value.remove_prefix(1);
            else if (value.starts_with("0x") || value.starts_with("0X")) value.remove_prefix(2);
            invalidColor_ = value.size()!=6 && value.size()!=8;
            unsigned parsed = 0;
            for (char c : value) {
                const int digit = c>='0'&&c<='9' ? c-'0' : c>='a'&&c<='f' ? c-'a'+10 : c>='A'&&c<='F' ? c-'A'+10 : -1;
                if (digit<0) { invalidColor_=true; break; }
                parsed = (parsed<<4) | unsigned(digit);
            }
            if (!invalidColor_) {
                const auto channels = value.size()==8 ? parsed>>8 : parsed;
                float next[4];
                color::BgrToHsb(IM_COL32((channels>>16)&255,(channels>>8)&255,channels&255,255),next);
                next[3] = p.allowsAlpha() && value.size()==8 ? float(parsed&255)/255.f : p.a();
                p.setHsba(next);
            }
            if (entered) { ImGui::ClearActiveID(); editingColor_=false; }
        }
        if (invalidColor_ && ImGui::IsItemHovered()) ImGui::SetTooltip("Invalid color. Use #RRGGBB or #RRGGBBAA.");
    }
    bool invalidColor_ = false;
    bool editingColor_ = false;
    std::array<char,64> colorBuffer_{};
};

constexpr float kBindIconSize = 14.0f;

class BindElement final : public ElementRenderer {
public:
    using ElementRenderer::ElementRenderer;

    [[nodiscard]] float width(const Properties::AbstractProperty&) const override {
        return Scale(kBindIconSize);
    }
    [[nodiscard]] float height(const Properties::AbstractProperty&) const override {
        return Scale(kBindIconSize);
    }

    void draw(Properties::AbstractProperty& propIn) override {
        auto& prop = static_cast<BindProperty&>(propIn);
        const ColorTheme& theme = gui_.colors();

        if (ImGui::InvisibleButton("bind", ImVec2(Scale(kBindIconSize), Scale(kBindIconSize))))
            ImGui::OpenPopup("bindpopup");

        PushPanelIcon(13);
        const char* glyph = Icon(u8"\uf11c");

        const ImVec2 glyphSize = ImGui::CalcTextSize(glyph);
        ImGui::GetWindowDrawList()->AddText(
            ImGui::GetItemRectMin() +
                ImVec2((Scale(kBindIconSize) - glyphSize.x) / 2.0f,
                       (Scale(kBindIconSize) - glyphSize.y) / 2.0f - Scale(0.5f)),
            prop.isActive() ? theme.iconHighlightColor : theme.mainColor, glyph);
        PopFont();

        DrawPopup(prop);
    }

private:
    void DrawPopup(BindProperty& prop) {
        const ColorTheme& theme = gui_.colors();

        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, Scale(12.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, Scale(1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Scale(4.0f), Scale(4.0f)));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, theme.modulePanelBgColor);
        ImGui::PushStyleColor(ImGuiCol_Border, theme.windowBorderColor);

        if (BeginPanelPopup(theme, "bindpopup")) {
            ImGui::PushStyleColor(ImGuiCol_Button, theme.buttonBgColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme.buttonActiveBgColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme.buttonHoveredBgColor);
            ImGui::PushStyleColor(ImGuiCol_Text, theme.mainColor);

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
            ImGui::BeginGroup();

            ImGui::Dummy(ImVec2(0.0f, Scale(4.0f)));
            {
                ImGui::BeginGroup();
                ImGui::Dummy(ImVec2(Scale(4.0f), 0.0f));
                ImGui::SameLine();

                PushText(kTextBody);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Scale(5.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(Scale(6.0f), Scale(1.0f)));

                if (!prop.keyOnly()) {
                    if (ImGui::Button(prop.hold() ? "Hold###bindtype" : "Click###bindtype"))
                        prop.setHold(!prop.hold());
                    ImGui::SameLine(0.0f, Scale(4.0f));
                }

                BindCapture& capture = BindCapture::Get();
                const bool listening = capture.target() == &prop;
                const std::string label =
                    listening ? std::string("LISTENING") : nova::bind::KeyName(prop.keybind());
                ImGui::Button((label + "###bindkey").c_str());
                capture.setPressing(ImGui::IsItemActive());
                if (ImGui::IsItemActivated()) {
                    if (listening) {
                        prop.setKeybind(0);
                        capture.Cancel();
                    } else {
                        capture.Begin(&prop);
                    }
                }
                ImGui::SameLine(0.0f, Scale(4.0f));

                if (ImGui::Button("Delete###bindclear")) {
                    prop.setKeybind(0);
                    if (listening)
                        capture.Cancel();
                }

                ImGui::SameLine();
                ImGui::Dummy(ImVec2(Scale(4.0f), 0.0f));

                ImGui::PopStyleVar(2);
                PopFont();
                ImGui::EndGroup();
            }
            ImGui::Dummy(ImVec2(0.0f, Scale(4.0f)));

            ImGui::EndGroup();
            ImGui::PopStyleVar();

            ImGui::PopStyleColor(4);
            EndPanelPopup();
        } else if (prop.keyOnly() && BindCapture::Get().target() == &prop) {
            BindCapture::Get().Cancel();
        }

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }
};

}

float ElementRenderer::height(const Properties::AbstractProperty&) const {
    return Scale(18.0f);
}

std::unique_ptr<ElementRenderer> MakeElement(PanelClickGui& gui, Properties::AbstractProperty& prop) {
    if (dynamic_cast<ui::music::SourceProperty*>(&prop))
        return std::make_unique<MediaSourceElement>(gui);
    if (dynamic_cast<Properties::StringProperty*>(&prop))
        return std::make_unique<TextElement>(gui);
    if (dynamic_cast<BindProperty*>(&prop))
        return std::make_unique<BindElement>(gui);
    if (dynamic_cast<BoolProperty*>(&prop))
        return std::make_unique<SwitchElement>(gui);
    if (dynamic_cast<NumberProp*>(&prop))
        return std::make_unique<SliderElement>(gui);
    if (dynamic_cast<ModeProperty*>(&prop))
        return std::make_unique<SelectorElement>(gui);
    if (dynamic_cast<ColorProperty*>(&prop))
        return std::make_unique<ColorSelectorElement>(gui);
    return nullptr;
}

}
