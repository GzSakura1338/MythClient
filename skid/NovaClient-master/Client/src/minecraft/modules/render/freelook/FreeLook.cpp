#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/java/api/game_freelook.h"
#include "minecraft/events/EventBus.h"
#include "minecraft/events/GameEvents.h"
#include <format>

namespace nova::module {
namespace {
class FreeLook final : public Module {
public:
    FreeLook() : Module("FreeLook", Category::Render, "General", "General"), key_(0xA4, true) {
        // Reuse the key picker/config format, but sample its physical state in
        // the render hook, so menus/key releases cannot desynchronize a toggle.
        key_.setGate([] { return false; });
        addSettings({Properties::PropDesc("Key", &key_)});
        invert_ = &addBool("Invert Pitch", false);
        lock_ = &addBool("Lock Pitch", true);
        custom_ = &addBool("Custom FOV", false);
        fov_ = &addNumber("FOV", 10, 150, 1, 90, {},
            [](double v) { return std::format("{:.0f}", v); }, [this] { return custom_->get(); });
        frame_ = event::EventBus::Get().Subscribe<event::RenderFrameEvent>([this](auto&) {
            Publish();
            java::UpdateFreeLook();
        }, event::Priority::Highest);
    }
    ~FreeLook() override { frame_.Reset(); java::StopFreeLook(); }
private:
    void Publish() {
        java::ConfigureFreeLook({isEnabled(), key_.hold(), invert_->get(), lock_->get(),
            custom_->get(), key_.keybind(), static_cast<float>(fov_->get())});
    }
    void onEnable() override { Publish(); }
    void onUpdate() override { Publish(); }
    void onDisable() override { Publish(); java::StopFreeLook(); }
    bind::BindProperty key_;
    Properties::BoolProperty *invert_{}, *lock_{}, *custom_{};
    Properties::NumberProp* fov_{};
    event::Subscription frame_;
};
}
std::unique_ptr<Module> MakeFreeLookModule() { return std::make_unique<FreeLook>(); }
}
