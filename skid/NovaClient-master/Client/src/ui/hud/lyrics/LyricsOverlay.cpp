#include "ui/hud/lyrics/LyricsOverlay.h"
#include "ui/clickgui/Fonts.h"
#include "ui/text/ImTextRenderer.h"
#include "utils/Color.h"

namespace nova::ui::music {
namespace { float Ease(float value) { return 1 - std::pow(1 - std::clamp(value, 0.f, 1.f), 3.f); } }
void LyricsOverlay::Draw(const std::shared_ptr<const smtc::MediaSnapshot>& snapshot, bool enabled, bool editing, float size, float delta, bool shadow) {
    shadow_ = shadow;
    scale_ = size * ImGui::GetStyle().FontScaleMain * ImGui::GetStyle().FontScaleDpi;
    scale_ = std::min(scale_, std::max(.1f, (ImGui::GetIO().DisplaySize.x - 12) / 320.f));
    std::shared_ptr<const smtc::Lyrics> lyrics;
    int index = -1;
    if (snapshot && snapshot->state == smtc::State::Ready && snapshot->lyrics && snapshot->HasDuration()) {
        position_ = snapshot->Position(); duration_ = snapshot->end;
        lyrics = snapshot->lyrics; index = lyrics->Index(position_);
    }
    if (editing && (!lyrics || lyrics->lines.empty())) {
        if (!previewLyrics_) {
            auto preview = std::make_shared<smtc::Lyrics>();
            preview->lines = {{0,"Previous lyrics",{}}, {1,"A line before",{}},
                {2,"Lyrics","Lyrics will appear here during playback"}, {3,"A line ahead",{}}, {4,"Upcoming lyrics",{}}};
            previewLyrics_ = std::move(preview);
        }
        lyrics = previewLyrics_; index = 2;
        position_ = 2.5; duration_ = 5;
    }
    const bool changedTrack = snapshot && (session_ != snapshot->session || track_ != snapshot->track);
    if (changedTrack) { session_ = snapshot->session; track_ = snapshot->track; lyrics_.reset(); }
    if (lyrics && !lyrics->lines.empty()) {
        const float target = static_cast<float>(std::max(0, index));
        if (lyrics != lyrics_ || std::abs(target - center_) > 3) center_ = target;
        if (index != active_ || lyrics != lyrics_) switched_ = ImGui::GetTime();
        lyrics_ = std::move(lyrics); active_ = index;
        center_ += (target - center_) * (1 - std::exp(-12.f * delta));
    }
    const bool show = enabled && (editing || (snapshot && snapshot->lyrics && !snapshot->lyrics->lines.empty() && snapshot->HasDuration()));
    reveal_ = std::clamp(reveal_ + (show ? delta : -delta) / .3f, 0.f, 1.f);
    Render(editing && enabled);
}
void LyricsOverlay::Line(ImDrawList* draws, const std::string& value, float left, float top, float width, ImU32 color, bool scroll, float fill) {
    if (value.empty()) return;
    const float length = text::TextWidth(value.c_str()), overflow = std::max(0.f, length - width);
    float shift = 0;
    if (scroll && overflow > 0) {
        const double elapsed = std::max(0.0, ImGui::GetTime() - switched_ - 1);
        const double travel = overflow / (25 * scale_), phase = std::fmod(elapsed, travel * 2 + 2);
        shift = static_cast<float>(phase < travel ? phase * 25 * scale_ : phase < travel + 1 ? overflow :
            phase < travel * 2 + 1 ? overflow - (phase - travel - 1) * 25 * scale_ : 0);
    }
    draws->PushClipRect(ImVec2(left, top), ImVec2(left + width, top + 18 * scale_), true);
    const float x = left + std::max(0.f, (width - length) * .5f) - shift;
    const int alpha = (color >> 24) & 255;
    text::DrawText(draws, value.c_str(), x, top, fill < 0 ? color : color::ReAlpha(color, int(alpha * .38f)), shadow_);
    if (fill > 0) {
        draws->PushClipRect(ImVec2(x, top), ImVec2(x + length * std::clamp(fill, 0.f, 1.f), top + 18 * scale_), true);
        text::DrawText(draws, value.c_str(), x, top, color, false);
        draws->PopClipRect();
    }
    draws->PopClipRect();
}
void LyricsOverlay::OnDraw(const Placement& placement, ImDrawList* draws) {
    if (!lyrics_) return;
    const float width = ContentWidth(), height = 200 * scale_;
    const auto display = ImGui::GetIO().DisplaySize;
    const float left = std::clamp(placement.pos.x - (placement.offset.x < 0 ? width : 0), 0.f, std::max(0.f, display.x - width));
    const float top = std::clamp(placement.pos.y - (placement.offset.y < 0 ? height : 0), 0.f, std::max(0.f, display.y - height));
    const float widthNow = width * Ease(reveal_);
    if (widthNow < .5f) return;
    const ImVec2 min(left + (width - widthNow) * .5f, top), max(min.x + widthNow, top + height);
    draws->PushClipRect(min, max, true);
    const float fontScale = scale_ / (ImGui::GetStyle().FontScaleMain * ImGui::GetStyle().FontScaleDpi);
    const int first = std::max(0, static_cast<int>(std::floor(center_)) - 3);
    const int last = std::min(static_cast<int>(lyrics_->lines.size()) - 1, static_cast<int>(std::ceil(center_)) + 3);
    for (int i = first; i <= last; ++i) {
        const float distance = i - center_, absolute = std::abs(distance);
        const float edge = std::clamp((2.5f - absolute) * 2, 0.f, 1.f);
        if (edge <= 0) continue;
        const float emphasis = active_ >= 0 ? std::max(0.f, 1 - absolute) : 0;
        const float alpha = edge * (.28f + .72f * emphasis + .16f * std::max(0.f, 1 - absolute * .5f));
        const auto& line = lyrics_->lines[i];
        const float y = top + (82 + distance * 40 + (line.translation.empty() ? 8 : 0)) * scale_;
        ImGui::PushFont(Medium12, (12 + 3 * emphasis) * fontScale);
        float fill = -1;
        if (i == active_) {
            if (line.words.empty()) {
                const double end = i + 1 < static_cast<int>(lyrics_->lines.size()) ? lyrics_->lines[i + 1].time : duration_;
                fill = static_cast<float>(line.Progress(position_, end));
            } else {
                const auto sung = line.Sung(position_);
                const float length = text::TextWidth(line.text.c_str());
                const float before = text::TextWidth(line.text.substr(0, sung.offset).c_str());
                const float after = text::TextWidth(line.text.substr(0, sung.offset + sung.length).c_str());
                fill = length > 0 ? static_cast<float>((before + (after - before) * sung.fraction) / length) : 0;
            }
        }
        Line(draws, line.text, left + 12 * scale_, y, width - 24 * scale_,
            color::ReAlpha(0xFFFFEAE5, int(255 * std::min(1.f, alpha))), i == active_, fill);
        ImGui::PopFont();
        ImGui::PushFont(Medium12, (10 + emphasis) * fontScale);
        Line(draws, line.translation, left + 12 * scale_, y + 19 * scale_, width - 24 * scale_,
            color::ReAlpha(0xFFFFEAE5, int(180 * std::min(1.f, alpha))), i == active_);
        ImGui::PopFont();
    }
    draws->PopClipRect();
}
}
