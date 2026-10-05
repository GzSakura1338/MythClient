#pragma once

#include <vector>

#include "config/settings/Properties.h"
#include "bind/BindProperty.h"
#include "ui/clickgui/panel/Theme.h"

namespace nova::menu::panel {

class ClientSettings {
public:
    static ClientSettings& Get();

    ClientSettings(const ClientSettings&) = delete;
    ClientSettings& operator=(const ClientSettings&) = delete;

    [[nodiscard]] Properties::ModeProperty& theme() { return theme_; }
    [[nodiscard]] Theme themeValue() const { return static_cast<Theme>(theme_.get()); }
    [[nodiscard]] float guiScale() const { return static_cast<float>(guiSize_.get() / 100.0); }
    [[nodiscard]] bool blurEnabled() const { return blurSupported_ && blur_.get(); }
    [[nodiscard]] bool stageManagerEnabled() const { return stageManager_.get(); }
    void SetBlurSupported(bool supported) { blurSupported_ = supported; }
    [[nodiscard]] int blurStrength() const { return static_cast<int>(blurStrength_.get()); }
    [[nodiscard]] int blurFps() const { return static_cast<int>(blurFps_.get()); }
    [[nodiscard]] float hudBackgroundAlpha() const { return static_cast<float>(hudBackgroundAlpha_.get() / 100.0); }
    [[nodiscard]] std::string clientName() const;
    [[nodiscard]] const std::vector<Properties::PropDesc>& rows() const { return rows_; }

private:
    ClientSettings();
    bool blurSupported_ = true;

    Properties::ModeProperty theme_;
    nova::bind::BindProperty unloadKey_{0x2E, false, true}; // VK_DELETE
    Properties::NumberProp guiSize_;
    Properties::NumberProp hudBackgroundAlpha_{0, 100, 1, 100, "{:.0f}%"};
    Properties::BoolProperty blur_{false};
    Properties::BoolProperty stageManager_{true};
    Properties::BoolProperty customClientName_{false};
    Properties::StringProperty clientName_{};
    Properties::NumberProp blurStrength_{1, 6, 1, 3, "{:.0f}"};
    Properties::NumberProp blurFps_{5, 120, 1, 30, "{:.0f} FPS"};
    std::vector<Properties::PropDesc> rows_;
};

}
