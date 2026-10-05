#include "ui/backend/blur/Blur.h"
#include "ui/clickgui/panel/TopBar.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

#include "FatalWidgets.h"
#include "config/ConfigManager.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "ui/clickgui/Fonts.h"
#include "ui/clickgui/panel/PageLayout.h"
#include "ui/clickgui/panel/PanelClickGui.h"
#include "ui/clickgui/panel/Widgets.h"
#include "ui/clickgui/panel/ReferenceStyle.h"
#include "ui/clickgui/panel/ClientSettings.h"

namespace nova::menu::panel {
namespace {

constexpr float kBarHeight = 56.0f;
constexpr float kColumnGap = 11.0f;

constexpr float kWordmarkSize = 28.0f;
constexpr float kWordmarkTracking = 1.2f;
constexpr float kWordmarkShift = 1.5f;
constexpr float kWordmarkCycle = 2.0f;

constexpr float kSelectorHeight = 30.0f;
constexpr float kSelectorRounding = 6.0f;
constexpr float kSelectorPadding = 10.0f;

constexpr float kSearchButton = 28.0f;
constexpr float kSearchField = 150.0f;
constexpr float kSearchGap = 6.0f;

constexpr float kPopupWidth = 268.0f;
constexpr float kPopupRow = 32.0f;
constexpr float kPopupRounding = 10.0f;
constexpr float kPopupAction = 32.0f;
constexpr float kPopupInset = 8.0f;
constexpr float kPopupGap = 6.0f;
constexpr float kPopupListHeight = 168.0f;
constexpr double kStatusSeconds = 3.0;

using IconDraw = void (*)(ImDrawList*, ImVec2, float, ImU32);

void DrawFile(ImDrawList* draws, ImVec2 center, float size, ImU32 color) {
    const float half = size / 2.0f;
    const float fold = size * 0.32f;
    const ImVec2 points[5] = {
        ImVec2(center.x - half, center.y - half),
        ImVec2(center.x + half - fold, center.y - half),
        ImVec2(center.x + half, center.y - half + fold),
        ImVec2(center.x + half, center.y + half),
        ImVec2(center.x - half, center.y + half),
    };
    draws->AddConvexPolyFilled(points, 5, color);
}

void DrawFolder(ImDrawList* draws, ImVec2 center, float size, ImU32 color) {
    const float half = size / 2.0f;
    const ImVec2 bodyMin(center.x - half, center.y - half * 0.65f);
    const ImVec2 bodyMax(center.x + half, center.y + half * 0.85f);
    draws->AddRectFilled(ImVec2(bodyMin.x + Scale(1.0f), bodyMin.y - size * 0.28f),
                         ImVec2(bodyMin.x + size * 0.5f, bodyMin.y + Scale(1.0f)), color, Scale(1.0f));
    draws->AddRectFilled(bodyMin, bodyMax, color, Scale(1.5f));
}

void DrawPencil(ImDrawList* draws, ImVec2 center, float size, ImU32 color) {
    const float half = size / 2.0f;
    const ImVec2 tip(center.x - half, center.y + half);
    draws->AddLine(tip, ImVec2(center.x + half, center.y - half), color, size * 0.28f);
    draws->AddTriangleFilled(tip, ImVec2(tip.x, tip.y - size * 0.36f),
                             ImVec2(tip.x + size * 0.36f, tip.y), color);
}

void DrawCopy(ImDrawList* draws, ImVec2 center, float size, ImU32 color) {
    const float half = size / 2.0f;
    draws->AddRect(ImVec2(center.x - half, center.y - half),
                   ImVec2(center.x + half * 0.35f, center.y + half * 0.35f), color, Scale(1.5f), 0, Scale(1.6f));
    draws->AddRectFilled(ImVec2(center.x - half * 0.35f, center.y - half * 0.35f),
                         ImVec2(center.x + half, center.y + half), color, Scale(1.5f));
}

void DrawTrash(ImDrawList* draws, ImVec2 center, float size, ImU32 color) {
    const float half = size / 2.0f;
    const float lid = center.y - half * 0.45f;
    draws->AddRectFilled(ImVec2(center.x - half, lid), ImVec2(center.x + half, lid + size * 0.15f),
                         color, Scale(1.0f));
    draws->AddRectFilled(ImVec2(center.x - half * 0.4f, center.y - half),
                         ImVec2(center.x + half * 0.4f, lid), color, Scale(1.0f));
    draws->AddRectFilled(ImVec2(center.x - half * 0.78f, lid + size * 0.24f),
                         ImVec2(center.x + half * 0.78f, center.y + half), color, size * 0.14f);
}

void DrawPlus(ImDrawList* draws, ImVec2 center, float size, ImU32 color) {
    const float half = size / 2.0f;
    draws->AddLine(ImVec2(center.x - half, center.y), ImVec2(center.x + half, center.y), color,
                   Scale(1.8f));
    draws->AddLine(ImVec2(center.x, center.y - half), ImVec2(center.x, center.y + half), color,
                   Scale(1.8f));
}

void DrawDots(ImDrawList* draws, ImVec2 center, float size, ImU32 color) {
    for (int index = -1; index <= 1; ++index) {
        draws->AddCircleFilled(
            ImVec2(center.x + static_cast<float>(index) * size * 0.34f, center.y), size * 0.10f,
            color);
    }
}

void DrawMagnifier(ImDrawList* draws, ImVec2 center, float size, ImU32 color) {
    const float unit = size / 14.0f;
    draws->AddCircle(center, 5.0f * unit, color, 0, 2.0f * unit);
    draws->AddLine(center + ImVec2(4, 4) * unit,
                   center + ImVec2(8, 8) * unit, color, 2.0f * unit);
}

float WordmarkWidth(const char* text) {
    float width = 0.0f;
    const float tracking=ImGui::GetFontSize()*(kWordmarkTracking/kWordmarkSize);
    for (const char* glyph = text; *glyph != '\0';) {
        const char* end=glyph+1;
        while((static_cast<unsigned char>(*end)&0xc0)==0x80)++end;
        width += ImGui::CalcTextSize(glyph,end).x + tracking;
        glyph=end;
    }
    return width > 0.0f ? width - tracking : 0.0f;
}

void DrawWordmark(ImDrawList* draws, const char* text, ImVec2 position, ImU32 color) {
    for (const char* glyph = text; *glyph != '\0';) {
        const char* end=glyph+1;
        while((static_cast<unsigned char>(*end)&0xc0)==0x80)++end;
        draws->AddText(nullptr,0,position,color,glyph,end);
        position.x += ImGui::CalcTextSize(glyph,end).x + ImGui::GetFontSize()*(kWordmarkTracking/kWordmarkSize);
        glyph=end;
    }
}

ImVec2 WordmarkSize(const char* text) {
    return ImVec2(WordmarkWidth(text), ImGui::GetTextLineHeight());
}

FatalWidgets::TextRenderer kWordmarkRenderer{&DrawWordmark, &WordmarkSize, &WordmarkWidth,
                                           &ImGui::GetTextLineHeight};

void WriteBuffer(char* buffer, std::size_t size, const std::string& text) {
    const std::size_t count = std::min(text.size(), size - 1);
    if (count > 0)
        std::memcpy(buffer, text.data(), count);
    buffer[count] = '\0';
}

bool ActionItem(const char* label, IconDraw icon, Color textColor, Color iconColor, Color hover) {
    const float width = Scale(kPopupWidth) - Scale(kPopupInset) * 2.0f;
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const ImVec2 max(min.x + width, min.y + Scale(kPopupAction));

    ImGui::InvisibleButton(label, ImVec2(width, Scale(kPopupAction)));
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();

    if (hovered)
        ImGui::GetWindowDrawList()->AddRectFilled(min, max, hover, Scale(kPopupRounding));

    icon(ImGui::GetWindowDrawList(), ImVec2(min.x + Scale(13.0f), min.y + Scale(kPopupAction) / 2.0f), Scale(13.0f),
         iconColor);

    PushText(kTextBody);
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(min.x + Scale(28.0f), min.y + (Scale(kPopupAction) - TextHeight()) / 2.0f), textColor, label);
    PopFont();

    return clicked;
}

}

TopBar::TopBar(PanelClickGui& gui) : PanelComponent(gui) {}

void TopBar::render() {
    const bool browser=gui_.layout().page()==PageLayout::BrowserPage;
    ImGui::BeginChild("topbar", ImVec2(browser?Scale(158):0.0f, Scale(kBarHeight)), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);

    barOriginX_ = ImGui::GetCursorScreenPos().x;
    barOriginY_ = ImGui::GetCursorScreenPos().y;
    barRight_ = barOriginX_ + ImGui::GetContentRegionAvail().x;
    barCenterY_ = barOriginY_ + Scale(kBarHeight) / 2.0f;

    auto* d = ImGui::GetWindowDrawList();
    const ImVec2 base(barOriginX_,barOriginY_);
    const bool dark=color::Red(gui_.colors().windowBgColor)<100;
    if(browser){renderTitle();ImGui::EndChild();return;}
    d->AddRectFilled(base+ImVec2(Scale(158),0),base+ImVec2(Scale(748),Scale(56)),dark?IM_COL32(11,13,20,225):IM_COL32(240,243,249,225),Scale(14),ImDrawFlags_RoundCornersTopRight);
    d->AddLine(base+ImVec2(Scale(158),Scale(56)),base+ImVec2(Scale(748),Scale(56)),gui_.colors().splitColor,Scale(1));
    renderTitle();
    if(gui_.layout().page()==PageLayout::ProxyPage) {
        PushText(kTextBody);
        d->AddText(base+ImVec2(Scale(179),(Scale(kBarHeight)-TextHeight())*.5f),gui_.colors().mainColor,"Proxy");
        PopFont();ImGui::EndChild();return;
    }
    if(gui_.layout().page()==PageLayout::AccountsPage) {
        PushText(kTextBody);
        d->AddText(base+ImVec2(Scale(179),(Scale(kBarHeight)-TextHeight())*.5f),gui_.colors().mainColor,"Accounts");
        PopFont();renderSearch();ImGui::EndChild();return;
    }
    renderSelector();
    renderSearch();
    renderSelectorPopup();

    ImGui::EndChild();
}

void TopBar::renderTitle() {
    const auto& t = gui_.colors();
    const auto& logoTheme = ThemeColors(Theme::Dark);
    auto* d = ImGui::GetWindowDrawList();
    const ImVec2 b(barOriginX_,barOriginY_);

    ImGui::PushFont(ExtraBold20, kWordmarkSize);
    const auto name=ClientSettings::Get().clientName();
    const float naturalWidth=WordmarkWidth(name.c_str());
    if(naturalWidth>Scale(138)) {
        PopFont();
        ImGui::PushFont(ExtraBold20,kWordmarkSize*Scale(138)/naturalWidth);
    }
    const float width = WordmarkWidth(name.c_str());
    d->PushClipRect(b+ImVec2(Scale(8),0),b+ImVec2(Scale(150),Scale(56)),true);
    FatalWidgets::Rendering::DrawFatalTitle(
        name.c_str(),
        ImVec2(barOriginX_ + (Scale(158) - width) / 2.0f,
               barCenterY_ - TextHeight() / 2.0f),
        fmodf(static_cast<float>(ImGui::GetTime()) / kWordmarkCycle, 1.0f), Scale(kWordmarkShift),
        logoTheme.clientNameColor, logoTheme.iconHighlightColor, logoTheme.clientIconBgColor,
        d, &kWordmarkRenderer);
    d->PopClipRect();
    PopFont();
    d->AddLine(b+ImVec2(Scale(10),Scale(56)),b+ImVec2(Scale(147),Scale(56)),t.splitColor,Scale(1));
}

void TopBar::renderSelector() {
    const ColorTheme& theme = gui_.colors();
    const std::string& current = config::ConfigManager::Get().CurrentName();

    PushText(kTextBody);
    const float textWidth = TextWidth(current.c_str());
    PopFont();

    const float openWidth = Scale(136.0f);
    const ImVec2 min(barOriginX_ + Scale(158) + Scale(kColumnGap),
                     barCenterY_ - Scale(kSelectorHeight) / 2.0f);
    const float splitX = min.x + Scale(kSelectorHeight);
    const ImVec2 max(splitX + openWidth, min.y + Scale(kSelectorHeight));

    ImGui::SetCursorScreenPos(min);
    ImGui::InvisibleButton("configsave", ImVec2(Scale(kSelectorHeight), Scale(kSelectorHeight)));
    const bool saveHovered = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked()) {
        config::ConfigManager::Get().SaveCurrent();
        statusAt_ = ImGui::GetTime();
    }

    const ImVec2 openMin(splitX, min.y);
    ImGui::SetCursorScreenPos(openMin);
    ImGui::InvisibleButton("configopen", ImVec2(openWidth, Scale(kSelectorHeight)));
    const bool openHovered = ImGui::IsItemHovered();
    const bool openClicked = ImGui::IsItemClicked();

    ImDrawList* draws = ImGui::GetWindowDrawList();
    draws->AddRectFilled(min, max, color::ReAlpha(theme.modulePanelBgColor,255), Scale(kSelectorRounding));
    if (saveHovered) {
        draws->AddRectFilled(min, ImVec2(splitX, max.y), theme.buttonHoveredBgColor,
                             Scale(kSelectorRounding), ImDrawFlags_RoundCornersLeft);
    }
    if (openHovered) {
        draws->AddRectFilled(openMin, max, theme.buttonHoveredBgColor, Scale(kSelectorRounding),
                             ImDrawFlags_RoundCornersRight);
    }

    PushPanelIcon(13);
    draws->AddText(CenterText(Icon(u8"\uf0c7"),min.x+Scale(19),barCenterY_,true,true), saveHovered?theme.iconHighlightColor:theme.mainColor,Icon(u8"\uf0c7"));
    PopFont();
    PushText(kTextControl);
    draws->PushClipRect(min+ImVec2(Scale(48),0),max-ImVec2(Scale(22),0),true);
    draws->AddText(ImVec2(min.x+Scale(48),barCenterY_-TextHeight()/2),theme.mainColor,current.c_str());
    draws->PopClipRect(); PopFont();

    Chevron(draws,min+ImVec2(Scale(148),Scale(11)),theme.secondaryColor);

    draws->AddRect(min, max, theme.splitColor, Scale(kSelectorRounding), 0, Scale(1.0f));

    if (openClicked) {
        ImGui::SetNextWindowPos(ImVec2(min.x, max.y + Scale(6.0f)), ImGuiCond_Appearing);
        ImGui::OpenPopup("configpopup");
    }
}

void TopBar::renderSelectorPopup() {
    const ColorTheme& theme = gui_.colors();

    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, Scale(12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, Scale(1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Scale(kPopupInset), Scale(kPopupInset)));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, Scale(kPopupGap)));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, theme.modulePanelBgColor);
    ImGui::PushStyleColor(ImGuiCol_Border, theme.windowBorderColor);
    // Native context-menu items inherit these colors from the config popup.
    ImGui::PushStyleColor(ImGuiCol_Text, theme.mainColor);
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, theme.secondaryColor);
    ImGui::PushStyleColor(ImGuiCol_Header, theme.selectorElementActiveColor);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, theme.selectorElementHoverColor);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, theme.selectorElementActiveColor);

    if (BeginPanelPopup(theme,"configpopup")) {
        renderConfigList();
        EndPanelPopup();
    } else if (renaming_ >= 0) {
        cancelEdit();
    }

    ImGui::PopStyleColor(7);
    ImGui::PopStyleVar(4);
}

void TopBar::renderConfigList() {
    const ColorTheme& theme = gui_.colors();
    config::ConfigManager& manager = config::ConfigManager::Get();
    MenuAction action;

    renderCreateRow();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, Scale(2.0f)));
    ImGui::BeginChild("configlist", ImVec2(Scale(kPopupWidth), Scale(kPopupListHeight)),
                      ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);

    if (manager.All().empty() && renaming_ < 0) {
        PushText(kTextSmall);
        ImGui::PushStyleColor(ImGuiCol_Text, theme.secondaryColor);
        ImGui::TextUnformatted("No configs yet");
        ImGui::PopStyleColor();
        PopFont();
    }

    for (int index = 0; index < static_cast<int>(manager.All().size()); ++index) {
        if (renaming_ == index) {
            renderEditRow();
            continue;
        }
        renderConfigRow(index, action);
    }

    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(1)) ImGui::OpenPopup("listmenu");
    if (BeginPanelPopup(theme,"listmenu")) {
        if (Choice(theme,"refresh","Refresh",false,Scale(kPopupWidth)))
            action.refresh = true;
        if (Choice(theme,"folder","Open folder",false,Scale(kPopupWidth)))
            action.openFolder = true;
        EndPanelPopup();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();

    renderStatus();

    if (action.overwrite) {
        manager.SaveAs(action.targetName);
        statusAt_ = ImGui::GetTime();
    }
    if (action.duplicate) {
        manager.Duplicate(action.targetName);
        statusAt_ = ImGui::GetTime();
    }
    if (action.remove) {
        manager.Delete(action.targetName);
        statusAt_ = ImGui::GetTime();
    }
    if (action.refresh) {
        manager.Refresh();
        statusAt_ = ImGui::GetTime();
    }
    if (action.openFolder)
        manager.OpenFolder();
    if (action.renameIndex >= 0)
        beginRename(action.renameIndex);
}

void TopBar::renderConfigRow(int index, MenuAction& action) {
    const ColorTheme& theme = gui_.colors();
    config::ConfigManager& manager = config::ConfigManager::Get();
    const std::string name = manager.All()[static_cast<std::size_t>(index)].name();
    const bool selected = name == manager.CurrentName();

    ImGui::PushID(index);

    const ImVec2 rowMin = ImGui::GetCursorScreenPos();
    const ImVec2 rowMax(rowMin.x + Scale(kPopupWidth), rowMin.y + Scale(kPopupRow));

    ImGui::SetNextItemAllowOverlap();
    ImGui::InvisibleButton("row", ImVec2(Scale(kPopupWidth), Scale(kPopupRow)));
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();

    ImDrawList* draws = ImGui::GetWindowDrawList();
    const float hover=Motion(ImGui::GetID("##rowhover"),hovered?1.f:0.f,22);
    draws->AddRectFilled(rowMin,rowMax,color::ReAlpha(theme.selectorElementHoverColor,int(color::Alpha(theme.selectorElementHoverColor)*hover)),Scale(10));
    if(selected) Check(draws,rowMin+ImVec2(Scale(14),Scale(16)),theme.textHighlightColor);
    PushText(kTextControl);
    draws->PushClipRect(rowMin,rowMax-ImVec2(Scale(32),0),true);
    draws->AddText(rowMin+ImVec2(Scale(35),(Scale(32)-TextHeight())*.5f),selected?theme.textHighlightColor:theme.mainColor,name.c_str());
    draws->PopClipRect(); PopFont();

    const float dotsX = rowMax.x - Scale(kPopupAction) / 2.0f - Scale(4.0f);
    ImGui::SetCursorScreenPos(ImVec2(dotsX - Scale(kPopupAction) / 2.0f, rowMin.y));
    ImGui::InvisibleButton("rowactions", ImVec2(Scale(kPopupAction), Scale(kPopupRow)));
    const bool dotsHovered = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked())
        ImGui::OpenPopup("rowmenu");
    DrawDots(draws, ImVec2(dotsX, rowMin.y + Scale(kPopupRow) / 2.0f), Scale(15.0f),
             dotsHovered ? theme.textHighlightColor : theme.secondaryColor);

    if (BeginPanelPopup(theme,"rowmenu")) {
        if (ActionItem("Overwrite", &DrawFile, theme.mainColor, theme.secondaryColor,
                       theme.selectorElementHoverColor)) {
            action.overwrite = true;
            action.targetName = name;
        }
        if (ActionItem("Rename", &DrawPencil, theme.mainColor, theme.secondaryColor,
                       theme.selectorElementHoverColor))
            action.renameIndex = index;
        if (ActionItem("Duplicate", &DrawCopy, theme.mainColor, theme.secondaryColor,
                       theme.selectorElementHoverColor)) {
            action.duplicate = true;
            action.targetName = name;
        }
        if (ActionItem("Open folder", &DrawFolder, theme.mainColor, theme.secondaryColor,
                       theme.selectorElementHoverColor))
            action.openFolder = true;
        if (ActionItem("Delete", &DrawTrash, IM_COL32(0xF0, 0x6A, 0x6A, 0xFF),
                       IM_COL32(0xF0, 0x6A, 0x6A, 0xFF), theme.selectorElementHoverColor)) {
            action.remove = true;
            action.targetName = name;
        }
        EndPanelPopup();
    }

    ImGui::PopID();

    if (clicked && !selected && ImGui::GetIO().MousePos.x < dotsX - Scale(kPopupAction)) {
        manager.LoadByName(name);
        ImGui::CloseCurrentPopup();
    }
}

void TopBar::renderCreateRow() {
    const ColorTheme& theme = gui_.colors();
    config::ConfigManager& manager = config::ConfigManager::Get();

    const float fieldWidth = Scale(kPopupWidth) - Scale(kPopupAction) - Scale(kPopupGap);
    const bool entered =
        configInput("##newconfig", "Config Name", createBuffer_, sizeof(createBuffer_), fieldWidth);
    const float fieldHeight = ImGui::GetItemRectSize().y;

    ImGui::SameLine(0.0f, Scale(kPopupGap));
    const ImVec2 buttonMin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("createconfig", ImVec2(Scale(kPopupAction), fieldHeight));
    const bool buttonHovered = ImGui::IsItemHovered();
    const bool pressed = ImGui::IsItemClicked();

    ImGui::GetWindowDrawList()->AddRectFilled(
        buttonMin, buttonMin + ImVec2(Scale(kPopupAction), fieldHeight),
        buttonHovered ? theme.buttonHoveredBgColor : theme.selectorBgColor, Scale(kPopupRounding));
    DrawPlus(ImGui::GetWindowDrawList(),
             buttonMin + ImVec2(Scale(kPopupAction) / 2.0f, fieldHeight / 2.0f), Scale(14.0f),
             buttonHovered ? theme.iconHighlightColor : theme.textHighlightColor);

    if (entered || pressed) {
        const std::string wanted = config::SanitizeConfigName(createBuffer_);
        if (!wanted.empty() && manager.SaveAs(wanted)) {
            createBuffer_[0] = '\0';
            statusAt_ = ImGui::GetTime();
        }
    }
}

void TopBar::renderEditRow() {
    if (focusEdit_) {
        ImGui::SetKeyboardFocusHere();
        focusEdit_ = false;
    }

    const bool entered =
        configInput("##configname", "config name", nameBuffer_, sizeof(nameBuffer_), Scale(kPopupWidth));

    if (entered)
        commitRename();
    else if (ImGui::IsItemDeactivated())
        cancelEdit();
}

void TopBar::renderStatus() {
    const ColorTheme& theme = gui_.colors();
    config::ConfigManager& manager = config::ConfigManager::Get();

    if (manager.StatusSerial() != seenStatus_) {
        seenStatus_ = manager.StatusSerial();
        statusAt_ = ImGui::GetTime();
    }
    if (manager.Status().empty() || ImGui::GetTime() - statusAt_ >= kStatusSeconds)
        return;

    PushText(kTextSmall);
    ImGui::PushStyleColor(ImGuiCol_Text, theme.secondaryColor);
    ImGui::TextUnformatted(manager.Status().c_str());
    ImGui::PopStyleColor();
    PopFont();
}

bool TopBar::configInput(const char* id, const char* hint, char* buffer, std::size_t size,
                         float width) {
    const ColorTheme& theme = gui_.colors();

    PushText(kTextBody);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Scale(kPopupRounding));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(Scale(8.0f), Scale(6.0f)));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.selectorBgColor);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, theme.selectorBgColor);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, theme.selectorBgColor);
    ImGui::PushStyleColor(ImGuiCol_Text, theme.mainColor);
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, theme.secondaryColor);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, color::ReAlpha(theme.iconHighlightColor, reference::kFieldSelectionAlpha));
    ImGui::PushStyleColor(ImGuiCol_NavCursor, 0u);

    ImGui::SetNextItemWidth(width);
    const bool entered = ImGui::InputTextWithHint(id, hint, buffer, size,
                                                  ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::PopStyleColor(7);
    ImGui::PopStyleVar(3);
    PopFont();
    return entered;
}

void TopBar::beginRename(int index) {
    config::ConfigManager& manager = config::ConfigManager::Get();
    if (index < 0 || index >= static_cast<int>(manager.All().size()))
        return;

    renaming_ = index;
    WriteBuffer(nameBuffer_, sizeof(nameBuffer_),
                manager.All()[static_cast<std::size_t>(index)].name());
    focusEdit_ = true;
}

void TopBar::commitRename() {
    config::ConfigManager& manager = config::ConfigManager::Get();
    if (renaming_ >= 0 && renaming_ < static_cast<int>(manager.All().size())) {
        const std::string before = manager.All()[static_cast<std::size_t>(renaming_)].name();
        const std::string wanted = config::SanitizeConfigName(nameBuffer_);
        if (!wanted.empty() && manager.Rename(before, wanted))
            statusAt_ = ImGui::GetTime();
    }

    cancelEdit();
}

void TopBar::cancelEdit() {
    renaming_ = -1;
    focusEdit_ = false;
    nameBuffer_[0] = '\0';
}

void TopBar::renderSearch() {
    const ColorTheme& theme = gui_.colors();
    const int frame = ImGui::GetFrameCount();
    const bool consecutive = searchFrame_ == frame - 1;
    const float delta = ImGui::GetIO().DeltaTime;
    bool open = gui_.isSearching();
    if (!consecutive) {
        searchReveal_ = open ? 1.0f : 0.0f;
        searchHighlight_ = open ? 1.0f : 0.0f;
        searchFocus_ = 0.0f;
        searchOpen_ = open;
        focusSearch_ = open;
    }
    searchFrame_ = frame;

    // The panel owns the query. Module/page selection may clear it between frames.
    if (gui_.search() != searchBuffer_)
        WriteBuffer(searchBuffer_, sizeof(searchBuffer_), gui_.search());

    const ImVec2 buttonMin(barOriginX_ + Scale(705), barCenterY_ - Scale(kSearchButton) / 2.0f);
    ImGui::SetCursorScreenPos(buttonMin);
    ImGui::InvisibleButton("searchtoggle", ImVec2(Scale(kSearchButton), Scale(kSearchButton)));
    const bool hovered = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked()) {
        open = !open;
        gui_.setSearching(open);
        if (!open) searchBuffer_[0] = '\0';
    }

    if (open != searchOpen_) focusSearch_ = open;
    searchOpen_ = open;
    const float target = open ? 1.0f : 0.0f;
    const float blend = 1.0f - std::exp(-20.0f * delta);
    searchReveal_ = std::lerp(searchReveal_, target, blend);
    if (std::abs(searchReveal_ - target) < .0005f) searchReveal_ = target;
    const float reveal = reference::PopupEase(searchReveal_);
    searchHighlight_ = std::lerp(searchHighlight_, open || hovered ? 1.0f : 0.0f,
                                 1.0f - std::exp(-16.0f * delta));
    DrawMagnifier(ImGui::GetWindowDrawList(),
                  buttonMin + ImVec2(Scale(kSearchButton) / 2.0f, Scale(kSearchButton) / 2.0f), Scale(14.0f),
                  reference::Mix(theme.secondaryColor, theme.iconHighlightColor, searchHighlight_ * .35f));

    const float width = Scale(kSearchField) * reveal;
    if (width < Scale(1.0f)) {
        if (!open) searchBuffer_[0] = '\0';
        return;
    }

    // Keep text layout fixed while the visible area unfolds from the right.
    PushText(kTextControl);
    const float height = Scale(kSelectorHeight);
    const float right = buttonMin.x - Scale(kSearchGap);
    const ImVec2 fieldMin(right - width, barCenterY_ - height / 2.0f);
    const ImVec2 fieldMax(right, fieldMin.y + height);
    const float rounding = std::min(Scale(kSelectorRounding), width * 0.5f);
    auto* draws = ImGui::GetWindowDrawList();
    // Match the config selector: opaque panel base, neutral hover overlay,
    // and a fixed separator border instead of an accent-colored focus ring.
    draws->AddRectFilled(fieldMin, fieldMax, color::ReAlpha(theme.modulePanelBgColor,255), rounding);
    draws->AddRectFilled(fieldMin, fieldMax,
        color::ReAlpha(theme.buttonHoveredBgColor,
            int(color::Alpha(theme.buttonHoveredBgColor) * searchFocus_)), rounding);
    ImGui::PushClipRect(fieldMin, fieldMax, true);
    // Center actual ink, including the lowercase hint, rather than the font's
    // line box (whose extra descent makes this font look too low in InputText).
    auto* baked = ImGui::GetFontBaked();
    float inkTop = TextHeight(), inkBottom = 0;
    const float glyphScale = ImGui::GetFontSize() / baked->Size;
    const char* hint=gui_.layout().page()==PageLayout::AccountsPage?"search accounts":"search modules";
    for (const char* at = searchBuffer_[0] ? searchBuffer_ : hint; *at;) {
        unsigned int codepoint = 0;
        const int bytes = ImTextCharFromUtf8(&codepoint, at, nullptr);
        if (bytes <= 0) break;
        at += bytes;
        const auto* glyph = baked->FindGlyph(codepoint);
        if (!glyph || !glyph->Visible) continue;
        inkTop = std::min(inkTop, glyph->Y0 * glyphScale);
        inkBottom = std::max(inkBottom, glyph->Y1 * glyphScale);
    }
    const float paddingY = std::max(0.f,(height-TextHeight())*.5f);
    const float inkOffset = inkBottom > inkTop ? (height-inkTop-inkBottom)*.5f-paddingY : 0.f;
    ImGui::SetCursorScreenPos(ImVec2(right - Scale(kSearchField), std::round(fieldMin.y+inkOffset)));
    ImGui::SetNextItemWidth(Scale(kSearchField));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Scale(kSelectorRounding));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(Scale(8.0f), paddingY));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, 0u);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, 0u);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, 0u);
    ImGui::PushStyleColor(ImGuiCol_Text, theme.mainColor);
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, theme.secondaryColor);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, color::ReAlpha(theme.iconHighlightColor, reference::kFieldSelectionAlpha));
    ImGui::PushStyleColor(ImGuiCol_NavCursor, 0u);

    ImGui::BeginDisabled(!open);
    if (focusSearch_) {
        ImGui::SetKeyboardFocusHere();
    }
    if (ImGui::InputTextWithHint("##search", hint, searchBuffer_, sizeof(searchBuffer_)) && open)
        gui_.setSearch(searchBuffer_);
    const float focus = open && (ImGui::IsItemActive() || ImGui::IsItemHovered()) ? 1.0f : 0.0f;
    if (ImGui::IsItemActive()) focusSearch_ = false;
    searchFocus_ += (focus - searchFocus_) * blend;
    ImGui::EndDisabled();

    ImGui::PopStyleColor(7);
    ImGui::PopStyleVar(4);
    ImGui::PopClipRect();
    draws->AddRect(fieldMin, fieldMax, theme.splitColor, rounding, 0, Scale(1.0f));
    PopFont();
}

}
