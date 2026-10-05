#include <memory>

#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/compat/Profile.h"
#include "ui/clickgui/ClickGui.h"
#include "imgui.h"

#include <windows.h>

namespace nova::module {
namespace {

class ClickGuiModule final : public Module {
public:
    ClickGuiModule() : Module("ClickGUI", Category::Render, "General", "General") {
        setBind(VK_RSHIFT);
        keepBindWhileMenuOpen();
        setBindGate([] {
            if (nova::clickgui::IsOpen())
                return !(ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput);
            return !compat::ChatScreenOpen();
        });
        mode_ = &addMode("Mode", static_cast<int>(nova::clickgui::Mode::Panel),
                         {"Panel", "Classic"});
    }

private:
    void onEnable() override {
        nova::clickgui::SetOpen(true);
        Publish();
    }
    void onDisable() override { nova::clickgui::SetOpen(false); }

    void onUpdate() override { Publish(); }

    void Publish() {
        nova::clickgui::SetMode(static_cast<nova::clickgui::Mode>(mode_->get()));
    }

    Properties::ModeProperty* mode_ = nullptr;
};

}

std::unique_ptr<Module> MakeClickGuiModule() {
    return std::make_unique<ClickGuiModule>();
}

}
