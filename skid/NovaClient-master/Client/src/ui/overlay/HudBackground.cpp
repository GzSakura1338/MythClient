#include "ui/overlay/HudBackground.h"
#include "ui/backend/blur/Blur.h"
#include "ui/clickgui/panel/ClientSettings.h"

namespace nova::ui::hud {
ImU32 BackgroundColor(ImU32 color) {
    const float opacity = std::clamp(menu::panel::ClientSettings::Get().hudBackgroundAlpha(), 0.0f, 1.0f);
    const auto alpha = static_cast<ImU32>(((color & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT) * opacity + 0.5f);
    return (color & ~IM_COL32_A_MASK) | (alpha << IM_COL32_A_SHIFT);
}

void Background(ImDrawList* draws, ImVec2 min, ImVec2 max, ImU32 color,
                float rounding, ImDrawFlags flags, float visibility) {
    // Master alpha affects the colored tint only. Visibility is the animation
    // of the whole glass layer; coupling the two leaks the sharp scene through.
    blur::Background(draws, min, max, BackgroundColor(color), rounding, flags,
                     std::clamp(visibility, 0.0f, 1.0f));
}
}
