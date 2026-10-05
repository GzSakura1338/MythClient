#include "imgui.h"
#include "ui/clickgui/Fonts.h"
#include "ui/clickgui/fonts/font_medium.h"
#include "ui/clickgui/fonts/font_extrabold.h"
#include "ui/clickgui/fonts/font_iconfont.h"
#include "ui/clickgui/fonts/font_iconfont2.h"
#include "ui/clickgui/fonts/font_classic.h"
#include "ui/clickgui/fonts/font_PanelRegularData.h"
#include "ui/clickgui/fonts/font_PanelStrongData.h"
#include "ui/clickgui/fonts/font_PanelIconsData.h"

constexpr int kAtlasSize = 2048;

ImFont* Medium12 = nullptr;
ImFont* ExtraBold20 = nullptr;
ImFont* Classic9 = nullptr;
ImFont* PanelRegular = nullptr;
ImFont* PanelStrong = nullptr;
ImFont* PanelIcons = nullptr;
ImFont* IconFont = nullptr;
ImFont* IconFont2 = nullptr;

void InitializeFatalMenuFonts()
{
    if (Medium12) return;
    ImFontAtlas* fonts = ImGui::GetIO().Fonts;
    fonts->TexMinWidth = kAtlasSize;
    fonts->TexMinHeight = kAtlasSize;
    Medium12 = fonts->AddFontFromMemoryCompressedBase85TTF(FontMedium_compressed_data_base85, 14.0f);
    ExtraBold20 = fonts->AddFontFromMemoryCompressedBase85TTF(FontExtraBold_compressed_data_base85, 24.0f);
    ImFontConfig classic;
    classic.FontDataOwnedByAtlas = false;
    classic.OversampleH = classic.OversampleV = 1;
    classic.PixelSnapH = true;
    Classic9 = fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(ClassicFontData), sizeof(ClassicFontData), 9.0f, &classic);
    if (Classic9) {
        ImFontConfig fallback;
        fallback.MergeMode = true;
        fonts->AddFontFromMemoryCompressedBase85TTF(FontMedium_compressed_data_base85, 9.0f, &fallback);
    }
    static constexpr ImWchar iconRange[] = { 0xE000, 0xF8FF, 0 };
    IconFont = fonts->AddFontFromMemoryCompressedBase85TTF(
        iconfont_compressed_data_base85, 16.0f, nullptr, iconRange);
    IconFont2 = fonts->AddFontFromMemoryCompressedBase85TTF(
        iconfont2_compressed_data_base85, 16.0f, nullptr, iconRange);
    if (!Medium12) Medium12 = ImGui::GetFont();
    if (!ExtraBold20) ExtraBold20 = ImGui::GetFont();
    if (!Classic9) Classic9 = Medium12;
    if (!IconFont) IconFont = ImGui::GetFont();
    if (!IconFont2) IconFont2 = ImGui::GetFont();
    ImFontConfig panel;
    panel.FontDataOwnedByAtlas = false;
    panel.OversampleH = 3;
    panel.PixelSnapH = true;
    PanelRegular = fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(PanelRegularData), sizeof(PanelRegularData), 15.f, &panel);
    ImFontConfig panelFallback;
    panelFallback.MergeMode = true;
    fonts->AddFontFromMemoryCompressedBase85TTF(FontMedium_compressed_data_base85, 15.f, &panelFallback);
    PanelStrong = fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(PanelStrongData), sizeof(PanelStrongData), 16.f, &panel);
    fonts->AddFontFromMemoryCompressedBase85TTF(FontMedium_compressed_data_base85, 16.f, &panelFallback);
    PanelIcons = fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(PanelIconsData), sizeof(PanelIconsData), 13.f, &panel);
}

void ResetFatalMenuFonts()
{
    Medium12 = nullptr;
    ExtraBold20 = nullptr;
    Classic9 = nullptr;
    PanelRegular = PanelStrong = PanelIcons = nullptr;
    IconFont = nullptr;
    IconFont2 = nullptr;
}
