#include "ui/hud/musichud/SourcePicker.h"
#include "ui/clickgui/Fonts.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/backend/blur/Blur.h"
#include "utils/Color.h"
#include "imgui_internal.h"
#include "smtc/SmtcService.h"

namespace nova::ui::music {
void DrawSourcePicker(ImVec2 anchor, float scale, std::string& source,
                      const std::vector<smtc::Source>& sources) {
    if (!ImGui::IsPopupOpen("music-source")) return;
    const auto& theme = menu::panel::ThemeColors(menu::panel::ClientSettings::Get().themeValue());
    const auto& style = ImGui::GetStyle();
    const float fontScale = scale / (style.FontScaleMain * style.FontScaleDpi);
    const float width = std::min(220 * scale, ImGui::GetIO().DisplaySize.x - 12);
    const float maxHeight = std::min(320 * scale, ImGui::GetIO().DisplaySize.y - 12);
    const auto display = ImGui::GetIO().DisplaySize;
    const float expectedHeight = std::min(maxHeight, (90.f + 32.f * sources.size() + (sources.empty() ? 40.f : 0.f)) * scale);
    ImGui::SetNextWindowPos(ImVec2(std::clamp(anchor.x - width, 6.f, std::max(6.f, display.x - width - 6)),
        std::clamp(anchor.y, 6.f, std::max(6.f, display.y - expectedHeight - 6))));
    ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0), ImVec2(width, maxHeight));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6 * scale, 6 * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2 * scale));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 12 * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, scale);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, theme.modulePanelBgColor);
    ImGui::PushStyleColor(ImGuiCol_Border, theme.windowBorderColor);
    ImGui::PushStyleColor(ImGuiCol_Text, theme.mainColor);
    ImGui::PushFont(Medium12, 12 * fontScale);
    if (ui::blur::BeginPopup("music-source", ImGuiWindowFlags_NoScrollbar)) {
        auto* draws = ImGui::GetWindowDrawList();
        auto row = [&](const char* id, const char* label, const char* detail, bool selected) {
            ImGui::PushID(id);
            const auto pos = ImGui::GetCursorScreenPos();
            const ImVec2 size(ImGui::GetContentRegionAvail().x, (detail ? 44.f : 30.f) * scale);
            const bool clicked = ImGui::InvisibleButton("choice", size);
            auto* storage = ImGui::GetStateStorage(); const auto key = ImGui::GetID("hover");
            float hover = storage->GetFloat(key);
            hover += ((ImGui::IsItemHovered() ? 1.f : 0.f) - hover) * (1.f - std::exp(-18.f * ImGui::GetIO().DeltaTime));
            storage->SetFloat(key, hover);
            const auto highlight = selected ? theme.selectorElementActiveColor : theme.selectorElementHoverColor;
            const int alpha = static_cast<int>((highlight >> 24) * (selected ? 1.f : hover));
            draws->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), color::ReAlpha(highlight, alpha), 8 * scale);
            const auto ink = selected ? theme.textHighlightColor : theme.mainColor;
            const float baseline = pos.y + (detail ? 7 * scale : (size.y - ImGui::GetFontSize()) * .5f);
            draws->PushClipRect(ImVec2(pos.x + 10 * scale, pos.y), ImVec2(pos.x + size.x - 25 * scale, pos.y + size.y), true);
            draws->AddText(ImVec2(pos.x + 10 * scale, baseline), ink, label);
            if (detail) {
                ImGui::PushFont(Medium12, 10 * fontScale);
                draws->AddText(ImVec2(pos.x + 10 * scale, pos.y + 25 * scale), theme.secondaryColor, detail);
                ImGui::PopFont();
            }
            draws->PopClipRect();
            if (selected) {
                const ImVec2 check(pos.x + size.x - 15 * scale, pos.y + size.y * .5f);
                draws->AddLine(ImVec2(check.x - 4 * scale, check.y), ImVec2(check.x - scale, check.y + 3 * scale), ink, 1.5f * scale);
                draws->AddLine(ImVec2(check.x - scale, check.y + 3 * scale), ImVec2(check.x + 5 * scale, check.y - 3 * scale), ink, 1.5f * scale);
            }
            if (ImGui::IsItemHovered() && ImGui::CalcTextSize(label).x > size.x - 35 * scale) ImGui::SetTooltip("%s", label);
            ImGui::PopID(); return clicked;
        };
        const auto head = ImGui::GetCursorScreenPos();
        draws->AddText(ImVec2(head.x + 10 * scale, head.y + 6 * scale), theme.secondaryColor, "Media source");
        ImGui::Dummy(ImVec2(0, 28 * scale));
        if (row("auto", "Auto", "Follow Windows media session", source.empty() || source == "Auto")) {
            source = "Auto"; ImGui::CloseCurrentPopup();
        }
        for (size_t i = 0; i < sources.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            if (row("session", sources[i].name.c_str(), nullptr, source == sources[i].id)) {
                source = sources[i].id; ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
        }
        if (sources.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, theme.secondaryColor);
            ImGui::TextWrapped("Enable system media controls in your player.");
            ImGui::PopStyleColor();
        }
        ImGui::EndPopup();
    }
    ImGui::PopFont(); ImGui::PopStyleColor(3); ImGui::PopStyleVar(4);
}

void DrawSourceSelector(SourceProperty& property, float scale) {
    const auto snapshot = smtc::ReadSnapshot();
    auto source = property.get();
    std::string label = source.empty() ? "Auto" : source;
    if (source == "cloudmusic.exe") label = "NetEase Cloud Music";
    for (const auto& item : snapshot->sources) if (source == item.id) { label = item.name; break; }
    const auto& theme = menu::panel::ThemeColors(menu::panel::ClientSettings::Get().themeValue());
    if (ImGui::InvisibleButton("source-selector", ImVec2(180 * scale, 20 * scale)))
        ImGui::OpenPopup("music-source");
    const auto min = ImGui::GetItemRectMin(), max = ImGui::GetItemRectMax();
    const bool hovered = ImGui::IsItemHovered(), open = ImGui::IsPopupOpen("music-source");
    auto* draws = ImGui::GetWindowDrawList();
    draws->AddRectFilled(min, max, theme.selectorBgColor, 6 * scale);
    if (hovered || open) draws->AddRectFilled(min, max, theme.selectorElementHoverColor, 6 * scale);
    const float fontScale = scale / (ImGui::GetStyle().FontScaleMain * ImGui::GetStyle().FontScaleDpi);
    ImGui::PushFont(Medium12, 12 * fontScale);
    draws->PushClipRect(ImVec2(min.x+8*scale,min.y), ImVec2(max.x-22*scale,max.y), true);
    draws->AddText(ImVec2(min.x+8*scale,min.y+(max.y-min.y-ImGui::GetFontSize())*.5f),theme.mainColor,label.c_str());
    draws->PopClipRect();
    const ImVec2 center(max.x-11*scale,(min.y+max.y)*.5f);
    const float direction = open ? -1.f : 1.f;
    draws->AddLine(ImVec2(center.x-3*scale,center.y-direction*1.5f*scale),ImVec2(center.x,center.y+direction*1.5f*scale),theme.mainColor,1.5f*scale);
    draws->AddLine(ImVec2(center.x,center.y+direction*1.5f*scale),ImVec2(center.x+3*scale,center.y-direction*1.5f*scale),theme.mainColor,1.5f*scale);
    if (hovered && ImGui::CalcTextSize(label.c_str()).x > 150*scale) ImGui::SetTooltip("%s",label.c_str());
    ImGui::PopFont();
    DrawSourcePicker(ImVec2(max.x,max.y+4*scale),scale,source,snapshot->sources);
    if (source != property.get()) property.set(std::move(source));
}
}
