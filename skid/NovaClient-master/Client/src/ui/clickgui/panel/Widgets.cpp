#include "ui/clickgui/panel/Widgets.h"
#include "ui/clickgui/panel/ReferenceStyle.h"

#include <algorithm>
#include <cmath>
#include "imgui_internal.h"
#include "ui/backend/blur/Blur.h"

#include "ui/clickgui/Fonts.h"

namespace nova::menu::panel {

namespace { float panelScale = 1.0f; }

void SetPanelScale(float scale) { panelScale = scale; }
float Scale(float pixels) { return pixels * panelScale; }

void PushText(float size) {
    ImGui::PushFont(PanelRegular, size);
}

void PushIcon(float size) {
    ImGui::PushFont(IconFont2, size);
}

void PopFont() {
    ImGui::PopFont();
}

void PushPanelIcon(float size) { ImGui::PushFont(PanelIcons, size); }
float Motion(ImGuiID id, float target, float speed, float initial) {
    return reference::Motion(id,target,speed,initial);
}
void Check(ImDrawList* d, ImVec2 p, ImU32 c) {
    reference::Check(d,p,c);
}
void Chevron(ImDrawList* d, ImVec2 p, ImU32 c, bool down) {
    if(!down){reference::Chevron(d,p,c);return;}
    const ImVec2 mid = down ? ImVec2(Scale(4),Scale(4)) : ImVec2(Scale(3),Scale(3));
    const ImVec2 end = down ? ImVec2(Scale(8),0) : ImVec2(0,Scale(6));
    d->AddLine(p,p+mid,c,Scale(1.3f)); d->AddLine(p+mid,p+end,c,Scale(1.3f));
}
bool Choice(const ColorTheme& t, const char* id, const char* label, bool selected, float width) {
    ImGui::PushID(id);
    auto p=ImGui::GetCursorScreenPos();
    const bool clicked=ImGui::InvisibleButton("choice",{width,Scale(32)});
    const float hover=Motion(ImGui::GetItemID() ^ 0x9921u,ImGui::IsItemHovered()?1.f:0.f,22);
    auto* d=ImGui::GetWindowDrawList();
    d->AddRectFilled(p,p+ImVec2(width,Scale(32)),color::ReAlpha(t.selectorElementHoverColor,int(color::Alpha(t.selectorElementHoverColor)*hover)),Scale(10));
    if(selected) Check(d,p+ImVec2(Scale(14),Scale(16)),t.textHighlightColor);
    PushText(kTextControl);
    d->PushClipRect(p,p+ImVec2(width,Scale(32)),true);
    d->AddText(p+ImVec2(Scale(35),(Scale(32)-TextHeight())*.5f),selected?t.textHighlightColor:t.mainColor,label);
    d->PopClipRect(); PopFont(); ImGui::PopID();
    return clicked;
}
bool BeginPanelPopup(const ColorTheme& t, const char* id, ImGuiWindowFlags flags, PopupMotion motion, ImVec2 padding) {
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding,Scale(16));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize,Scale(1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{Scale(padding.x),Scale(padding.y)});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{0,0});
    const bool dark=color::Red(t.windowBgColor)<100;
    ImGui::PushStyleColor(ImGuiCol_PopupBg,dark?IM_COL32(20,20,29,235):IM_COL32(248,249,252,245));
    ImGui::PushStyleColor(ImGuiCol_Border,dark?IM_COL32(57,61,76,205):IM_COL32(180,187,202,205));
    ImGui::PushStyleColor(ImGuiCol_Text,t.mainColor);
    bool result=ImGui::BeginPopup(id,flags|ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleColor(3); ImGui::PopStyleVar(4);
    if(result) {
        auto* w=ImGui::GetCurrentWindow();
        w->DrawList->PushClipRectFullScreen();
        reference::PopupBackdrop(w->DrawList,w->Pos,w->Size);
        w->DrawList->PopClipRect();
        if(w->Appearing) w->StateStorage.SetFloat(ImGui::GetID("##reveal"),0);
        // Animation is applied to the popup draw list, including its rounded backdrop.
        w->StateStorage.SetInt(ImGui::GetID("##motion"),int(motion));
    }
    return result;
}
void EndPanelPopup() {
    auto* w=ImGui::GetCurrentWindow();
    const auto motion=static_cast<PopupMotion>(w->StateStorage.GetInt(ImGui::GetID("##motion")));
    const float open=Motion(ImGui::GetID("##reveal"),1,motion==PopupMotion::Account?17.f:20.f,0);
    CapturePopupAnimation(w,open,motion);
    for(auto* child:GImGui->Windows) if(child->Active&&(child==w||ImGui::IsWindowChildOf(child,w,false)))
        TransformPopup(child->DrawList,w->Pos,w->Size,open,motion);
    ImGui::EndPopup();
}

float TextWidth(const char* text) {
    return ImGui::CalcTextSize(text).x;
}

ImVec2 CenterText(const char* text, float x, float y, bool horizontal, bool vertical) {
    return ImVec2(horizontal ? x - TextWidth(text) / 2.0f : x,
                  vertical ? y - TextHeight() / 2.0f : y);
}

void SplitLine(const ColorTheme& theme) {
    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, Scale(1.0f)));
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), theme.splitColor);
}

void DrawScrollBar(const ColorTheme& theme, const char* id, const ImVec2& contentSize) {
    const float scrollMaxY = ImGui::GetScrollMaxY();
    if (scrollMaxY <= Scale(2.0f))
        return;

    const float trackWidth = Scale(4.0f);
    const float trackPadding = Scale(4.0f);
    const float rounding = trackWidth / 2.0f;

    const ImVec2 windowPos = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();

    const float trackMinX = windowPos.x + windowSize.x - trackPadding - trackWidth;
    const float trackMaxX = windowPos.x + windowSize.x - trackPadding;
    const float trackMinY = windowPos.y + Scale(8.0f);
    const float trackMaxY = windowPos.y + windowSize.y - Scale(8.0f);
    const float trackHeight = trackMaxY - trackMinY;

    const float visibleRatio = std::min(1.0f, contentSize.y / (contentSize.y + scrollMaxY));
    const float thumbHeight = std::max(Scale(24.0f), trackHeight * visibleRatio);
    const float thumbTravel = std::max(0.0f, trackHeight - thumbHeight);

    ImVec2 cursorPos = ImGui::GetCursorPos();
    ImGui::SetCursorScreenPos(ImVec2(trackMinX, trackMinY));
    ImGui::InvisibleButton(id, ImVec2(trackWidth, trackHeight));

    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();
    if (held) {
        const float mouseDeltaY = ImGui::GetIO().MouseDelta.y;
        const float scrollOffset =
            thumbTravel <= 0.0f ? 0.0f : mouseDeltaY / thumbTravel * scrollMaxY;
        if (scrollOffset != 0.0f)
            ImGui::SetScrollY(std::clamp(ImGui::GetScrollY() + scrollOffset, 0.0f, scrollMaxY));
    }

    const float thumbOffset = thumbTravel * (ImGui::GetScrollY() / scrollMaxY);
    const float thumbMinY = trackMinY + thumbOffset;
    const float thumbMaxY = thumbMinY + thumbHeight;

    ImGui::SetCursorPos(cursorPos);
    ImGui::Dummy(ImVec2(0.0f, 0.0f));

    const Color thumbColor = held      ? theme.scrollbarThumbActiveColor
                             : hovered ? theme.scrollbarThumbHoverColor
                                       : theme.scrollbarThumbColor;
    ImDrawList* draws = ImGui::GetWindowDrawList();
    draws->AddRectFilled(ImVec2(trackMinX, trackMinY), ImVec2(trackMaxX, trackMaxY),
                         theme.scrollbarTrackColor, rounding);
    draws->AddRectFilled(ImVec2(trackMinX, thumbMinY), ImVec2(trackMaxX, thumbMaxY),
                         thumbColor, rounding);
}

}
