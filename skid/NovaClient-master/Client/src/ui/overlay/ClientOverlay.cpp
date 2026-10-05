#include "ui/overlay/HudBackground.h"
#include "ui/overlay/ClientOverlay.h"

#include <algorithm>

#include "ui/clickgui/Fonts.h"
#include "ui/text/ImTextRenderer.h"
#include "ui/overlay/WindowPlacement.h"

namespace nova::overlay {
namespace {

constexpr ImGuiWindowFlags kDefaultWindowFlags =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground |
    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

constexpr float kTitleHeight = 30.0f;
constexpr float kContentOffset = 9.0f;
constexpr float kTitleMargin = 12.0f;
constexpr float kWindowWidth = 200.0f;
constexpr float kWindowRounding = 12.0f;

constexpr float kTitleAnimationSeconds = 0.35f;

constexpr ImU32 kTitleColor = 0xFFFFEAE5;

float Scale() {
    const ImGuiStyle& style = ImGui::GetStyle();
    return style.FontScaleMain * style.FontScaleDpi;
}

}

void ClientOverlay::Render(bool input) {
    const bool visible = IsVisible();
    if (!visible && titleProgress_ <= 0.0f)
        return;
    const Placement placement = PreRender(input);
    ImDrawList* draws = ImGui::GetBackgroundDrawList();
    if (visible) OnDraw(placement, draws);
}

ClientOverlay::Placement ClientOverlay::PreRender(bool input) {
    const bool showTitle = IsTitleAlwaysVisible() || input;
    const double now = ImGui::GetTime();
    const float delta = lastTitleFrame_ < 0.0 ? 0.0f :
        static_cast<float>(std::clamp(now - lastTitleFrame_, 0.0, 0.1));
    lastTitleFrame_ = now;
    titleProgress_ = std::clamp(titleProgress_ + (showTitle ? delta : -delta) /
        kTitleAnimationSeconds, 0.0f, 1.0f);
    const float moveProgress = std::min(1.0f, titleProgress_ * 2.0f);
    const float revealProgress = std::max(0.0f, titleProgress_ * 2.0f - 1.0f);
    const float displacement = moveProgress * moveProgress * (3.0f - 2.0f * moveProgress);
    const float expanded = revealProgress * revealProgress * (3.0f - 2.0f * revealProgress);
    ImGuiWindowFlags flags = kDefaultWindowFlags;
    if (!input)
        flags |= ImGuiWindowFlags_NoInputs;

    const float scale = Scale();
    const float titleHeight = kTitleHeight * scale;
    const float contentOffset = kContentOffset * scale;

    const float yOffset = (titleHeight + contentOffset) * displacement;

    ImGui::PushFont(Medium12, kOverlayTitleSize);

    const float measured = ContentWidth();
    const float titleFloor = nova::text::TextWidth(OverlayName()) + kTitleMargin * 2.0f * scale;
    const float windowWidth =
        measured > 0.0f ? std::max(measured, titleFloor) : kWindowWidth * scale;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, kWindowRounding * scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1.0f, 1.0f));

    ImGui::SetNextWindowSize(ImVec2(windowWidth, titleHeight));
    const ImVec2 fallback = DefaultPosition();
    ui::PlaceWindow(OverlayId(), fallback.x, fallback.y);
    ImGui::Begin(OverlayId(), nullptr, flags);
    ui::CaptureWindow(OverlayId());
    const ImVec2 windowPos = ImGui::GetWindowPos();

    const ImVec2 windowSize = ImGui::GetWindowSize();
    ImGui::Dummy(windowSize);

    Placement placement;
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    placement.offset.x = windowPos.x + windowSize.x / 2.0f > displaySize.x / 2.0f ? -1.0f : 1.0f;
    placement.offset.y = windowPos.y + windowSize.y / 2.0f > displaySize.y / 2.0f ? -1.0f : 1.0f;

    if (expanded > 0.0f) {
        const float revealedWidth = windowSize.x * expanded;
        const float revealLeft = windowPos.x +
            (placement.offset.x < 0.0f ? windowSize.x - revealedWidth : 0.0f);
        const ImVec2 titleMin(revealLeft, windowPos.y);
        const ImVec2 titleMax(revealLeft + revealedWidth, windowPos.y + titleHeight);
        ImDrawList* titleDraws = ImGui::GetWindowDrawList();
        titleDraws->PushClipRect(titleMin, titleMax, true);
        ui::hud::Background(titleDraws, titleMin, titleMax, 0xFF33221F,
            std::min(kWindowRounding * scale, revealedWidth * 0.5f));
        const ImVec2 namePos = nova::text::CenterText(OverlayName(),
            windowPos.x + kTitleMargin * scale, windowPos.y + titleHeight / 2.0f, false, true);
        nova::text::DrawText(titleDraws, OverlayName(), namePos.x, namePos.y, kTitleColor, false);
        titleDraws->PopClipRect();
    }

    placement.pos = ImVec2(
        windowPos.x + (placement.offset.x < 0.0f ? windowSize.x : 0.0f),
        windowPos.y + (placement.offset.y < 0.0f
                           ? -contentOffset + windowSize.y * (1.0f - displacement)
                           : yOffset));

    placement.min = windowPos;
    placement.max = ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y);
    placement.downY = windowPos.y + yOffset;
    placement.upY = windowPos.y - contentOffset + windowSize.y * (1.0f - displacement);

    ImGui::End();

    ImGui::PopStyleVar(4);
    ImGui::PopFont();
    return placement;
}

}
