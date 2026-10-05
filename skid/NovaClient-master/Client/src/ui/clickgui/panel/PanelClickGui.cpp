#include "ui/clickgui/panel/PanelClickGui.h"
#include "ui/backend/blur/Blur.h"

#include <algorithm>
#include <cmath>

#include "imgui.h"
#include "imgui_internal.h"
#include "ui/clickgui/Fonts.h"
#include "ui/clickgui/panel/PageLayout.h"
#include "ui/clickgui/panel/ClientSettings.h"
#include "ui/clickgui/panel/Widgets.h"
#include "ui/clickgui/panel/ReferenceStyle.h"
#include "ui/overlay/WindowPlacement.h"

namespace nova::menu::panel {
namespace {

constexpr float kWindowWidth = 748.0f;
constexpr float kWindowHeight = 576.0f;
constexpr float kWindowRounding = 14.0f;
constexpr float kWindowPadding = 0.0f;

}

PanelClickGui::PanelClickGui()
    : topBar_(*this), layout_(std::make_unique<PageLayout>(*this)) {}

PanelClickGui::~PanelClickGui() = default;

const ColorTheme& PanelClickGui::colors() const {
    return PanelThemeColors(ClientSettings::Get().themeValue());
}

Properties::ModeProperty& PanelClickGui::themeSetting() {
    return ClientSettings::Get().theme();
}

void PanelClickGui::pageChanged() { pageMix_ = 0.f; }
float PanelClickGui::pageReveal() { return reference::PageEase(pageMix_); }
void PanelClickGui::resumeFromWorkspace() {
    lastFrame_=ImGui::GetFrameCount();
    revealStart_=ImGui::GetTime()-.5;
    popupAnimations_.reset();
}

void PanelClickGui::render() {
    const ColorTheme& theme = colors();
    reference::SetTheme(&theme);
    if(lastFrame_ != ImGui::GetFrameCount()-1) {revealStart_=ImGui::GetTime();popupAnimations_.reset();}
    lastFrame_=ImGui::GetFrameCount();
    const float reveal=std::clamp(float((ImGui::GetTime()-revealStart_)/.48),0.f,1.f);

    if (appliedScale_ == 0.0f || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
        appliedScale_ = ClientSettings::Get().guiScale();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float fit = std::min(std::max(1.0f, display.x - 16.0f) / kWindowWidth,
                               std::max(1.0f, display.y - 16.0f) / kWindowHeight);
    const float scale = std::min(appliedScale_, fit);
    if(renderedScale_!=scale){popupAnimations_.reset();renderedScale_=scale;}
    SetPopupAnimations(&popupAnimations_);
    SetPanelScale(scale);
    // Scope both native ImGui widgets and font rasterization to the menu.
    // Custom geometry uses Scale(), keeping visuals and hitboxes identical.
    const ImGuiStyle previousStyle = ImGui::GetStyle();
    ImGui::GetStyle().ScaleAllSizes(scale);
    ImGui::GetStyle().FontScaleMain *= scale;
    ImGui::PushFont(PanelRegular, 15.0f);

    ImGui::SetNextWindowSize(ImVec2(Scale(kWindowWidth), Scale(kWindowHeight)));
    ui::PlaceWindowCentered("PanelClickGui", Scale(kWindowWidth), Scale(kWindowHeight));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, Scale(kWindowRounding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Scale(kWindowPadding), Scale(kWindowPadding)));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(Scale(kWindowPadding), Scale(kWindowPadding)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, Scale(1.0f));
    // Keep a 12 px native drag target while drawing a slim 6 px thumb.
    // Scoped here so every nested settings popup inherits the panel theme.
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Scale(12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarPadding, Scale(3.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, Scale(6.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.windowBgColor);
    ImGui::PushStyleColor(ImGuiCol_Border, theme.windowBorderColor);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, theme.scrollbarTrackColor);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, theme.scrollbarThumbColor);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, theme.scrollbarThumbHoverColor);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, theme.scrollbarThumbActiveColor);

    const bool visible = ImGui::Begin("PanelClickGui", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoBackground);
    ui::CaptureWindow("PanelClickGui");
    if (visible) {
        // Generate the complete window for shelf snapshots, even when dragged
        // partly off-screen. Children still intersect with this window and
        // their own scroll bounds; the GPU clips the final frame to the display.
        const auto panelMin=ImGui::GetWindowPos();
        const auto panelMax=panelMin+ImGui::GetWindowSize();
        ImGui::PushClipRect(panelMin,panelMax,false);
        ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha,1.f);
        ImGui::BeginDisabled(reveal<1.f);
        {
            auto* draws = ImGui::GetWindowDrawList();
            const auto min = ImGui::GetWindowPos();
            const auto size = ImGui::GetWindowSize();
            const ImVec2 max(min.x + size.x, min.y + size.y);
            draws->PushClipRect(min-ImVec2(1,1),max+ImVec2(1,1),false);
            ui::blur::LayerBackground(draws, min, max, theme.windowBgColor, Scale(kWindowRounding));
            draws->AddRect(min, max, theme.windowBorderColor, Scale(kWindowRounding), 0, Scale(1.0f));
            draws->PopClipRect();
        }
        const auto origin = ImGui::GetWindowPos();
        auto* d = ImGui::GetWindowDrawList();
        d->AddRectFilled(origin, origin+ImVec2(Scale(158),Scale(576)),theme.categoryPanelBgColor,Scale(14),ImDrawFlags_RoundCornersLeft);
        d->AddLine(origin+ImVec2(Scale(158),0),origin+ImVec2(Scale(158),Scale(576)),theme.windowBorderColor,Scale(1));
        topBar_.render();
        layout_->navigation(origin);
        layout_->content(origin);
        layout_->account(origin);
        ImGui::EndDisabled();
        ImGui::PopStyleVar();
        ImGui::PopClipRect();
        // Transform every child together. Clip rectangles follow the same transform.
        auto* root=ImGui::GetCurrentWindow();
        if(reveal<1.f) {
            const float e=1.f-std::pow(1.f-reveal,4.f), z=ImLerp(.92f,1.f,e);
            const ImVec2 pivot=root->Pos+ImVec2(Scale(750),Scale(756))*.5f, shift(0,Scale(16)*(1-e));
            auto transform=[&](ImVec2 p) { return pivot+(p-pivot)*z+shift; };
            for(auto* w:GImGui->Windows) if(w->Active && (w==root || w->RootWindow==root || w->RootWindowPopupTree==root)) {
                for(auto& v:w->DrawList->VtxBuffer) {v.pos=transform(v.pos);v.col=color::ReAlpha(v.col,int(color::Alpha(v.col)*e));}
                for(auto& c:w->DrawList->CmdBuffer) {
                    if(!c.UserCallback && !c.TexRef._TexData && ui::blur::IsBackdropTexture(c.TexRef._TexID))
                        for(unsigned i=0;i<c.ElemCount;++i) {
                            auto& v=w->DrawList->VtxBuffer[c.VtxOffset+w->DrawList->IdxBuffer[c.IdxOffset+i]];
                            v.uv=ui::blur::BackdropUv(v.pos);
                        }
                    auto a=transform({c.ClipRect.x,c.ClipRect.y}), b=transform({c.ClipRect.z,c.ClipRect.w});
                    c.ClipRect={a.x,a.y,b.x,b.y};
                }
            }
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(7);
    ImGui::GetStyle() = previousStyle;
    ImGui::PopFont();
    popupAnimations_.drawClosing();
    SetPopupAnimations(nullptr);
    reference::SetTheme(nullptr);
    SetPanelScale(1.0f);
}

}
