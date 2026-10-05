#pragma once
#include "ui/overlay/ClientOverlay.h"
#include "smtc/MediaSnapshot.h"

namespace nova::ui::music {
// A second draggable overlay owned by MusicHUD, not a separately registered module.
class LyricsOverlay final : private overlay::ClientOverlay {
public:
    void Draw(const std::shared_ptr<const smtc::MediaSnapshot>& snapshot, bool enabled, bool editing, float size, float delta, bool shadow);
private:
    const char* OverlayId() const override { return "overlays.musiclyrics"; }
    const char* OverlayName() const override { return "Lyrics"; }
    ImVec2 DefaultPosition() const override { return ImVec2(.30f, .86f); }
    bool IsVisible() const override { return reveal_ > 0; }
    float ContentWidth() const override { return 320 * scale_; }
    void OnDraw(const Placement& placement, ImDrawList* draws) override;
    void Line(ImDrawList* draws, const std::string& value, float left, float top, float width, ImU32 color, bool scroll, float fill = -1);
    std::shared_ptr<const smtc::Lyrics> lyrics_, previewLyrics_;
    std::uint64_t session_{}, track_{};
    int active_ = -1;
    bool shadow_ = true;
    float scale_ = 1, reveal_{}, center_{};
    double switched_{};
    double position_{}, duration_{};
};
}
