#include "cs2/modules/combat/recoilcontrol/RecoilControl.h"
#include "cs2/hooks/InputHook.h"
#include "ui/notifications/Notifications.h"
#include <algorithm>
#include <cmath>

namespace nova::cs2::modules {
RecoilControl::RecoilControl() : Module("Recoil Control", "combat", "General"),
    horizontal_(addNumber("Horizontal", 0, 100, 1, 100, "{:.0f}%")),
    vertical_(addNumber("Vertical", 0, 100, 1, 100, "{:.0f}%")),
    start_(addNumber("Start Bullet", 1, 10, 1, 2, "{:.0f}")),
    smooth_(addNumber("Smooth", 0, 100, 1, 0, "{:.0f} ms")) {}
void RecoilControl::onEnable() {
    if (!hooks::input::Available()) {
        notifications::Post("Recoil Control: input adapter unavailable for this game version", notifications::Type::Error);
        setEnabled(false);
    }
}
void RecoilControl::onDisable() { hooks::input::Publish({}, false); }
void RecoilControl::onUpdate() {
    if (!requestedEnabled()) return;
    if (!hooks::input::Available()) {
        notifications::Post("Recoil Control stopped: input adapter unavailable", notifications::Type::Error);
        setEnabled(false); onDisable(); return;
    }
    auto value = [](double number, unsigned fallback) {
        return std::isfinite(number) ? static_cast<unsigned>(std::clamp(std::round(number), 0., 100.)) : fallback;
    };
    hooks::input::Publish({value(horizontal_.get(), 100), value(vertical_.get(), 100),
        value(start_.get(), 2), value(smooth_.get(), 0)}, true);
}
}
