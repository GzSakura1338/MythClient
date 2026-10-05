#include "cs2/modules/render/radaresp/RadarESP.h"
#include "cs2/hooks/RadarHook.h"
#include "ui/notifications/Notifications.h"

namespace nova::cs2::modules {
RadarESP::RadarESP() : Module("Radar ESP", "render", "General") {}
void RadarESP::onEnable() {
    if (!hooks::radar::Available()) {
        notifications::Post("Radar ESP: unsupported game version or radar hook unavailable", notifications::Type::Error);
        setEnabled(false);
        return;
    }
    hooks::radar::SetEnabled(true);
}
void RadarESP::onDisable() { hooks::radar::SetEnabled(false); }
}
