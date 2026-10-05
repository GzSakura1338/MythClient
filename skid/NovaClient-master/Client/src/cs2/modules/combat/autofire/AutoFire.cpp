#include "cs2/modules/combat/autofire/AutoFire.h"
#include "cs2/hooks/InputHook.h"
#include "cs2/modules/ModuleManager.h"
#include "ui/notifications/Notifications.h"
#include "core/RuntimeControl.h"
#include <windows.h>
#include <algorithm>
#include <cmath>

namespace nova::cs2::modules {
AutoFire::AutoFire() : Module("Auto Fire", "combat", "General"),
    delay_(addNumber("Delay", 0, 500, 1, 0, "{:.0f} ms")), scoped_(addBool("Scoped Only", false)) {}
void AutoFire::onEnable() {
    nextLog_ = 0;
    if (!hooks::input::FireAvailable()) {
        reportUnavailable();
        setEnabled(false); onDisable();
        return;
    }
    unavailableReported_ = false;
}
void AutoFire::reportUnavailable() {
    // A held bind or repeated enable attempt must not flood the HUD with the same fault.
    if (unavailableReported_) return;
    unavailableReported_ = true;
    const auto d = hooks::input::ReadFireDebug();
    const auto* reason = hooks::input::FireUnavailableReason();
    LogTo("CS2: Auto Fire unavailable: %s; status=%s frames=%u samples=%u targets=%u writes=%u deferred=%u lastDeferred=%s exception=0x%08X address=%p",
        reason, d.status, d.frames, d.samples, d.targets, d.writes, d.deferred, d.lastDeferred, d.exceptionCode,
        reinterpret_cast<void*>(d.exceptionAddress));
    notifications::Post(std::string("Auto Fire: ") + reason + "; see nova-cs2.log", notifications::Type::Error);
}
void AutoFire::onDisable() { hooks::input::PublishFire({}, false, false, false, 0); }
void AutoFire::onUpdate() {
    if (!requestedEnabled()) return;
    if (!hooks::input::FireAvailable()) {
        reportUnavailable();
        setEnabled(false); onDisable(); return;
    }
    const auto* team = ModuleManager::Get().Find("Team");
    const auto delay = std::isfinite(delay_.get()) ? static_cast<unsigned>(std::clamp(std::round(delay_.get()), 0., 500.)) : 0;
    hooks::input::PublishFire({delay, scoped_.get()}, true, team && team->requestedEnabled(), keybind().hold(), keybind().keybind());
    const auto now = GetTickCount64();
    if (now >= nextLog_) {
        nextLog_ = now + 5000;
        const auto d = hooks::input::ReadFireDebug();
        LogTo("CS2: Auto Fire status=%s frames=%u samples=%u targets=%u writes=%u deferred=%u lastDeferred=%s time=%.4f next=%.4f weapon=%s ray=%s",
            d.status, d.frames, d.samples, d.targets, d.writes, d.deferred, d.lastDeferred, d.time, d.nextAttack,
            sdk::weapons::FireStatus(), sdk::aim::ShotStatus());
    }
}
}
