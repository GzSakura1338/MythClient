#include "ui/overlay/HudBackground.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <format>
#include <memory>
#include <string>

#include "ui/hud/autoblockhud/AutoBlockHud.h"
#include "ui/clickgui/Fonts.h"
#include "ui/overlay/ClientOverlay.h"
#include "ui/text/ImTextRenderer.h"
#include "utils/Color.h"

namespace nova::ui::autoblockhud {
namespace {

float EaseOut(float value) {
    const float inverse = 1.0f - value;
    return 1.0f - inverse * inverse * inverse;
}

ImU32 WithAlpha(ImU32 color, float alpha) {
    return color::ReAlpha(color, static_cast<int>(std::lround(
        color::Alpha(color) * std::clamp(alpha, 0.0f, 1.0f))));
}

}

struct AutoBlockHud::Impl final : private overlay::ClientOverlay {
public:
    explicit Impl(Settings& s) : duration_(&s.duration),expandFrom_(&s.expandFrom),width_(&s.width),
        background_(&s.background),textColor_(&s.text),barColor_(&s.bar),shadow_(&s.shadow) {}

    void Draw(bool enabled,bool editing,const Snapshot& stats) {
        if (!ImGui::GetCurrentContext() || !ImGui::GetFont()) return;
        portrait_ = stats.portrait;
        const double now = ImGui::GetTime();
        delta_ = static_cast<float>(lastFrame_ > 0 ? std::clamp(now - lastFrame_, 0.0, 0.1) : 0);
        lastFrame_ = now;
        if (!enabled && progress_ == 0) {
            preview_ = false;
            Render(false);
            return;
        }
        preview_ = enabled && editing;
        if (enabled && stats.active) {
            lastActive_ = now;
            targetPercent_ = stats.percent;
        }
        const bool show = enabled && (preview_ || stats.active || now - lastActive_ < 0.15);
        const float duration = static_cast<float>(duration_->get() / 1000.0);
        progress_ = std::clamp(progress_ + (show ? delta_ : -delta_) / duration, 0.0f, 1.0f);
        expansion_ = EaseOut(progress_);
        if (progress_ == 0 && !show) {
            targetPercent_ = barFill_ = 0;
            fromNumber_ = toNumber_ = 0;
            roll_ = 1;
            Render(false);
            return;
        }
        barFill_ += (targetPercent_ / 100.0f - barFill_) * (1.0f - std::exp(-7.0f * delta_));
        roll_ = std::min(1.0f, roll_ + delta_ * 4.5f);
        const int next = std::clamp(static_cast<int>(std::lround(targetPercent_)), 0, 100);
        if (roll_ >= 1 && next != toNumber_) {
            fromNumber_ = toNumber_;
            toNumber_ = next;
            roll_ = 0;
        }
        Render(preview_);
    }

private:
    const char* OverlayId() const override { return "overlays.autoblockhud"; }
    const char* OverlayName() const override { return "AutoBlockHUD"; }
    bool IsVisible() const override { return progress_ > 0 || preview_; }
    ImVec2 DefaultPosition() const override { return ImVec2(0.60f, 0.62f); }

    float DigitWidth() const {
        float width = 0;
        for (char digit = '0'; digit <= '9'; ++digit) {
            const char text[]{digit, 0};
            width = std::max(width, text::TextWidth(text));
        }
        return width;
    }

    float MeasuredWidth() const {
        const float font = ImGui::GetFontSize();
        const float number = DigitWidth() * 3 + text::TextWidth("%");
        const float minimum = std::round(font * 0.26f) * 4 + std::round(font * 1.58f) +
            text::TextWidth("AutoBlock") + number;
        return std::max(minimum, static_cast<float>(width_->get()) * font / kOverlayTextSize);
    }

    float ContentWidth() const override {
        ImGui::PushFont(Medium12, kOverlayTextSize);
        const float width = MeasuredWidth();
        ImGui::PopFont();
        return width;
    }

    void DrawNumber(ImDrawList* draws, float x, float y, float line, ImU32 color) const {
        const auto before = std::format("{:3d}", fromNumber_);
        const auto after = std::format("{:3d}", toNumber_);
        const float cell = DigitWidth();
        const float eased = EaseOut(roll_);
        const float direction = toNumber_ >= fromNumber_ ? 1.0f : -1.0f;
        draws->PushClipRect(ImVec2(x - 1, y - 1),
            ImVec2(x + cell * 3 + 1, y + line + 2), true);
        for (int i = 0; i < 3; ++i) {
            const char previous[]{before[i], 0}, current[]{after[i], 0};
            const float left = x + cell * i;
            const float newX = left + (cell - text::TextWidth(current)) * 0.5f;
            if (before[i] == after[i] || roll_ >= 1) {
                text::DrawText(draws, current, newX, y, color, shadow_->get());
            } else {
                text::DrawText(draws, previous,
                    left + (cell - text::TextWidth(previous)) * 0.5f,
                    y - direction * (line + 2) * eased, WithAlpha(color, 1 - eased), shadow_->get());
                text::DrawText(draws, current, newX,
                    y + direction * (line + 2) * (1 - eased), WithAlpha(color, eased), shadow_->get());
            }
        }
        draws->PopClipRect();
        text::DrawText(draws, "%", x + cell * 3, y, color, shadow_->get());
    }

    void OnDraw(const Placement& placement, ImDrawList* draws) override {
        ImGui::PushFont(Medium12, kOverlayTextSize);
        const float font = ImGui::GetFontSize();
        const float pad = std::round(font * 0.26f);
        const float head = std::round(font * 1.58f);
        const float line = text::TextHeight();
        const float gap = std::round(font * 0.21f);
        const float barHeight = std::max(3.0f, std::round(font * 0.32f));
        const float width = MeasuredWidth();
        const float contentHeight = line + gap + barHeight;
        const float height = 2 * pad + std::max(head, contentHeight);
        const auto display = ImGui::GetIO().DisplaySize;
        const float left = std::clamp(placement.pos.x - (placement.offset.x < 0 ? width : 0),
            0.0f, std::max(0.0f, display.x - width));
        const float top = std::clamp(placement.pos.y - (placement.offset.y < 0 ? height : 0),
            0.0f, std::max(0.0f, display.y - height));
        const bool fromRight = expandFrom_->get() == 2 ||
            (expandFrom_->get() == 0 && placement.offset.x < 0);
        const float revealedWidth = width * expansion_;
        if (revealedWidth < 0.5f) {
            ImGui::PopFont();
            return;
        }
        const float revealLeft = fromRight ? left + width - revealedWidth : left;
        const ImVec2 revealMin(revealLeft, top);
        const ImVec2 revealMax(revealLeft + revealedWidth, top + height);
        const float rounding = std::round(font * 0.42f);
        ui::hud::Background(draws, revealMin, revealMax, background_->getColorAbgr(),
            std::min(rounding, revealedWidth * 0.5f));
        draws->PushClipRect(revealMin, revealMax, true);
        const ImVec2 headMin(left + pad, top + (height - head) * 0.5f);
        const ImVec2 headMax(headMin.x + head, headMin.y + head);
        if (portrait_.texture != 0) {
            ui::DrawPortrait(draws, portrait_, headMin, headMax,
                IM_COL32_WHITE, head * 0.18f);
        } else {
            draws->AddRectFilled(headMin, headMax, IM_COL32(0, 0, 0, 90), head * 0.18f);
        }
        const float x = left + pad * 2 + head;
        const float y = top + (height - contentHeight) * 0.5f;
        const float available = width - head - pad * 3;
        const ImU32 color = textColor_->getColorAbgr();
        text::DrawText(draws, "AutoBlock", x, y, color, shadow_->get());
        DrawNumber(draws, left + width - pad - DigitWidth() * 3 - text::TextWidth("%"), y, line, color);
        const float barY = y + line + gap;
        draws->AddRectFilled(ImVec2(x, barY), ImVec2(x + available, barY + barHeight),
            IM_COL32(0, 0, 0, 110), barHeight * 0.5f);
        if (available * barFill_ > 0.5f)
            draws->AddRectFilled(ImVec2(x, barY), ImVec2(x + available * barFill_, barY + barHeight),
                barColor_->getColorAbgr(), barHeight * 0.5f);
        draws->PopClipRect();
        ImGui::PopFont();
    }

    Properties::NumberProp* duration_ = nullptr;
    Properties::ModeProperty* expandFrom_ = nullptr;
    Properties::NumberProp* width_ = nullptr;
    Properties::ColorProp* background_ = nullptr;
    Properties::ColorProp* textColor_ = nullptr;
    Properties::ColorProp* barColor_ = nullptr;
    Properties::BoolProperty* shadow_ = nullptr;
    Portrait portrait_;
    double lastFrame_ = 0;
    double lastActive_ = -1;
    float delta_ = 0;
    float progress_ = 0;
    float expansion_ = 0;
    float targetPercent_ = 0;
    float barFill_ = 0;
    float roll_ = 1;
    int fromNumber_ = 0;
    int toNumber_ = 0;
    bool preview_ = false;
};

AutoBlockHud::AutoBlockHud() : impl_(std::make_unique<Impl>(settings_)) {}
AutoBlockHud::~AutoBlockHud() = default;
void AutoBlockHud::Draw(bool enabled,bool editing,const Snapshot& snapshot) { impl_->Draw(enabled,editing,snapshot); }

}
