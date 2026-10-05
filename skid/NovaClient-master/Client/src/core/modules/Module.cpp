#include "core/modules/Module.h"
#include "bind/PhysicalKey.h"
#include "ui/clickgui/ClickGui.h"
#include <stdexcept>

namespace nova::modules {
Module::Module(std::string name, std::string category, std::string page, bool hud, bool enabled)
    : name_(std::move(name)), category_(std::move(category)), page_(std::move(page)), hud_(hud), enabled_(enabled) {
    bind_.syncState(enabled);
    bind_.setStateChangeConsumer([this](bool value) { if (!stopped_) enabled_.set(value); });
    bind_.setGate([this] { return !stopped_ && (bindWhileMenuOpen_ || !clickgui::IsOpen()); });
    settings_ = {{"Enabled", &enabled_}, {"Bind", &bind_}};
}
void Module::addSettings(std::vector<Properties::PropDesc> rows) {
    for (auto& row : rows) settings_.push_back(std::move(row));
}
Properties::BoolProperty& Module::addBool(std::string label, bool value) {
    return own<Properties::BoolProperty>(std::move(label), value);
}
Properties::NumberProp& Module::addNumber(std::string label, double min, double max, double step, double value, const char* format) {
    return own<Properties::NumberProp>(std::move(label), min, max, step, value, format);
}
Properties::ModeProperty& Module::addMode(std::string label, int value, std::vector<std::string> names) {
    return own<Properties::ModeProperty>(std::move(label), value, std::move(names));
}
void Module::setModes(std::vector<std::unique_ptr<Mode>> modes, int defaultIndex) {
    if (mode_ || running_ || modes.empty()) throw std::logic_error("Register module modes once before startup");
    std::vector<std::string> names;
    for (const auto& mode : modes) {
        if (!mode || mode->name().empty() || std::find(names.begin(), names.end(), mode->name()) != names.end())
            throw std::invalid_argument("Invalid or duplicate module mode");
        names.push_back(mode->name());
    }
    modes_ = std::move(modes);
    mode_ = std::make_unique<Properties::ModeProperty>(defaultIndex, std::move(names));
    std::vector<Properties::PropDesc> children;
    for (size_t i = 0; i < modes_.size(); ++i) {
        auto rows = modes_[i]->settings();
        if (!rows.empty()) children.emplace_back(modes_[i]->name().c_str(), nullptr, std::move(rows),
            [this, i] { return mode_->get() == static_cast<int>(i); });
    }
    settings_.emplace_back("Mode", mode_.get(), std::move(children));
}
std::string Module::suffix() const {
    return mode_ ? std::string(mode_->getValueName(mode_->get())) : std::string{};
}
ui::Feature Module::feature() {
    return {name_, name_, category_, page_, "General", hud_, &enabled_, &hidden_, settings_};
}
void Module::tick() {
    if (stopped_) return;
    if (bind_.hold()) {
        DWORD process = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &process);
        bind_.reconcileHold(process == GetCurrentProcessId() && bind::IsPhysicalBindingDown(bind_.keybind()));
    }
    const bool desired = enabled_.get();
    const int selected = mode_ ? mode_->get() : -1;
    if (activeMode_ >= 0 && (!desired || selected != activeMode_)) {
        modes_[activeMode_]->onDisable(); activeMode_ = -1;
    }
    if (running_ != desired) {
        running_ = desired;
        bind_.syncState(desired);
        if (desired) onEnable(); else onDisable();
    }
    if (!running_) return;
    if (selected >= 0 && activeMode_ < 0) {
        activeMode_ = selected; modes_[activeMode_]->onEnable();
    }
    onUpdate();
    if (activeMode_ >= 0) modes_[activeMode_]->onUpdate();
}
void Module::renderOverlay() {
    if (stopped_) return;
    if (running_ || renderWhenDisabled()) onRender();
    if (running_ && activeMode_ >= 0) modes_[activeMode_]->onRender();
}
void Module::shutdown() {
    if (stopped_) return;
    stopped_ = true;
    if (activeMode_ >= 0) { modes_[activeMode_]->onDisable(); activeMode_ = -1; }
    if (running_) { running_ = false; onDisable(); }
    bind_.syncState(false);
}
}
