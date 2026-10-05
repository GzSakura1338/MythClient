#include "ui/overlay/HudBackground.h"
#include <algorithm>
#include <cmath>
#include <deque>
#include <format>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ui/notifications/NotificationHud.h"
#include "ui/notifications/Notifications.h"
#include "ui/clickgui/Fonts.h"
#include "imgui.h"

namespace nova::ui {
namespace {

constexpr float kCardHeight = 34.0f;
constexpr float kIconSize = 24.0f;
constexpr float kPadding = 5.0f;
constexpr float kTextInset = kPadding + kIconSize + 6.0f;
constexpr float kRightPadding = 10.0f;
// Match the default TargetHUD / AutoBlockHUD palette.
constexpr ImU32 kBackground = 0xCC331914;
constexpr ImU32 kTextColor = 0xFFFFEAE5;

float EaseOut(float value) {
    const float inverse = 1.0f - value;
    return 1.0f - inverse * inverse * inverse;
}

struct Toast {
    notifications::Message message;
    float progress = 0;
    float held = 0;
    float y = 0;
    bool placed = false;
    bool closing = false;
};

}

struct NotificationHud::Impl {
public:
    explicit Impl(NotificationSettings& settings) : duration_(&settings.duration), animation_(&settings.animation),
        offset_(&settings.offset), gap_(&settings.gap), limit_(&settings.limit),
        modules_(&settings.modules), configs_(&settings.configs) {}

private:
    static std::string FitText(std::string text, float width) {
        if (ImGui::CalcTextSize(text.c_str()).x <= width) return text;
        do {
            std::size_t end = text.size() - 1;
            while (end > 0 && (static_cast<unsigned char>(text[end]) & 0xC0) == 0x80) --end;
            text.resize(end);
        } while (!text.empty() && ImGui::CalcTextSize((text + "...").c_str()).x > width);
        return text + "...";
    }

    static void DrawIcon(ImDrawList* draws, ImVec2 center, float scale, notifications::Type type) {
        const float r = 5.5f * scale;
        const float stroke = 1.8f * scale;
        constexpr ImU32 color = IM_COL32_WHITE;
        if (type == notifications::Type::Success) {
            draws->AddLine(center + ImVec2(-r, 0), center + ImVec2(-r * 0.2f, r * 0.7f), color, stroke);
            draws->AddLine(center + ImVec2(-r * 0.2f, r * 0.7f), center + ImVec2(r, -r * 0.7f), color, stroke);
        } else if (type == notifications::Type::Error) {
            draws->AddLine(center - ImVec2(r * 0.65f, r * 0.65f), center + ImVec2(r * 0.65f, r * 0.65f), color, stroke);
            draws->AddLine(center + ImVec2(-r * 0.65f, r * 0.65f), center + ImVec2(r * 0.65f, -r * 0.65f), color, stroke);
        } else {
            const float direction = type == notifications::Type::Warning ? -1.0f : 1.0f;
            draws->AddCircleFilled(center + ImVec2(0, -r * 0.7f * direction), 1.3f * scale, color);
            draws->AddLine(center + ImVec2(0, -r * 0.15f * direction), center + ImVec2(0, r * 0.7f * direction), color, stroke);
        }
    }

    void DrawToast(Toast& toast, ImDrawList* draws, float center, float scale, float maxWidth) {
        const float height = kCardHeight * scale;
        const float inset = (kTextInset + kRightPadding) * scale;
        const std::string text = FitText(toast.message.text, std::max(1.0f, maxWidth - inset));
        const ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
        const float width = std::min(maxWidth, std::max(160.0f * scale, textSize.x + inset));
        const float revealed = width * EaseOut(toast.progress);
        if (revealed < 0.5f) return;
        const ImVec2 min(center - revealed * 0.5f, toast.y);
        const ImVec2 max(center + revealed * 0.5f, toast.y + height);
        // Match AutoBlockHUD's geometric reveal, with both edges anchored to the center.
        // Content stays at its final position; no alpha fade or horizontal stretching.
        const float rounding = height * 0.5f;
        ui::hud::Background(draws, min, max, kBackground, std::min(rounding, revealed * 0.5f));
        draws->PushClipRect(min, max, true);
        const float left = center - width * 0.5f;
        const ImVec2 icon(left + (kPadding + kIconSize * 0.5f) * scale, toast.y + height * 0.5f);
        draws->AddCircleFilled(icon, kIconSize * 0.5f * scale, ui::hud::BackgroundColor(IM_COL32(0, 0, 0, 90)));
        DrawIcon(draws, icon, scale, toast.message.type);
        draws->AddText(ImVec2(left + kTextInset * scale, toast.y + (height - textSize.y) * 0.5f),
            kTextColor, text.c_str());
        draws->PopClipRect();
    }

public:
    void Draw(bool enabled) {
        if (!ImGui::GetCurrentContext() || !ImGui::GetFont()) return;
        const float delta = std::clamp(ImGui::GetIO().DeltaTime, 0.0f, 0.1f);
        for (auto& message : notifications::Drain()) {
            if (!enabled || (message.source == notifications::Source::Config && !configs_->get()) ||
                (message.source == notifications::Source::Module && !modules_->get())) continue;
            if (waiting_.size() >= 32) waiting_.pop_front();
            waiting_.push_back(std::move(message));
        }
        if (!enabled) {
            waiting_.clear();
            for (auto& toast : active_) toast.closing = true;
        }

        ImGui::PushFont(Medium12, kOverlayTextSize);
        const float scale = ImGui::GetFontSize() / kOverlayTextSize;
        const auto display = ImGui::GetIO().DisplaySize;
        const float height = kCardHeight * scale;
        const float stride = height + static_cast<float>(gap_->get()) * scale;
        const float top = display.y * 0.5f + static_cast<float>(offset_->get()) * scale;
        const int fitting = std::max(0, static_cast<int>((display.y - top - 8.0f + stride - height) / stride));
        const int limit = std::min(static_cast<int>(limit_->get()), fitting);
        // Bounds changes animate surplus cards out rather than removing them abruptly.
        for (std::size_t i = static_cast<std::size_t>(limit); i < active_.size(); ++i)
            active_[i].closing = true;
        while (enabled && active_.size() < static_cast<std::size_t>(limit) && !waiting_.empty()) {
            active_.push_back(Toast{std::move(waiting_.front())});
            waiting_.pop_front();
        }
        const float animation = static_cast<float>(animation_->get()) / 1000.0f;
        float y = top;
        ImDrawList* draws = ImGui::GetBackgroundDrawList();
        for (auto& toast : active_) {
            if (!toast.closing && toast.progress >= 1.0f) {
                toast.held += delta;
                if (toast.held >= duration_->get()) toast.closing = true;
            }
            toast.progress = std::clamp(toast.progress + (toast.closing ? -delta : delta) / animation, 0.0f, 1.0f);
            if (!toast.placed) { toast.y = y; toast.placed = true; }
            toast.y += (y - toast.y) * (1.0f - std::exp(-16.0f * delta));
            DrawToast(toast, draws, display.x * 0.5f, scale, std::min(458.0f * scale, std::max(1.0f, display.x - 16.0f)));
            // Reserve full height on entry, collapse it smoothly on exit.
            y += stride * (toast.closing ? EaseOut(toast.progress) : 1.0f);
        }
        std::erase_if(active_, [](const Toast& toast) { return toast.closing && toast.progress <= 0.0f; });
        ImGui::PopFont();
    }

    Properties::NumberProp *duration_, *animation_, *offset_, *gap_, *limit_;
    Properties::BoolProperty *modules_, *configs_;
    std::deque<notifications::Message> waiting_;
    std::vector<Toast> active_;
};

NotificationHud::NotificationHud() : impl_(std::make_unique<Impl>(settings_)) {}
NotificationHud::~NotificationHud() = default;
void NotificationHud::Draw(bool enabled) { impl_->Draw(enabled); }

}
