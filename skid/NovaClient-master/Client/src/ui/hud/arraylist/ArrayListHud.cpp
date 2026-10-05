#include "ui/overlay/HudBackground.h"
#include <algorithm>
#include <cmath>
#include <format>
#include <vector>

#include "ui/hud/arraylist/ArrayListHud.h"
#include "ui/clickgui/Fonts.h"
#include "ui/overlay/ClientOverlay.h"
#include "ui/text/ImTextRenderer.h"
#include "utils/Color.h"

namespace nova::ui::arraylist {
namespace {

// The gap between a module's name and its suffix, in the row's own units.
constexpr float kSuffixGap = 6.0f;
}

std::vector<Properties::PropDesc> Settings::Rows() {
    return {
        {"Size",&size,{{"Horizontal padding",&horizontalPadding},{"Vertical padding",&verticalPadding}}},
        {"Animations",&animations,{{"Animation speed",&animationSpeed},{"Slide from",&slideFrom}}},
        {"Text Shadow",&textShadow}, {"Text Color",nullptr,textColor.descriptors()},
        {"Background",&background,backgroundColor.descriptors()},
        {"Sidebar",&sidebar,{{"Width",&sidebarWidth},{"Follow text color",&sidebarFollowTextColor},
            {"Color",nullptr,sidebarColor.descriptors(),[this]{return !sidebarFollowTextColor.get();}},
            {"Gradient",&sidebarGradient},{"Shadow",&sidebarShadow}}}
    };
}

std::vector<Properties::PropDesc> Settings::ConfigRows() {
    std::vector<Properties::PropDesc> rows{
        {"Animations",&animations,{{"Animation speed",&animationSpeed},{"Slide from",&slideFrom}}},
        {"Size",&size},{"HorizontalPadding",&horizontalPadding},{"VerticalPadding",&verticalPadding}};
    const auto appendColor=[&](Properties::ColorProp& color){for(auto& row:color.descriptors())rows.push_back(std::move(row));};
    appendColor(textColor);
    rows.emplace_back("TextShadow",&textShadow);
    rows.emplace_back("Background",&background);
    appendColor(backgroundColor);
    rows.emplace_back("Sidebar",&sidebar);
    rows.emplace_back("SidebarFollowTextColor",&sidebarFollowTextColor);
    appendColor(sidebarColor);
    rows.emplace_back("SidebarWidth",&sidebarWidth);
    rows.emplace_back("SidebarGradient",&sidebarGradient);
    rows.emplace_back("SidebarShadow",&sidebarShadow);
    return rows;
}

struct ArrayListHud::Impl final : private overlay::ClientOverlay {
public:
    explicit Impl(Settings& s) : animations_(&s.animations), animationSpeed_(&s.animationSpeed),
        slideFrom_(&s.slideFrom), size_(&s.size), horizontalPadding_(&s.horizontalPadding),
        verticalPadding_(&s.verticalPadding), textColor_(&s.textColor), textShadow_(&s.textShadow),
        background_(&s.background), backgroundColor_(&s.backgroundColor), sidebar_(&s.sidebar),
        sidebarFollowTextColor_(&s.sidebarFollowTextColor), sidebarColor_(&s.sidebarColor),
        sidebarWidth_(&s.sidebarWidth), sidebarGradient_(&s.sidebarGradient), sidebarShadow_(&s.sidebarShadow) {}

private:
    struct Row {
        std::string id, name;
        std::string suffix;
        float progress = 0.0f;
        float width = 0.0f;
        bool shown = false;
    };

    // Starain 1.20.1 SmoothAnimationTimer: percentage-space approach with a
    // minimum speed, so the final fraction reaches the endpoint exactly.
    static float Advance(float current, bool shown, float delta, float speed) {
        const float target = shown ? 100.0f : 0.0f;
        const float distance = std::abs(target - current);
        const float step = std::min(distance, delta * std::max(10.0f, distance * 40.0f) * speed);
        return shown ? current + step : current - step;
    }

public:
    void Draw(bool enabled, bool editing, std::span<const Entry> entries) {
        if (ImGui::GetCurrentContext() == nullptr || ImGui::GetFont() == nullptr)
            return;
        const double now = ImGui::GetTime();
        delta_ = lastFrame_ < 0.0 ? 0.0f : static_cast<float>(std::clamp(now - lastFrame_, 0.0, 0.1));
        lastFrame_ = now;
        enabled_ = enabled;
        entries_ = entries;
        Render(enabled && editing);
        entries_ = {};
    }

private:
    void OnDraw(const Placement& placement, ImDrawList* draws) override {
        const float uiScale = static_cast<float>(size_->get() / 100.0);
        ImGui::PushFont(Medium12, kOverlayTextSize * uiScale);

        const float horizontalPadding = static_cast<float>(horizontalPadding_->get()) * uiScale;
        const float verticalPadding = static_cast<float>(verticalPadding_->get()) * uiScale;
        const float sidebarWidth = SidebarWidth(uiScale);
        const float height = nova::text::TextHeight() + verticalPadding * 2.0f;
        const bool animated = animations_->get();

        for (const auto& entry : entries_) {
            if (!enabled_ || !entry.enabled || entry.hidden)
                continue;
            if (std::ranges::none_of(rows_, [&](const Row& row) { return row.id == entry.id; })) {
                Row row;
                row.id = entry.id;
                row.name = entry.name;
                row.suffix = entry.suffix;
                rows_.push_back(std::move(row));
            }
        }

        for (Row& row : rows_) {
            const auto entry = std::ranges::find_if(entries_, [&](const Entry& e) { return e.id == row.id; });
            row.shown = enabled_ && entry != entries_.end() && entry->enabled && !entry->hidden;
            row.progress = animated ? Advance(row.progress, row.shown, delta_, static_cast<float>(animationSpeed_->get()))
                                    : (row.shown ? 100.0f : 0.0f);
            if (entry != entries_.end()) { row.suffix = entry->suffix; row.name = entry->name; }
            const float nameWidth = nova::text::TextWidth(row.name.c_str());
            const float padding = horizontalPadding * 2.0f + sidebarWidth;
            row.width = nameWidth + SuffixWidth(row.suffix, uiScale) + padding;
        }
        std::ranges::stable_sort(rows_, [](const Row& a, const Row& b) { return a.width > b.width; });

        float yOffset = 0.0f;
        int index = 0;
        const bool fromRight = slideFrom_->get() == 2 || (slideFrom_->get() == 0 && placement.offset.x < 0.0f);
        for (const Row& row : rows_) {
            if (row.progress <= 0.0f) continue;
            const float progress = row.progress / 100.0f;
            // Right anchored: left = anchor - width * (2 * progress - 1).
            // Mirror the same full-row motion when the HUD is on the left.
            const float slide = (fromRight ? 1.0f : -1.0f) * 2.0f * row.width * (1.0f - progress);
            RenderRow(ImVec2(placement.pos.x + slide, placement.pos.y + yOffset * placement.offset.y),
                draws, row, sidebarWidth, height, horizontalPadding, verticalPadding, uiScale,
                placement.offset, index++);
            yOffset += height * progress;
        }
        std::erase_if(rows_, [](const Row& row) { return !row.shown && row.progress <= 0.0f; });

        ImGui::PopFont();
    }

    // Zero, and deliberately: ClientOverlay then sizes the drag window from its own
    // default instead of from the list.
    //
    // The window is placed by its top-left, and the rows hang off whichever of its
    // edges the overlay is anchored to -- the left edge on the left of the screen,
    // the right edge on the right. A window as wide as the widest row therefore
    // moves the edge the rows hang from every time a module is toggled, and on the
    // right of the screen that edge is the one the list should be pinned by: it slid
    // further right as the list widened, so the list grew outwards from a fixed left
    // edge instead of inwards from a fixed right one.
    //
    // A width that does not follow the content pins the edge, and a row wider than
    // the window simply reaches past it. The window is only a handle to drag.
    [[nodiscard]] float ContentWidth() const override { return 0.0f; }

    [[nodiscard]] float SidebarWidth(float uiScale) const {
        return sidebar_->get() ? static_cast<float>(sidebarWidth_->get()) * uiScale : 0.0f;
    }

    // The module's own word for what it is doing, drawn after the name with a gap.
    [[nodiscard]] static float SuffixWidth(const std::string& suffix, float uiScale) {
        if (suffix.empty())
            return 0.0f;
        return kSuffixGap * uiScale + nova::text::TextWidth(suffix.c_str());
    }

    void RenderRow(ImVec2 pos, ImDrawList* draws, const Row& row,
                   float sidebarWidth, float height, float horizontalPadding, float verticalPadding,
                   float uiScale, ImVec2 offset,
                   int index) const {
        const float width = row.width;
        const float left = pos.x - (offset.x < 0.0f ? width : 0.0f);
        const float top = pos.y - (offset.y < 0.0f ? height : 0.0f);
        const float bottom = top + height;
        const char* text = row.name.c_str();
        const ImU32 backgroundColor = backgroundColor_->getColorAbgr(index);
        const ImU32 textColor = textColor_->getColorAbgr(index);
        const ImU32 sidebarColor = sidebarFollowTextColor_->get()
                                       ? textColor
                                       : sidebarColor_->getColorAbgr(index);

        if (background_->get()) {
            ui::hud::Background(draws,
                ImVec2(left, top), ImVec2(left + width, bottom), backgroundColor);
        }

        ImVec2 sidebarEnd = pos;
        if (sidebarWidth > 0.0f) {
            sidebarEnd = ImVec2(pos.x + sidebarWidth * offset.x, pos.y);
            draws->AddRectFilled(ImVec2(std::min(pos.x, sidebarEnd.x), top),
                                 ImVec2(std::max(pos.x, sidebarEnd.x), bottom),
                                 sidebarColor);
            if (sidebarShadow_->get()) {
                draws->AddRectFilled(
                    ImVec2(std::min(sidebarEnd.x, sidebarEnd.x + 2.0f * uiScale * offset.x), top),
                    ImVec2(std::max(sidebarEnd.x, sidebarEnd.x + 2.0f * uiScale * offset.x), bottom),
                    nova::text::Dimmed(sidebarColor));
            }
            if (sidebarGradient_->get()) {
                const ImU32 gradientStart =
                    nova::color::ReAlpha(sidebarColor, static_cast<int>(0.2f * 255.0f));
                const ImU32 gradientEnd = nova::color::ReAlpha(sidebarColor, 0);
                draws->AddRectFilledMultiColor(
                    ImVec2(std::min(sidebarEnd.x, sidebarEnd.x + 8.0f * uiScale * offset.x), top),
                    ImVec2(std::max(sidebarEnd.x, sidebarEnd.x + 8.0f * uiScale * offset.x), bottom),
                    offset.x > 0 ? gradientStart : gradientEnd,
                    offset.x > 0 ? gradientEnd : gradientStart,
                    offset.x > 0 ? gradientEnd : gradientStart,
                    offset.x > 0 ? gradientStart : gradientEnd);
            }
        }

        float textX = sidebarEnd.x + offset.x * horizontalPadding;
        float textY = sidebarEnd.y + offset.y * verticalPadding;

        const float textWidth = width - horizontalPadding * 2.0f - sidebarWidth;
        if (offset.x < 0.0f)
            textX -= textWidth;
        if (offset.y < 0.0f)
            textY -= nova::text::TextHeight();
        nova::text::DrawText(draws, text, textX, textY, textColor, textShadow_->get());
        const float suffixX = textX + nova::text::TextWidth(text) + kSuffixGap * uiScale;
        if (!row.suffix.empty()) {
            // Secondary text needs a subtle contrast step, not the 25% shadow color.
            const ImU32 suffixColor = nova::color::Interpolate(
                textColor, nova::color::ReAlpha(0, nova::color::Alpha(textColor)), 0.2f);
            nova::text::DrawText(draws, row.suffix.c_str(), suffixX, textY, suffixColor, textShadow_->get());
        }
    }

    [[nodiscard]] const char* OverlayId() const override { return "overlays.toggledmodules"; }
    [[nodiscard]] const char* OverlayName() const override { return "ArrayList"; }
    [[nodiscard]] bool IsVisible() const override { return enabled_ || !rows_.empty(); }

    std::vector<Row> rows_;
    std::span<const Entry> entries_;
    bool enabled_ = false;
    double lastFrame_ = -1.0;
    float delta_ = 0.0f;
    Properties::BoolProperty* animations_ = nullptr;
    Properties::NumberProp* animationSpeed_ = nullptr;
    Properties::ModeProperty* slideFrom_ = nullptr;
    Properties::NumberProp* size_ = nullptr;

    Properties::NumberProp* horizontalPadding_ = nullptr;
    Properties::NumberProp* verticalPadding_ = nullptr;

    Properties::ColorProp* textColor_ = nullptr;
    Properties::BoolProperty* textShadow_ = nullptr;

    Properties::BoolProperty* background_ = nullptr;
    Properties::ColorProp* backgroundColor_ = nullptr;

    Properties::BoolProperty* sidebar_ = nullptr;
    Properties::BoolProperty* sidebarFollowTextColor_ = nullptr;
    Properties::ColorProp* sidebarColor_ = nullptr;
    Properties::NumberProp* sidebarWidth_ = nullptr;
    Properties::BoolProperty* sidebarGradient_ = nullptr;
    Properties::BoolProperty* sidebarShadow_ = nullptr;
};

ArrayListHud::ArrayListHud() : impl_(std::make_unique<Impl>(settings_)) {}
ArrayListHud::~ArrayListHud() = default;
void ArrayListHud::Draw(bool enabled, bool editing, std::span<const Entry> entries) { impl_->Draw(enabled,editing,entries); }

}
