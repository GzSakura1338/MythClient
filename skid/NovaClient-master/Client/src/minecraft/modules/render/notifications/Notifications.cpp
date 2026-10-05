#include "minecraft/modules/Module.h"
#include "minecraft/modules/ModuleManager.h"
#include "minecraft/modules/Registry.h"
#include "config/ConfigManager.h"
#include "ui/notifications/NotificationHud.h"
#include "ui/notifications/Notifications.h"
#include "imgui.h"
#include <unordered_map>

namespace nova::module {
namespace {
class NotificationsModule final : public Module {
public:
    NotificationsModule() : Module("Notifications", Category::Render, "General", "HUD") {
        enableOverlay();
        setEnabled(true);
        addSettings(view_.Settings().Rows());
    }
private:
    bool renderWhenDisabled() const override { return true; }
    void onRender() override {
        if (!ImGui::GetCurrentContext() || !ImGui::GetFont()) return;
        const bool enabled = static_cast<const Properties::BoolProperty*>(enabledProperty())->get();
        const auto revision = config::ConfigManager::Get().LoadSerial();
        const bool baseline = !sampled_ || revision != loadSerial_;
        loadSerial_ = revision;
        sampled_ = true;
        for (const auto& module : ModuleManager::Get().All()) {
            const bool state = static_cast<const Properties::BoolProperty*>(module->enabledProperty())->get();
            const auto [entry, inserted] = states_.try_emplace(module.get(), state);
            if (!inserted && entry->second != state && !baseline && enabled && view_.Settings().modules.get() &&
                module.get() != this && module->name() != "ClickGUI") {
                notifications::Post(module->name() + (state ? " enabled" : " disabled"),
                    state ? notifications::Type::Success : notifications::Type::Info, notifications::Source::Module);
            }
            entry->second = state;
        }
        view_.Draw(enabled);
    }
    ui::NotificationHud view_;
    std::unordered_map<const Module*, bool> states_;
    unsigned int loadSerial_ = 0;
    bool sampled_ = false;
};
}
std::unique_ptr<Module> MakeNotificationsModule() { return std::make_unique<NotificationsModule>(); }
}
