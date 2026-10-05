#include <format>
#include <memory>
#include <string>

#include "minecraft/modules/Module.h"
#include "minecraft/modules/Registry.h"
#include "minecraft/java/api/base.h"

#include <windows.h>

namespace nova::module {
namespace {

class ExampleModule final : public Module {
public:
    ExampleModule() : Module("Example", Category::Combat, "Aimbot", "General") {
        target_ = &addBool("Target", true, {
            subBool("Players", true),
            subBool("Mobs", false),
            subNumber("Max distance", 1, 32, 0.5, 6.0, [](double value) {
                return std::format("{:.1f} blocks", value);
            }),
        });
        chance_ = &addNumber("Hit-chance", 0, 1, 0.01, 0, {}, [](double value) {
            if (value == 0) return std::string("Auto");
            return std::format("{:.0f}%", value * 100);
        });
        minDamage_ = &addNumber("Min-damage", 0, 120, 1, 0, {}, [](double value) {
            if (value == 0) return std::string("Lethal");
            if (value > 100) return std::format("HP + {:.0f}", value - 100);
            return std::format("{:.0f} HP", value);
        });
        priority_ = &addMode("Priority", 0, {"Hit-chance", "Damage"});
    }

private:
    void onEnable() override { LogTo("Example: enabled"); }
    void onDisable() override { LogTo("Example: disabled"); }

    void onUpdate() override {
        const ULONGLONG now = GetTickCount64();
        if (now - lastLog_ < 5000)
            return;
        lastLog_ = now;
        LogTo("Example: target=%s chance=%.2f min-damage=%.0f priority=%s",
              target_->get() ? "on" : "off", chance_->get(), minDamage_->get(),
              priority_->getValueName(priority_->get()).data());
    }

    Properties::BoolProperty* target_ = nullptr;
    Properties::NumberProp* chance_ = nullptr;
    Properties::NumberProp* minDamage_ = nullptr;
    Properties::ModeProperty* priority_ = nullptr;
    ULONGLONG lastLog_ = 0;
};

}

std::unique_ptr<Module> MakeExampleModule() {
    return std::make_unique<ExampleModule>();
}

}
