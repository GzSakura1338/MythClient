#include "cs2/modules/render/fovcontrol/FovControl.h"
#include "cs2/hooks/FovHook.h"
#include "ui/notifications/Notifications.h"

namespace nova::cs2::modules {
FovControl::FovControl() : Module("FOV Control", "render", "General"),
    degrees_(addNumber("FOV", 30, 150, .1, 90, "{:.1f}")),
    preserveZoom_(addBool("Preserve Zoom", true)),
    removeShake_(addBool("Remove View Shake", true)) {}
void FovControl::onEnable() {
    if (!hooks::fov::Available()) {
        notifications::Post("FOV Control: camera adapter unavailable for this game version", notifications::Type::Error);
        setEnabled(false);
    }
}
void FovControl::onDisable() { hooks::fov::Publish({}, false); }
void FovControl::onUpdate() {
    if (!requestedEnabled()) return;
    if (!hooks::fov::Available()) {
        notifications::Post("FOV Control stopped: camera adapter unavailable", notifications::Type::Error);
        setEnabled(false); onDisable(); return;
    }
    hooks::fov::Publish({static_cast<float>(degrees_.get()), preserveZoom_.get(), removeShake_.get()}, true);
}
}
