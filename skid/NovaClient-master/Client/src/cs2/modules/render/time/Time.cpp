#include "cs2/modules/render/time/Time.h"
#include "cs2/hooks/TimeHook.h"
#include "ui/notifications/Notifications.h"
namespace nova::cs2::modules {
Time::Time() : Module("Time", "render", "General"),
    period_(addMode("Time", 0, {"Day", "Night"})),
    day_(addNumber("Day Brightness", 1, 3, .05, 1.25, "{:.2f}")),
    night_(addNumber("Night Brightness", .05, 1, .01, .20, "{:.2f}")) {}
std::string Time::suffix() const { return period_.get() == 1 ? "Night" : "Day"; }
void Time::onEnable() {
    if (!hooks::time::Available()) {
        notifications::Post("Time: environment adapter unavailable for this game version", notifications::Type::Error);
        setEnabled(false);
    }
}
void Time::onDisable() { hooks::time::Publish({}, false); }
void Time::onUpdate() {
    if (!requestedEnabled()) return;
    if (!hooks::time::Available()) {
        notifications::Post("Time stopped: environment adapter unavailable", notifications::Type::Error);
        setEnabled(false); onDisable(); return;
    }
    hooks::time::Publish({period_.get() == 1 ? sdk::environment::Period::Night : sdk::environment::Period::Day,
        static_cast<float>(day_.get()), static_cast<float>(night_.get())}, true);
}
}
