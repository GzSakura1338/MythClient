#pragma once
#include <memory>
#include <span>
#include "config/settings/Properties.h"

namespace nova::ui::arraylist {
struct Entry {
    std::string id, name, suffix;
    bool enabled = false;
    bool hidden = false;
};

struct Settings {
    Properties::BoolProperty animations{true};
    Properties::NumberProp animationSpeed{.1,1,.05,.4,"{:.2f}"};
    Properties::ModeProperty slideFrom{0,{"Auto","Left","Right"}};
    Properties::NumberProp size{50,200,5,100,"{:.0f}%"};
    Properties::NumberProp horizontalPadding{0,20,1,10}, verticalPadding{0,10,1,4};
    Properties::ColorProp textColor{0xFFFFEAE5,false};
    Properties::BoolProperty textShadow{false}, background{true};
    Properties::ColorProp backgroundColor{0xCC331914,true};
    Properties::BoolProperty sidebar{true}, sidebarFollowTextColor{true};
    Properties::ColorProp sidebarColor{0xFFFFEAE5,true};
    Properties::NumberProp sidebarWidth{1,10,1,4};
    Properties::BoolProperty sidebarGradient{true}, sidebarShadow{false};
    std::vector<Properties::PropDesc> Rows();
    // Preserve the historical flat color keys, independently of UI grouping.
    std::vector<Properties::PropDesc> ConfigRows();
};

class ArrayListHud {
public:
    ArrayListHud();
    ~ArrayListHud();
    ArrayListHud(const ArrayListHud&) = delete;
    ArrayListHud& operator=(const ArrayListHud&) = delete;
    Settings& Options() { return settings_; }
    // Entries are only borrowed during this call. Exiting rows retain their text.
    void Draw(bool enabled, bool editing, std::span<const Entry> entries);
private:
    struct Impl;
    Settings settings_;
    std::unique_ptr<Impl> impl_;
};
}
