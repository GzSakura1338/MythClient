#include <cassert>
#include <deque>
#include <format>
#include <memory>
#include <iostream>
#include "config/settings/Properties.h"

namespace nova::module {
enum class Category { Combat };
class Module {
    std::deque<std::string> labels;
    std::vector<std::unique_ptr<Properties::AbstractProperty>> owned;
    template<class T, class... Args> Properties::PropDesc Row(std::string name, Args&&... args) {
        auto value = std::make_unique<T>(std::forward<Args>(args)...);
        auto* pointer = value.get(); owned.push_back(std::move(value));
        return {labels.emplace_back(std::move(name)).c_str(), pointer};
    }
public:
    Module(std::string, Category, const char*, const char*) {}
    Properties::PropDesc subNumber(std::string name, double min, double max, double step, double value,
                                  std::function<std::string(double)> format) {
        return Row<Properties::NumberProp>(std::move(name), min, max, step, value, std::move(format));
    }
    Properties::PropDesc subBool(std::string name, bool value) {
        return Row<Properties::BoolProperty>(std::move(name), value);
    }
    Properties::PropDesc subMode(std::string name, int value, std::vector<std::string> choices) {
        return Row<Properties::ModeProperty>(std::move(name), value, std::move(choices));
    }
};
}
// @AUTOBLOCK_SETTINGS@

class Probe : public nova::module::AutoBlockModuleBase {
public:
    Probe() : AutoBlockModuleBase("probe") {}
    using AutoBlockModuleBase::BuildBlockSettings;
    using AutoBlockModuleBase::ReadBlockSettings;
};

int main() {
    Probe module;
    auto rows = module.BuildBlockSettings();
    auto row = [&](const char* name) -> Properties::PropDesc& {
        for (auto& r : rows) if (std::string(r.getName()) == name) return r;
        std::abort();
    };
    auto set = [&](const char* name, int value) {
        static_cast<Properties::ModeProperty*>(row(name).getProp())->set(value);
    };
    assert(module.ReadBlockSettings().mode == nova::module::AutoBlockMode::Lag);
    for (int mode = 0; mode < 6; ++mode) {
        set("Mode", mode);
        assert(row("Force block animation").isVisible());
        auto* animation = static_cast<Properties::BoolProperty*>(row("Force block animation").getProp());
        animation->set(false);
        assert(!module.ReadBlockSettings().animation);
        animation->set(true);
        assert(module.ReadBlockSettings().animation);
    }
    set("Mode", 3);
    const auto settings = module.ReadBlockSettings();
    assert(settings.hypixel.mode == 0 && settings.hypixel.lagMode == 1 && settings.hypixel.aps == 10);
    assert(settings.hypixel.range == 6 && settings.hypixel.noSwap && !settings.requireRight);
    assert(row("NoSwap").isVisible() && !row("LagMode").isVisible());
    assert(!row("Range").isVisible() && !row("Hold ticks").isVisible() && !row("Require right mouse").isVisible());
    assert(row("AutoBlock Require Press").isVisible());
    set("HypixelMode",3);
    assert(row("LagMode").isVisible() && row("C09Instead").isVisible() && !row("NoSwap").isVisible());
    set("LagMode",4);
    assert(row("FullC09(Will Cause Damage Less)").isVisible() && !row("C09Instead").isVisible());
    set("HypixelMode",2);
    assert(row("PostBlock").isVisible() && row("StartBlinkTick").isVisible());
    assert(module.ReadBlockSettings().hypixel.startBlinkTick == 0);
    nova::json::Value stored;
    row("Mode").getProp()->save(stored);
    set("Mode",0); row("Mode").getProp()->load(stored);
    assert(module.ReadBlockSettings().mode == nova::module::AutoBlockMode::Hypixel);
    set("Mode",4);
    const auto slinky=module.ReadBlockSettings().slinky;
    assert(slinky.range==3.5 && slinky.hurtMs==200 && slinky.holdMs==150 && slinky.lagChance==0);
    assert(slinky.lagMs==100 && slinky.queueSize==3 && slinky.damageWindow==2000);
    assert(slinky.immediately && slinky.preventDelayAttacks && slinky.blockAgain && slinky.ignoreMobs);
    assert(!slinky.requireLeft && !slinky.requireRight && !slinky.damagedOnly && !slinky.ignoreTeammates && !slinky.ignoreBots && !slinky.debug);
    assert(row("Slinky Range").isVisible() && row("Block Immediately").isVisible() && row("Debug").isVisible());
    assert(!row("Range").isVisible() && !row("Require right mouse").isVisible() && !row("Maximum Hold Time (ms)").isVisible());
    static_cast<Properties::NumberProp*>(row("Slinky Range").getProp())->set(1.3);
    assert(module.ReadBlockSettings().slinky.range>1.29 && module.ReadBlockSettings().slinky.range<1.31);
    row("Mode").getProp()->save(stored);set("Mode",0);row("Mode").getProp()->load(stored);
    assert(module.ReadBlockSettings().mode==nova::module::AutoBlockMode::Slinky);
    set("Mode",0);assert(!row("Slinky Range").isVisible() && !row("Queue Size").isVisible());
    assert(!row("APS Mode").isVisible());
    set("Mode",5);
    assert(module.ReadBlockSettings().mode == nova::module::AutoBlockMode::LagNew);
    assert(row("APS Mode").isVisible() && row("Manual Left Click").isVisible());
    assert(row("Range").isVisible() && row("Require right mouse").isVisible());
    assert(!row("HypixelMode").isVisible() && !row("Maximum Hold Time (ms)").isVisible());
    assert(!module.ReadBlockSettings().lagNewManualClick);
    assert(module.ReadBlockSettings().lagNewApsMode == 0);
    for (int aps = 0; aps < 5; ++aps) {
        set("APS Mode", aps);
        assert(module.ReadBlockSettings().lagNewApsMode == aps);
        row("APS Mode").getProp()->save(stored);
        set("APS Mode", (aps + 1) % 5);
        row("APS Mode").getProp()->load(stored);
        assert(module.ReadBlockSettings().lagNewApsMode == aps);
    }
    static_cast<Properties::BoolProperty*>(row("Manual Left Click").getProp())->set(true);
    assert(module.ReadBlockSettings().lagNewManualClick);
    row("Mode").getProp()->save(stored); set("Mode",0); row("Mode").getProp()->load(stored);
    assert(module.ReadBlockSettings().mode == nova::module::AutoBlockMode::LagNew);
    std::cout << "PASS: production AutoBlock properties, defaults, conditional visibility, mode serialization\n";
}
