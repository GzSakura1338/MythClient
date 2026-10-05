#include "cs2/modules/combat/aimbot/Aimbot.h"
#include "cs2/modules/ModuleManager.h"
#include "cs2/hooks/InputHook.h"
#include "cs2/sdk/camera/Camera.h"
#include "ui/hud/aim/FovCircle.h"
#include "ui/notifications/Notifications.h"
#include <algorithm>
#include <cmath>

namespace nova::cs2::modules {
Aimbot::Aimbot() : Module("Aimbot", "combat", "General"),
    fov_(addNumber("Aim FOV", .1, 180, .1, 5, "{:.1f} deg")),
    horizontal_(addNumber("Horizontal Speed", 0, 720, 1, 120, "{:.0f} deg/s")),
    vertical_(addNumber("Vertical Speed", 0, 720, 1, 90, "{:.0f} deg/s")),
    smooth_(addNumber("Smooth", 0, 500, 5, 0, "{:.0f} ms")),
    target_(addMode("Target", 0, {"Single", "Switch"})),
    point_(addMode("Aim Priority", 1, {"Head", "Chest", "Pelvis"})),
    showFov_(addBool("Show FOV", true)),
    onlyClick_(addBool("Only While Clicking", true)),
    compensate_(addBool("Recoil Compensation", true)) {
    keybind().setHold(true);
}
void Aimbot::onEnable() {
    if (!hooks::input::AimAvailable()) {
        notifications::Post("Aimbot: aim adapter unavailable for this game version", notifications::Type::Error);
        setEnabled(false);
    }
}
void Aimbot::onDisable() { hooks::input::PublishAim({}, false, false, true, 0); }
void Aimbot::onUpdate() {
    if (!requestedEnabled()) return;
    if (!hooks::input::AimAvailable()) {
        notifications::Post("Aimbot stopped: aim adapter unavailable", notifications::Type::Error);
        setEnabled(false); onDisable(); return;
    }
    const auto number = [](double n, unsigned fallback) {
        return std::isfinite(n) ? static_cast<unsigned>(std::clamp(std::round(n), 0., 720.)) : fallback;
    };
    const auto* team = ModuleManager::Get().Find("Team");
    aim::Settings settings{static_cast<float>(fov_.get()), number(horizontal_.get(), 120), number(vertical_.get(), 90),
        target_.get() == 1, static_cast<sdk::aim::Point>(point_.get())};
    settings.compensateRecoil = compensate_.get();
    settings.smoothMs = std::min(number(smooth_.get(), 0), 500u);
    hooks::input::PublishAim(settings, true,
        team && team->requestedEnabled(), keybind().hold(), keybind().keybind(), onlyClick_.get());
}
void Aimbot::onRender() {
    if (!requestedEnabled() || !showFov_.get() || !ImGui::GetCurrentContext()) return;
    float projection = 0;
    if (!sdk::camera::CaptureVerticalProjection(projection)) return;
    const float fov = aim::Normalize({static_cast<float>(fov_.get())}).fov;
    ui::aim::Draw(ui::aim::Radius(fov, projection, ImGui::GetIO().DisplaySize.y));
}
}
