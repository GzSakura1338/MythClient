#include "ui/backend/blur/Blur.h"
#include "ui/clickgui/panel/ClientSettings.h"

namespace nova::ui::blur {
Options GetOptions() {
    const auto& settings = menu::panel::ClientSettings::Get();
    return {settings.blurEnabled(),
            std::clamp(settings.blurStrength(), 1, 6), std::clamp(settings.blurFps(), 5, 120)};
}
}
