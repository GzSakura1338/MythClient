#pragma once
#include "imgui.h"
#include "ui/clickgui/panel/Theme.h"
namespace nova::menu::panel::reference {
// Softer feedback shared by selectors and editable numeric fields.
inline constexpr float kFieldHoverFill = .45f;
inline constexpr float kFieldHoverBorder = .22f;
inline constexpr int kFieldSelectionAlpha = 48;
void SetTheme(const ColorTheme*);
float Motion(ImGuiID,float,float speed=16.f,float initial=-1.f);
ImU32 Mix(ImU32,ImU32,float);
void Text(ImDrawList*,ImVec2,ImU32,const char*,float size=15.f,ImFont* font=nullptr);
void TextY(ImDrawList*,float x,float y,float height,ImU32,const char*,float size=15.f,ImFont* font=nullptr);
bool Hit(const char*,ImVec2,ImVec2);
void Dots(ImDrawList*,ImVec2);
void Chevron(ImDrawList*,ImVec2,ImU32);
void Check(ImDrawList*,ImVec2,ImU32);
void Card(ImDrawList*,ImVec2,ImVec2,const char*);
void Toggle(ImDrawList*,int id,ImVec2,bool& value,bool enabled=true);
bool Select(ImDrawList*,int id,ImVec2,float width,const char* value);
float Slider(ImDrawList*,int id,ImVec2,float amount,float initial,float width=80.f);
void PopupBackdrop(ImDrawList*,ImVec2,ImVec2);
float PopupEase(float);
float PageEase(float&);
// Apply the shared page entrance to the current content window and its children.
void PageReveal(float progress);
}
