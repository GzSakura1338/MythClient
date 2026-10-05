#include "minecraft/modules/Module.h"
#include "minecraft/modules/ModuleManager.h"
#include "minecraft/modules/Registry.h"
#include "ui/clickgui/ClickGui.h"
#include "ui/hud/arraylist/ArrayListHud.h"

namespace nova::module {
namespace {
class ArrayListModule final : public Module {
public:
    ArrayListModule() : Module("ArrayList", Category::Render, "General", "Overlay") {
        enableOverlay();
        configRows_ = settings(); // Keep Enabled/Bind first in the historical schema.
        for (auto& row : view_.Options().ConfigRows()) configRows_.push_back(std::move(row));
        addSettings(view_.Options().Rows());
    }
    const std::vector<Properties::PropDesc>& configSettings() const override { return configRows_; }
private:
    bool renderWhenDisabled() const override { return true; }
    void onRender() override {
        entries_.clear();
        for (const auto& module : ModuleManager::Get().All()) {
            if (module.get() == this) continue;
            entries_.push_back({"minecraft." + module->name(),module->name(),module->suffix(),
                module->isEnabled(),module->hiddenInArrayList()});
        }
        view_.Draw(isEnabled(),clickgui::IsOpen(),entries_);
    }
    ui::arraylist::ArrayListHud view_;
    std::vector<Properties::PropDesc> configRows_;
    std::vector<ui::arraylist::Entry> entries_;
};
}
std::unique_ptr<Module> MakeArrayListModule() { return std::make_unique<ArrayListModule>(); }
}
