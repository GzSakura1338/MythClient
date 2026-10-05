#include "cs2/modules/render/chams/Chams.h"
#include "cs2/hooks/ChamsHook.h"
#include "ui/notifications/Notifications.h"
namespace nova::cs2::modules {
Chams::Chams() : Module("Chams", "render", "General"),
    material_(addMode("Material", 0, {"White", "Illuminate"})),
    throughWalls_(addBool("Through Walls", false)) {
    addSettings({{"Visible Color", &visible_}, {"Hidden Color", &hidden_, {}, [this] { return throughWalls_.get(); }}});
}
std::string Chams::suffix() const { return material_.get() == 1 ? "Illuminate" : "White"; }
void Chams::onEnable() {
    if (!hooks::chams::Available()) {
        notifications::Post("Chams: material adapter unavailable for this game version", notifications::Type::Error);
        setEnabled(false);
    }
}
void Chams::onDisable() { hooks::chams::Publish({}); }
void Chams::onUpdate() {
    if (!requestedEnabled()) return;
    if (!hooks::chams::Available()) {
        notifications::Post("Chams stopped: material adapter unavailable", notifications::Type::Error);
        setEnabled(false); onDisable(); return;
    }
    hooks::chams::Publish({true, material_.get() == 1, throughWalls_.get(), visible_.valueAbgr(), hidden_.valueAbgr()});
}
}
