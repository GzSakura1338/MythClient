// Native port of neverlose-last/examples/example_win32_directx11/neverlose_menu.cpp.
// Maintain this C++ implementation directly; Scale and property arguments are Nova adapters.
#include "ui/clickgui/panel/ReferenceStyle.h"
#include <cmath>
#include "imgui_internal.h"
#include "ui/clickgui/panel/Widgets.h"
#include "ui/backend/blur/Blur.h"

namespace nova::menu::panel::reference {
namespace {
const ColorTheme* palette = nullptr;
constexpr float kTextBody=15.f, kTextCaption=10.f, kTextControl=14.f;
ImVec2 V(float x,float y) { return {Scale(x),Scale(y)}; }
ImU32 C(int r,int g,int b,int a=255) {
    const ImU32 raw=IM_COL32(r,g,b,a);
    if(!palette || color::Red(palette->windowBgColor)<100)return raw;
    // Light theme is an adapter extension; the reference only supplies dark.
    if(r==17&&g==19&&b==27)return palette->modulePanelBgColor;
    if(r==31&&g==34&&b==44)return palette->windowBorderColor;
    if(r==29&&g==33&&b==43)return palette->switchDisabledBgColor;
    if(r==89&&g==94&&b==106)return palette->secondaryColor;
    if(r==25&&g==28&&b==38)return palette->selectorBgColor;
    if(r==31&&g==38&&b==54)return palette->buttonHoveredBgColor;
    if(r==32&&g==35&&b==46)return palette->splitColor;
    if(r==170&&g==173&&b==184)return palette->mainColor;
    if(r==34&&g==38&&b==48)return palette->sliderBgColor;
    if(r==20&&g==20&&b==29)return IM_COL32(248,249,252,245);
    if(r==57&&g==61&&b==76)return IM_COL32(180,187,202,205);
    return raw;
}
}
void SetTheme(const ColorTheme* theme) { palette=theme; }

void PageReveal(float progress) {
    if(progress>=.9999f)return;
    auto* parent=ImGui::GetCurrentWindow();
    const float offset=Scale(9)*(1-progress);
    for(auto* window:GImGui->Windows)if(window->Active&&(window==parent||ImGui::IsWindowChildOf(window,parent,false))) {
        for(auto& vertex:window->DrawList->VtxBuffer){vertex.pos.x+=offset;vertex.col=color::ReAlpha(vertex.col,int(color::Alpha(vertex.col)*progress));}
        for(auto& command:window->DrawList->CmdBuffer){command.ClipRect.x+=offset;command.ClipRect.z+=offset;}
    }
}

// Source: neverlose_menu.cpp :: Motion
float Motion(ImGuiID id, float target, float speed, float initial)
{
    float* value = ImGui::GetStateStorage()->GetFloatRef(id, initial < 0.0f ? target : initial);
    *value = ImLerp(*value, target, 1.0f - std::exp(-speed * ImGui::GetIO().DeltaTime));
    if (std::fabs(*value - target) < 0.0005f) *value = target;
    return *value;
}

// Source: neverlose_menu.cpp :: Mix
ImU32 Mix(ImU32 a, ImU32 b, float t)
{
    return ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(a), ImGui::ColorConvertU32ToFloat4(b), ImSaturate(t)));
}

// Source: neverlose_menu.cpp :: Text
void Text(ImDrawList* d, ImVec2 p, ImU32 color, const char* value, float size, ImFont* font)
{
    d->AddText(font ? font : ImGui::GetFont(), Scale(size), p, color, value);
}

// Source: neverlose_menu.cpp :: TextY
void TextY(ImDrawList* d, float x, float y, float h, ImU32 color, const char* value, float size, ImFont* font)
{
    ImFont* f = font ? font : ImGui::GetFont();
    const float th = f->CalcTextSizeA(Scale(size), FLT_MAX, 0.0f, value).y;
    Text(d, ImVec2(x, y + std::floor((Scale(h) - th) * 0.5f)), color, value, size, f);
}

// Source: neverlose_menu.cpp :: Hit
bool Hit(const char* id, ImVec2 p, ImVec2 size)
{
    ImGui::SetCursorScreenPos(p);
    return ImGui::InvisibleButton(id, size);
}

// Source: neverlose_menu.cpp :: Dots
void Dots(ImDrawList* d, ImVec2 p)
{
    for (int i = 0; i < 3; ++i) d->AddCircleFilled(p + V(i * 3.0f, 0), Scale(1.0f), C(187, 191, 202));
}

// Source: neverlose_menu.cpp :: Chevron
void Chevron(ImDrawList* d, ImVec2 p, ImU32 color)
{
    d->AddLine(p, p + V(3, 3), color, Scale(1.3f));
    d->AddLine(p + V(3, 3), p + V(0, 6), color, Scale(1.3f));
}

// Source: neverlose_menu.cpp :: Check
void Check(ImDrawList* d, ImVec2 p, ImU32 color)
{
    d->AddLine(p, p + V(3, 3), color, Scale(1.5f));
    d->AddLine(p + V(3, 3), p + V(8, -3), color, Scale(1.5f));
}

// Source: neverlose_menu.cpp :: Card
void Card(ImDrawList* d, ImVec2 p, ImVec2 size, const char* title)
{
    Text(d, p + V(12, -18), C(89, 94, 106), title, kTextCaption);
    d->AddRectFilled(p, p + size, C(17, 19, 27, 224), Scale(14.0f));
    d->AddRect(p, p + size, C(31, 34, 44), Scale(14.0f));
}

// Source: neverlose_menu.cpp :: Toggle
void Toggle(ImDrawList* d, int id, ImVec2 p, bool& value, bool enabled)
{
    ImGui::PushID(id);
    if (enabled && Hit("##toggle", p - V(5, 6), V(39, 30))) value = !value;
    const ImGuiID iid = ImGui::GetItemID();
    const float on = Motion(iid ^ 0x55aa721u, value ? 1.0f : 0.0f, 19.0f);
    const float hover = Motion(iid ^ 0x118a0u, enabled && ImGui::IsItemHovered() ? 1.0f : 0.0f, 20.0f);
    ImGui::PopID();
    d->AddRectFilled(p - V(1 + hover, 1 + hover), p + V(30 + hover, 19 + hover), C(75, 126, 255, (int)(45 * hover)), Scale(10));
    d->AddRectFilled(p, p + V(29, 18), Mix(C(29, 33, 43), C(75, 126, 255), on), Scale(9));
    d->AddCircleFilled(p + V(ImLerp(9.0f, 20.0f, on), 9), Scale(7), Mix(C(133, 144, 156), C(248, 249, 252), on));
}

// Source: RowControl Select/MultiSelect; caller owns the property and popup.
bool Select(ImDrawList* d,int id,ImVec2 cp,float cw,const char* value) {
        ImGui::PushID(id);
        const bool click = Hit("##select", cp, ImVec2(cw, Scale(23)));
        const ImGuiID iid = ImGui::GetItemID();
        const float response = Motion(iid ^ 0x6161u, ImGui::IsItemHovered() ? 1.0f : 0.0f);
        ImGui::PopID();
        d->AddRectFilled(cp, cp + ImVec2(cw, Scale(23)), Mix(C(25, 28, 38), C(31, 38, 54), response * kFieldHoverFill), Scale(5));
        d->AddRect(cp, cp + ImVec2(cw, Scale(23)), Mix(C(32, 35, 46), C(75, 126, 255), response * kFieldHoverBorder), Scale(5));
        d->PushClipRect(cp, cp + ImVec2(cw - Scale(20), Scale(23)), true);
        TextY(d, cp.x + Scale(7), cp.y, 23, C(170, 173, 184), value ? value : "Select", kTextControl);
        d->PopClipRect();
        d->AddLine(cp + ImVec2(cw - Scale(13), Scale(9)), cp + ImVec2(cw - Scale(9), Scale(13)), C(139, 143, 154), Scale(1.0f));
        d->AddLine(cp + ImVec2(cw - Scale(9), Scale(13)), cp + ImVec2(cw - Scale(5), Scale(9)), C(139, 143, 154), Scale(1.0f));
    return click;
}

// Source: RowControl Slider; normalized value is supplied by NumberProp.
float Slider(ImDrawList* d,int id,ImVec2 start,float amount,float initial,float width) {
    const float tw=Scale(width);
        ImGui::PushID(id);
        Hit("##slider", start - V(5, 8), ImVec2(tw + Scale(10), Scale(19)));
        if (ImGui::IsItemActive()) amount = ImClamp((ImGui::GetIO().MousePos.x - start.x) / tw, 0.0f, 1.0f);
        const float shown = Motion(ImGui::GetItemID() ^ 0xa8f1u, amount, 14.0f, initial);
        ImGui::PopID();
        d->AddRectFilled(start, start + ImVec2(tw, Scale(3)), C(34, 38, 48), Scale(2));
        d->AddRectFilled(start, start + ImVec2(tw * shown, Scale(3)), C(75, 126, 255), Scale(2));
        d->AddCircleFilled(start + ImVec2(tw * shown, Scale(1.5f)), Scale(5.5f), C(247, 248, 252));
    return amount;
}

// PopupLayer background, without the reference's offset shadow.
void PopupBackdrop(ImDrawList* d,ImVec2 p,ImVec2 size) {
    // Preserve the reference tint opacity even when sampling the blur backdrop.
    ui::blur::LayerBackground(d, p, p + size, C(20, 20, 29, 235), Scale(16), 0, 1.f, 1.f);
    d->AddRect(p, p + size, C(57, 61, 76, 205), Scale(16));
    d->AddLine(p + V(16, 1), p + V(size.x / Scale(1.f) - 16, 1), C(255, 255, 255, 22));
}

float PopupEase(float open) {
    const float eased = 1.0f - std::pow(1.0f - open, 3.0f);
    return eased;
}

float PageEase(float& page_mix) {
    page_mix=ImLerp(page_mix,1.0f,1.0f-std::exp(-15.0f*ImGui::GetIO().DeltaTime));const float pe=1.0f-std::pow(1.0f-page_mix,3.0f);
    return pe;
}
}
