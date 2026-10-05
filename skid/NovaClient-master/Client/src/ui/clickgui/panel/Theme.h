#pragma once

#include <cstdint>

#include "utils/Color.h"

namespace nova::menu::panel {

using Color = nova::color::Value;

struct ColorTheme {
    Color clientIconBgColor;
    Color clientIconColor;
    Color clientNameColor;
    Color clientVersionColor;
    Color windowBgColor;
    Color windowBorderColor;
    Color categoryPanelBgColor;
    Color mainColor;
    Color secondaryColor;
    Color textHighlightColor;
    Color iconHighlightColor;
    Color categoryHighlightColor;
    Color mainPanelBgColor;
    Color modulePanelBgColor;
    Color splitColor;
    Color switchDisabledBgColor;
    Color switchEnabledBgColor;
    Color switchIndicatorColor;
    Color buttonBgColor;
    Color buttonHoveredBgColor;
    Color buttonActiveBgColor;
    Color highlightButtonBgColor;
    Color highlightButtonHoveredBgColor;
    Color highlightButtonActiveBgColor;
    Color highlightButtonTextColor;
    Color sliderBgColor;
    Color sliderHighlightBgColor;
    Color sliderIndicatorColor;
    Color selectorBgColor;
    Color selectorElementHoverColor;
    Color selectorElementActiveColor;
    Color scrollbarTrackColor;
    Color scrollbarThumbColor;
    Color scrollbarThumbHoverColor;
    Color scrollbarThumbActiveColor;
};

enum class Theme : int {
    Light = 0,
    Dark,
};

[[nodiscard]] const ColorTheme& ThemeColors(Theme theme);
// Menu-only palette; the shared HUD palette above remains independent.
[[nodiscard]] const ColorTheme& PanelThemeColors(Theme theme);

}
