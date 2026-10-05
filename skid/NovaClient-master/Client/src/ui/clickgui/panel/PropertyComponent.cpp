#include "ui/backend/blur/Blur.h"
#include "ui/clickgui/panel/PropertyComponent.h"

#include <algorithm>
#include <limits>

#include "imgui.h"
#include "ui/clickgui/FeatureTree.h"
#include "ui/clickgui/panel/PanelClickGui.h"
#include "ui/clickgui/panel/Widgets.h"

namespace nova::menu::panel {
namespace {

constexpr float kRowHeight = 37.0f;

constexpr float kLabelGap = 6.0f;
constexpr float kGroupButton = 20.0f;
constexpr float kGroupGap = 2.0f;
constexpr float kControlOverhang = 7.0f;

}

PropertyComponent::PropertyComponent(PanelClickGui& gui, const Properties::PropDesc& descriptor)
    : gui_(gui), descriptor_(descriptor), property_(descriptor.getProp()) {
    if (property_ != nullptr)
        element_ = MakeElement(gui, *property_);

    for (const Properties::PropDesc& child : descriptor_.getSubProps()) {
        if (child.getProp() != nullptr && child.getProp() == property_)
            continue;
        children_.push_back(std::make_unique<PropertyComponent>(gui, child));
    }
    if (const auto* feature = FeatureForToggle(property_); feature && feature->hiddenInArrayList) {
        moduleMenuRow_ = std::make_unique<PropertyComponent>(gui,
            Properties::PropDesc("Hide in ArrayList", feature->hiddenInArrayList));
    }
}

PropertyComponent::~PropertyComponent() = default;

void PropertyComponent::render() {
    const ColorTheme& theme = gui_.colors();

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, 0u);
    // Group-only rows have no property, and the same property can appear in
    // multiple rows. Each persistent component needs its own widget scope.
    ImGui::PushID(this);
    const std::string rowName = std::string(descriptor_.getName()) + "###row";
    ImGui::BeginChild(rowName.c_str(), ImVec2(0.0f, Scale(kRowHeight)), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    const float rowY = ImGui::GetCursorPosY();
    // Leave space inside the child clip for switch halos and font overhang.
    const float rowWidth = ImGui::GetContentRegionAvail().x - Scale(kControlOverhang);
    const bool hasGroup = !children_.empty();
    const float groupWidth = hasGroup ? Scale(kGroupButton+kGroupGap) : 0;
    const float elementWidth = element_ ? element_->width(*property_) : 0;
    const float elementX = rowWidth-groupWidth-elementWidth;
    PushText(kTextBody);
    const auto p=ImGui::GetCursorScreenPos();
    auto* d=ImGui::GetWindowDrawList();
    const float labelWidth = std::max(1.f, elementX - Scale(kLabelGap));
    const char* label = descriptor_.getName();
    ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, false, labelWidth);
    const bool compact = labelSize.y > Scale(kRowHeight - 4);
    if (compact) {
        PopFont();
        PushText(kTextSmall);
        labelSize = ImGui::CalcTextSize(label, nullptr, false, labelWidth);
    }
    d->PushClipRect(p,p+ImVec2(labelWidth,Scale(kRowHeight)),true);
    d->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
               p+ImVec2(0,std::max(0.f,(Scale(kRowHeight)-labelSize.y)*.5f-Scale(1.f))),
               theme.mainColor,label,nullptr,labelWidth);
    d->PopClipRect();
    if(TextWidth(descriptor_.getName())>elementX-Scale(kLabelGap) &&
       ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(p,p+ImVec2(std::max(0.f,elementX-Scale(kLabelGap)),Scale(kRowHeight))))
        ImGui::SetTooltip("%s",descriptor_.getName());
    PopFont();
    if (element_) {
        ImGui::SetCursorPos({std::max(0.f,elementX),rowY+(Scale(kRowHeight)-element_->height(*property_))*.5f});
        element_->draw(*property_);
    }
    if (hasGroup) {
        ImGui::SetCursorPos({rowWidth-Scale(kGroupButton),rowY+(Scale(kRowHeight)-Scale(kGroupButton))*.5f});
        renderGroupPopupButton();
    }
    renderModulePopup();

    ImGui::EndChild();
    ImGui::PopID();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void PropertyComponent::renderModulePopup() {
    if (!moduleMenuRow_) return;
    // Keep OpenPopup and BeginPopup in this module row's window/ID scope.
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("modulemenu");

    const auto& theme = gui_.colors();
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, Scale(12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, Scale(1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Scale(12.0f), Scale(4.0f)));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, theme.modulePanelBgColor);
    ImGui::PushStyleColor(ImGuiCol_Border, theme.windowBorderColor);
    ImGui::SetNextWindowSizeConstraints(ImVec2(Scale(240.0f), 0.0f), ImVec2(Scale(240.0f), std::numeric_limits<float>::max()));
    // Let the single-row menu fit its content instead of preserving a stale height.
    if (BeginPanelPopup(theme, "modulemenu", ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        moduleMenuRow_->render();
        EndPanelPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

void PropertyComponent::renderGroupPopupButton() {
    const ColorTheme& theme = gui_.colors();

    if (ImGui::InvisibleButton("group", ImVec2(Scale(kGroupButton), Scale(kGroupButton)))) {
        ImGui::OpenPopup("grouppopup");
    }

    const bool open = ImGui::IsPopupOpen("grouppopup");
    PushPanelIcon(13);
    ImGui::GetWindowDrawList()->AddText(ImGui::GetItemRectMin() + ImVec2(Scale(2.0f), Scale(3.0f)),
                                        open ? theme.iconHighlightColor : theme.mainColor,
                                        Icon(u8"\uf013"));
    PopFont();

    if (open)
        renderGroupPopup(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

void PropertyComponent::renderGroupPopup(ImVec2 anchorMin, ImVec2 anchorMax) {
    const ColorTheme& theme = gui_.colors();
    const auto* viewport=ImGui::GetMainViewport();
    const ImVec2 safeMin=viewport->WorkPos+ImVec2(Scale(12),Scale(12));
    const ImVec2 safeMax=viewport->WorkPos+viewport->WorkSize-ImVec2(Scale(12),Scale(12));
    const float outerWidth=std::min(std::max(ImGui::GetWindowWidth(),Scale(340))+Scale(24),
        std::max(1.f,safeMax.x-safeMin.x));
    const float width=std::max(1.f,outerWidth-Scale(24));
    const auto visibleRows = std::count_if(children_.begin(), children_.end(),
        [](const auto& child) { return child->isVisible(); });
    const float contentHeight = static_cast<float>(visibleRows) *
        (Scale(kRowHeight) + ImGui::GetStyle().ItemSpacing.y);
    const float height=std::min(std::max(Scale(kRowHeight),contentHeight),
        std::max(1.f,std::min(viewport->WorkSize.y*.7f,safeMax.y-safeMin.y-Scale(16))));
    const ImVec2 popupSize(outerWidth,height+Scale(16));
    const float gap=Scale(8);
    const float right=anchorMax.x+gap;
    const float left=anchorMin.x-gap-popupSize.x;
    float x=right;
    if(right+popupSize.x>safeMax.x) {
        x=left>=safeMin.x?left:
            (safeMax.x-anchorMax.x>=anchorMin.x-safeMin.x?right:left);
    }
    const float y=(anchorMin.y+anchorMax.y)*.5f-Scale(8+kRowHeight*.5f);
    const ImVec2 position(std::clamp(x,safeMin.x,std::max(safeMin.x,safeMax.x-popupSize.x)),
        std::clamp(y,safeMin.y,std::max(safeMin.y,safeMax.y-popupSize.y)));
    // Auto-place on opening only; preserve native popup dragging afterwards.
    ImGui::SetNextWindowPos(position, ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(popupSize);

    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, Scale(12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, Scale(1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Scale(12.0f), Scale(4.0f)));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, theme.modulePanelBgColor);
    ImGui::PushStyleColor(ImGuiCol_Border, theme.windowBorderColor);

    if (BeginPanelPopup(theme, "grouppopup", ImGuiWindowFlags_NoScrollbar, PopupMotion::Dropdown, ImVec2(12,8))) {
        // Balance the row's reserved control overhang with matching left space.
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Scale(kControlOverhang));
        ImGui::BeginChild("grouppanel", ImVec2(width - Scale(kControlOverhang), height), ImGuiChildFlags_None,
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
        for (const auto& child : children_)
            if (child->isVisible())
                child->render();
        ImGui::EndChild();
        EndPanelPopup();
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

}
