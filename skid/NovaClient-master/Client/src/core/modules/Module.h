#pragma once
#include <deque>
#include <memory>
#include <string>
#include <vector>
#include "bind/BindProperty.h"
#include "contracts/UiCatalog.h"

namespace nova::modules {
// Submodes own their settings and transient state. Only the selected mode of an
// enabled module receives callbacks; switching first stops the previous mode.
class Mode {
public:
    explicit Mode(std::string name) : name_(std::move(name)) {}
    virtual ~Mode() = default;
    const std::string& name() const { return name_; }
    virtual std::vector<Properties::PropDesc> settings() { return {}; }
    virtual void onEnable() {}
    virtual void onDisable() {}
    virtual void onUpdate() {}
    virtual void onRender() {}
private:
    std::string name_;
};

// UI-thread lifecycle, like Minecraft's Module. Engine-thread work must be
// scheduled through a game host hook, not performed from these callbacks.
class Module {
public:
    Module(std::string name, std::string category, std::string page = "General",
           bool hud = false, bool enabled = false);
    virtual ~Module() = default;
    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;
    const std::string& name() const { return name_; }
    const std::string& category() const { return category_; }
    bool isEnabled() const { return running_; }
    bool requestedEnabled() const { return enabled_.get(); }
    void setEnabled(bool value) { enabled_.set(value); bind_.syncState(value); }
    bind::BindProperty& keybind() { return bind_; }
    const bind::BindProperty& keybind() const { return bind_; }
    Properties::BoolProperty& hiddenInArrayListProperty() { return hidden_; }
    bool hiddenInArrayList() const { return hidden_.get(); }
    Properties::ModeProperty* modeProperty() { return mode_.get(); }
    virtual std::string suffix() const;
    const std::vector<Properties::PropDesc>& settings() const { return settings_; }
    virtual bool persistEnabled() const { return true; }
    ui::Feature feature();
    void tick();
    void renderOverlay();
    void shutdown();

protected:
    virtual void onEnable() {}
    virtual void onDisable() {}
    virtual void onUpdate() {}
    virtual void onRender() {}
    virtual bool renderWhenDisabled() const { return false; }
    void keepBindWhileMenuOpen() { bindWhileMenuOpen_ = true; }
    void addSettings(std::vector<Properties::PropDesc> rows);
    void setModes(std::vector<std::unique_ptr<Mode>> modes, int defaultIndex = 0);
    Properties::BoolProperty& addBool(std::string label, bool value = false);
    Properties::NumberProp& addNumber(std::string label, double min, double max,
                                     double step, double value, const char* format = "{}");
    Properties::ModeProperty& addMode(std::string label, int value, std::vector<std::string> names);

private:
    template<class T, class... Args> T& own(std::string label, Args&&... args) {
        auto property = std::make_unique<T>(std::forward<Args>(args)...);
        auto& result = *property;
        owned_.push_back(std::move(property));
        settings_.emplace_back(labels_.emplace_back(std::move(label)).c_str(), &result);
        return result;
    }
    std::string name_, category_, page_;
    bool hud_ = false, running_ = false, stopped_ = false, bindWhileMenuOpen_ = false;
    Properties::BoolProperty enabled_, hidden_{false};
    bind::BindProperty bind_;
    std::deque<std::string> labels_;
    std::vector<std::unique_ptr<Properties::AbstractProperty>> owned_;
    std::vector<Properties::PropDesc> settings_;
    std::vector<std::unique_ptr<Mode>> modes_;
    std::unique_ptr<Properties::ModeProperty> mode_;
    int activeMode_ = -1;
};
}
