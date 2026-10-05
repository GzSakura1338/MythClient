#pragma once

#include "imgui.h"
#include "ui/clickgui/panel/PopupAnimation.h"
#include "ui/clickgui/panel/Theme.h"

namespace nova::menu::panel {

// Fixed for the entire panel render, including all of its popups.
void SetPanelScale(float scale);
float Scale(float pixels);

inline constexpr float kTextTitle = 16.0f;
inline constexpr float kTextBody = 15.0f;
inline constexpr float kTextControl = 14.0f;
inline constexpr float kTextSmall = 12.0f;
inline constexpr float kTextCaption = 10.0f;
inline constexpr float kIconLarge = 16.0f;
inline constexpr float kIconSmall = 14.0f;

void PushText(float size);
void PushIcon(float size);
void PopFont();
void PushPanelIcon(float size = 13.f);
float Motion(ImGuiID id, float target, float speed = 16.f, float initial = -1.f);
void Check(ImDrawList*, ImVec2, ImU32);
void Chevron(ImDrawList*, ImVec2, ImU32, bool down = false);
bool Choice(const ColorTheme&, const char* id, const char* text, bool selected, float width);
// Scoped popup style/animation. Pair true with EndPanelPopup().
bool BeginPanelPopup(const ColorTheme&, const char* id, ImGuiWindowFlags flags = 0, PopupMotion motion = PopupMotion::Dropdown, ImVec2 padding = ImVec2(4,4));
void EndPanelPopup();

float TextWidth(const char* text);
inline float TextHeight() { return ImGui::GetTextLineHeight(); }

ImVec2 CenterText(const char* text, float x, float y, bool horizontal, bool vertical);

inline const char* Icon(const char8_t* glyph) {
    return reinterpret_cast<const char*>(glyph);
}

void SplitLine(const ColorTheme& theme);

void DrawScrollBar(const ColorTheme& theme, const char* id, const ImVec2& contentSize);

}
