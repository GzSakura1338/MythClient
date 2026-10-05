#include "ui/clickgui/panel/Theme.h"

namespace nova::menu::panel {
namespace {

constexpr ColorTheme kLight{
0xFF663329,
0xFFFFEAE5,
0xFF331A15,
0xFF665552,
0xFFFFFFFF,
0x33000000,
0x00000000,
0xFF4D4D4D,
0xFF808080,
0xFF1A1A1A,
0xFFFF7D66,
0x50999999,
0xFFEBEBEB,
0xFFF2F2F2,
0x20000000,
0xFFE5E5E5,
0xFFFF9966,
0xFFFFFFFF,
0xFFE5E5E5,
0xFFE5E5E5,
0xFFE5E5E5,
0xFFFF9966,
0xFFFF9966,
0xFFFF9966,
0xFFFFFFFF,
0xFFE5E5E5,
0xFFFF9966,
0xFFFF9966,
0xFFE5E5E5,
0x50CCCCCC,
0x50999999,
0x00000000,
0x47000000,
0xB3FF7D66,
0xFFFF7D66,
};

constexpr ColorTheme kDark{
0xFF663329,
0xFFFFEAE5,
0xFFE5D2CF,
0xFF807573,
0xFF0E0E0A,
0x33FFFFFF,
0x00000000,
0xFFCCCCCC,
0xFF808080,
0xFFFFFFFF,
0xFFFF7D66,
0x50000000,
0xFF0D0D0D,
0xFF101010, // Keep dark cards close to the outer glass instead of lifting their brightness.
0x20FFFFFF,
0xFF262626,
0xFFFF9966,
0xFFFFFFFF,
0xFF262626,
0xFF262626,
0xFF262626,
0xFFFF9966,
0xFFFF9966,
0xFFFF9966,
0xFFFFFFFF,
0xFF262626,
0xFFFF9966,
0xFFFF9966,
0xFF262626,
0x50CCCCCC,
0x50999999,
0x00000000,
0x47FFFFFF,
0xB3FF7D66,
0xFFFF7D66,
};

}

const ColorTheme& ThemeColors(Theme theme) {
    return theme == Theme::Dark ? kDark : kLight;
}

const ColorTheme& PanelThemeColors(Theme theme) {
    constexpr auto C = [](int r, int g, int b, int a = 255) -> Color {
        return Color(r) | (Color(g) << 8) | (Color(b) << 16) | (Color(a) << 24);
    };
    static constexpr ColorTheme dark{
        .clientIconBgColor=C(8,27,48), .clientIconColor=C(94,185,255),
        .clientNameColor=C(228,230,236), .clientVersionColor=C(91,96,108),
        .windowBgColor=C(13,15,22,222), .windowBorderColor=C(30,33,43),
        .categoryPanelBgColor=C(18,21,30,231),
        .mainColor=C(207,209,218), .secondaryColor=C(91,96,108),
        .textHighlightColor=C(224,229,243), .iconHighlightColor=C(75,126,255),
        .categoryHighlightColor=C(39,43,54), .mainPanelBgColor=0,
        .modulePanelBgColor=C(17,19,27,224), .splitColor=C(28,31,40),
        .switchDisabledBgColor=C(29,33,43), .switchEnabledBgColor=C(75,126,255),
        .switchIndicatorColor=C(248,249,252),
        .buttonBgColor=C(25,28,38), .buttonHoveredBgColor=C(31,38,54), .buttonActiveBgColor=C(39,43,54),
        .highlightButtonBgColor=C(75,126,255), .highlightButtonHoveredBgColor=C(82,141,255),
        .highlightButtonActiveBgColor=C(64,107,217), .highlightButtonTextColor=C(230,233,241),
        .sliderBgColor=C(34,38,48), .sliderHighlightBgColor=C(75,126,255), .sliderIndicatorColor=C(247,248,252),
        .selectorBgColor=C(25,28,38), .selectorElementHoverColor=C(75,126,255,25), .selectorElementActiveColor=C(75,126,255,40),
        .scrollbarTrackColor=0, .scrollbarThumbColor=C(62,70,81), .scrollbarThumbHoverColor=C(98,108,121), .scrollbarThumbActiveColor=C(75,126,255)
    };
    static constexpr ColorTheme light = [=] {
        ColorTheme t=dark;
        t.clientIconBgColor=C(222,234,251); t.clientNameColor=C(33,39,53);
        t.clientVersionColor=t.secondaryColor=C(107,115,132);
        t.windowBgColor=C(242,244,249,240); t.windowBorderColor=C(205,211,224);
        t.categoryPanelBgColor=C(232,236,245,240); t.mainColor=C(48,56,73);
        t.textHighlightColor=C(38,75,148); t.categoryHighlightColor=C(215,225,244);
        t.modulePanelBgColor=C(250,251,254,230); t.splitColor=C(222,227,237);
        t.switchDisabledBgColor=C(216,223,234);
        t.buttonBgColor=t.selectorBgColor=C(232,236,245);
        t.buttonHoveredBgColor=C(221,229,246); t.buttonActiveBgColor=C(209,222,246);
        t.sliderBgColor=C(211,219,234); t.scrollbarThumbColor=C(163,174,194);
        t.scrollbarThumbHoverColor=C(121,140,171);
        return t;
    }();
    return theme == Theme::Dark ? dark : light;
}

}
