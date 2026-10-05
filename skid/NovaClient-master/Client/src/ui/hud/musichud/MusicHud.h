#pragma once

#include <memory>
#include "ui/hud/musichud/SourceProperty.h"

namespace nova::ui::music {

struct MusicHudSettings {
    SourceProperty source;
    Properties::NumberProp size{60, 180, 5, 100, "{:.0f}%"};
    Properties::BoolProperty controls{true};
    Properties::BoolProperty hidePaused{false};
    Properties::BoolProperty lyrics{true};
    Properties::BoolProperty lyricsShadow{true};
    Properties::NumberProp lyricsSize{60, 180, 5, 100, "{:.0f}%"};

    std::vector<Properties::PropDesc> Rows() {
        return {{"Source", &source}, {"Size", &size}, {"Controls", &controls},
            {"Hide When Paused", &hidePaused}, {"Lyrics", &lyrics},
            {"Lyrics Shadow", &lyricsShadow}, {"Lyrics Size", &lyricsSize}};
    }
};

// The host supplies enable/edit state and registers Rows() with its catalog.
// All media presentation and animation state belongs to this common feature.
class MusicHud {
public:
    MusicHud();
    ~MusicHud();
    MusicHud(const MusicHud&) = delete;
    MusicHud& operator=(const MusicHud&) = delete;
    MusicHudSettings& Settings() { return settings_; }
    void SetEnabled(bool enabled);
    void Draw(bool editing);
    // Native preview uses the existing placeholder/lyrics view without WinRT.
    void DrawPreview();
private:
    struct Impl;
    MusicHudSettings settings_;
    std::unique_ptr<Impl> impl_;
};

}
