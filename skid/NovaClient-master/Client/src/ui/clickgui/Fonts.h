#pragma once

#include "imgui.h"

extern ImFont* Medium12;
extern ImFont* ExtraBold20;
extern ImFont* Classic9;
extern ImFont* PanelRegular;
extern ImFont* PanelStrong;
extern ImFont* PanelIcons;

extern ImFont* IconFont;
extern ImFont* IconFont2;

inline constexpr float kOverlayTextSize = 19.0f;
inline constexpr float kOverlayTitleSize = 22.0f;

void InitializeFatalMenuFonts();
void ResetFatalMenuFonts();
