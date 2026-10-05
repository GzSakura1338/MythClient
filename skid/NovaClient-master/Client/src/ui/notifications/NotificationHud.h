#pragma once
#include <memory>
#include "config/settings/Properties.h"

namespace nova::ui {
struct NotificationSettings {
    Properties::NumberProp duration{1,8,.5,3,"{:.1f} s"};
    Properties::NumberProp animation{100,1000,50,300,"{:.0f} ms"};
    Properties::NumberProp offset{10,200,5,35};
    Properties::NumberProp gap{2,24,1,8};
    Properties::NumberProp limit{1,6,1,4};
    Properties::BoolProperty modules{true}, configs{true};
    std::vector<Properties::PropDesc> Rows() {
        return {{"Display time",&duration},{"Animation time",&animation},{"Below crosshair",&offset},
            {"Spacing",&gap},{"Max visible",&limit},{"Module toggles",&modules},{"Config messages",&configs}};
    }
};

class NotificationHud {
public:
    NotificationHud();
    ~NotificationHud();
    NotificationHud(const NotificationHud&) = delete;
    NotificationHud& operator=(const NotificationHud&) = delete;
    NotificationSettings& Settings() { return settings_; }
    void Draw(bool enabled);
private:
    struct Impl;
    NotificationSettings settings_;
    std::unique_ptr<Impl> impl_;
};
}
