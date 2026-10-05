#pragma once
#include "ui/hud/world/ScreenGeometry.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/clickgui/panel/Theme.h"

namespace nova::ui::throwableaura {
// Screen geometry only; no Minecraft/JNI dependencies in this view.
inline void Draw(const ScreenGeometry& target, const ScreenGeometry& path, const ImVec2* impact) {
    const auto& theme = menu::panel::ThemeColors(menu::panel::ClientSettings::Get().themeValue());
    const ImU32 color = theme.iconHighlightColor;
    DrawGeometry(target, (color & 0x00ffffffu) | (24u << 24), color, 1.5f);
    DrawGeometry(path, 0, color, 1.8f);
    if (impact) {
        auto* draw = ImGui::GetBackgroundDrawList();
        draw->AddCircleFilled(*impact, 3.f, color);
        draw->AddCircle(*impact, 5.5f, color, 20, 1.4f);
    }
}
}
