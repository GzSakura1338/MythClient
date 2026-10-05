#include "minecraft/modules/Module.h"
#include "bind/PhysicalKey.h"
#include "utils/Keyboard.h"

#include "minecraft/java/api/game_click.h"
#include "ui/clickgui/ClickGui.h"

namespace nova::module {

Module::Module(std::string name, Category category, std::string_view page, std::string_view subpage)
    : name_(std::move(name)), category_(category), page_(page), subpage_(subpage) {
    keybind_.setStateChangeConsumer([this](bool state) { setEnabled(state); });
    keybind_.setGate([this] {
        if (!bindWhileMenuOpen_ && nova::java::ScreenOpen())
            return false;
        return !nova::clickgui::IsOpen() || bindWhileMenuOpen_;
    });

    settings_.emplace_back(intern("Enabled"), &enabledSetting_);
    settings_.emplace_back(intern("Bind"), &keybind_);
}

void Module::tick() {
    // Key-up may be swallowed by bind capture, or delivered to another window.
    // Only release an accepted hold; never activate from polling/repeated keys.
    if (keybind_.hold())
        keybind_.reconcileHold(nova::input::IsProcessForeground() &&
                              nova::bind::IsPhysicalBindingDown(keybind_.keybind()));
    const bool now = enabledSetting_.get();
    if (now != enabled_) {
        enabled_ = now;
        keybind_.syncState(now);
        if (now)
            onEnable();
        else
            onDisable();
    }
    if (enabled_)
        onUpdate();
}

void Module::renderOverlay() {
    if (overlay_ && (enabled_ || renderWhenDisabled()))
        onRender();
}

Properties::BoolProperty& Module::addBool(std::string label, bool defaultValue,
                                          std::vector<Properties::PropDesc> subProps,
                                          Properties::PropDesc::Visibility visible) {
    return own<Properties::BoolProperty>(std::move(label), std::move(subProps),
                                         std::move(visible), defaultValue);
}

Properties::NumberProp& Module::addNumber(std::string label, double min, double max, double step,
                                          double defaultValue,
                                          std::vector<Properties::PropDesc> subProps,
                                          std::function<std::string(double)> format,
                                          Properties::PropDesc::Visibility visible) {
    if (!format) {
        return own<Properties::NumberProp>(std::move(label), std::move(subProps),
                                           std::move(visible), min, max, step, defaultValue, "{}");
    }
    return own<Properties::NumberProp>(std::move(label), std::move(subProps), std::move(visible),
                                       min, max, step, defaultValue, std::move(format));
}

Properties::ModeProperty& Module::addMode(std::string label, int defaultIndex,
                                          std::vector<std::string> values,
                                          std::vector<Properties::PropDesc> subProps,
                                          Properties::PropDesc::Visibility visible) {
    return own<Properties::ModeProperty>(std::move(label), std::move(subProps),
                                         std::move(visible), defaultIndex, std::move(values));
}

Properties::ColorProp& Module::addColor(std::uint32_t defaultColor, bool allowAlpha) {
    auto property = std::make_unique<Properties::ColorProp>(defaultColor, allowAlpha);
    Properties::ColorProp& reference = *property;
    owned_.push_back(std::move(property));
    for (auto& row : reference.descriptors())
        settings_.push_back(std::move(row));
    return reference;
}

Properties::PropDesc Module::subBool(std::string label, bool defaultValue) {
    auto property = std::make_unique<Properties::BoolProperty>(defaultValue);
    Properties::BoolProperty& reference = *property;
    owned_.push_back(std::move(property));
    return Properties::PropDesc(intern(std::move(label)), &reference);
}

Properties::PropDesc Module::subNumber(std::string label, double min, double max, double step,
                                       double defaultValue,
                                       std::function<std::string(double)> format) {
    std::unique_ptr<Properties::NumberProp> property;
    if (format)
        property = std::make_unique<Properties::NumberProp>(min, max, step, defaultValue,
                                                            std::move(format));
    else
        property = std::make_unique<Properties::NumberProp>(min, max, step, defaultValue, "{}");
    Properties::NumberProp& reference = *property;
    owned_.push_back(std::move(property));
    return Properties::PropDesc(intern(std::move(label)), &reference);
}

Properties::PropDesc Module::subMode(std::string label, int defaultIndex,
                                     std::vector<std::string> values) {
    auto property = std::make_unique<Properties::ModeProperty>(defaultIndex, std::move(values));
    Properties::ModeProperty& reference = *property;
    owned_.push_back(std::move(property));
    return Properties::PropDesc(intern(std::move(label)), &reference);
}

}
