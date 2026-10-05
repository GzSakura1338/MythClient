#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "bind/BindProperty.h"
#include "minecraft/modules/Category.h"
#include "config/settings/Properties.h"

namespace nova::module {

class Module {
public:
    Module(std::string name, Category category, std::string_view page, std::string_view subpage);
    virtual ~Module() = default;

    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] Category category() const { return category_; }
    [[nodiscard]] const std::string& page() const { return page_; }
    [[nodiscard]] const std::string& subpage() const { return subpage_; }

    [[nodiscard]] bool isEnabled() const { return enabled_; }
    void setEnabled(bool value) { enabledSetting_.set(value); }

    [[nodiscard]] bool hiddenInArrayList() const { return hiddenInArrayList_.get(); }
    void setHiddenInArrayList(bool value) { hiddenInArrayList_.set(value); }
    [[nodiscard]] Properties::BoolProperty& hiddenInArrayListProperty() {
        return hiddenInArrayList_;
    }

    [[nodiscard]] int bind() const { return keybind_.keybind(); }
    void setBind(int virtualKey) { keybind_.setKeybind(virtualKey); }

    [[nodiscard]] bool drawsOverlay() const { return overlay_; }
    [[nodiscard]] const std::vector<Properties::PropDesc>& settings() const { return settings_; }
    // UI grouping may differ from stable serialized keys for existing configs.
    [[nodiscard]] virtual const std::vector<Properties::PropDesc>& configSettings() const { return settings_; }
    [[nodiscard]] const Properties::AbstractProperty* enabledProperty() const { return &enabledSetting_; }
    [[nodiscard]] Properties::AbstractProperty* enabledProperty() { return &enabledSetting_; }

    void tick();

    void renderOverlay();

    // A short word the module wants shown next to its name -- the scaffold's
    // "Telly"/"Snap"/"Normal", say. Empty means nothing is appended. Starain calls
    // this getSuffix and puts it after the name in its array list.
    [[nodiscard]] virtual std::string suffix() const { return {}; }

protected:
    void replaceSettings(std::vector<Properties::PropDesc> rows) { settings_ = std::move(rows); }
    virtual void onEnable() {}
    virtual void onDisable() {}
    virtual void onUpdate() {}
    virtual void onRender() {}
    virtual bool renderWhenDisabled() const { return false; }

    void enableOverlay() { overlay_ = true; }

    void keepBindWhileMenuOpen() { bindWhileMenuOpen_ = true; }
    void setBindGate(std::function<bool()> gate) { keybind_.setGate(std::move(gate)); }

    void addSettings(std::vector<Properties::PropDesc> rows) {
        for (auto& row : rows) settings_.push_back(std::move(row));
    }

    Properties::BoolProperty& addBool(std::string label, bool defaultValue = false,
                                      std::vector<Properties::PropDesc> subProps = {},
                                      Properties::PropDesc::Visibility visible = {});
    Properties::NumberProp& addNumber(std::string label, double min, double max, double step,
                                      double defaultValue,
                                      std::vector<Properties::PropDesc> subProps = {},
                                      std::function<std::string(double)> format = {},
                                      Properties::PropDesc::Visibility visible = {});
    Properties::ModeProperty& addMode(std::string label, int defaultIndex,
                                      std::vector<std::string> values,
                                      std::vector<Properties::PropDesc> subProps = {},
                                      Properties::PropDesc::Visibility visible = {});
    Properties::ColorProp& addColor(std::uint32_t defaultColor = 0xFFFFFFFFu,
                                    bool allowAlpha = true);

    Properties::PropDesc subBool(std::string label, bool defaultValue = false);
    Properties::PropDesc subNumber(std::string label, double min, double max, double step,
                                   double defaultValue,
                                   std::function<std::string(double)> format = {});
    Properties::PropDesc subMode(std::string label, int defaultIndex,
                                 std::vector<std::string> values);

private:
    template <typename T, typename... A>
    T& own(std::string label, std::vector<Properties::PropDesc> subProps,
           Properties::PropDesc::Visibility visible, A&&... args) {
        auto property = std::make_unique<T>(std::forward<A>(args)...);
        T& reference = *property;
        owned_.push_back(std::move(property));
        settings_.emplace_back(intern(std::move(label)), &reference, std::move(subProps),
                               std::move(visible));
        return reference;
    }

    const char* intern(std::string label) { return labels_.emplace_back(std::move(label)).c_str(); }

    std::string name_;
    Category category_;
    std::string page_;
    std::string subpage_;

    nova::bind::BindProperty keybind_;
    bool enabled_ = false;
    bool overlay_ = false;
    bool bindWhileMenuOpen_ = false;
    Properties::BoolProperty hiddenInArrayList_{false};

    Properties::BoolProperty enabledSetting_{false};

    std::deque<std::string> labels_;
    std::vector<std::unique_ptr<Properties::AbstractProperty>> owned_;
    std::vector<Properties::PropDesc> settings_;
};

}
