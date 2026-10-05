#pragma once
#include <memory>
#include "config/settings/Properties.h"
#include "ui/overlay/Portrait.h"

namespace nova::ui::autoblockhud {
struct Snapshot {
    bool active = false;
    float percent = 0;
    Portrait portrait;
};
struct Settings {
    Properties::NumberProp duration{100,1000,50,300,"{:.0f} ms"};
    Properties::ModeProperty expandFrom{0,{"Auto","Left","Right"}};
    Properties::NumberProp width{160,320,5,200,"{:.0f}"};
    Properties::ColorProp background{0xCC331914,true},text{0xFFFFEAE5,false},bar{0xFFFFEAE5,false};
    Properties::BoolProperty shadow{false};
    std::vector<Properties::PropDesc> Rows() {
        std::vector<Properties::PropDesc> rows{{"Animation time",&duration},{"Expand from",&expandFrom},{"Width",&width}};
        for(auto* color:{&background,&text,&bar})for(auto& row:color->descriptors())rows.push_back(std::move(row));
        rows.emplace_back("Text Shadow",&shadow);
        return rows;
    }
};
class AutoBlockHud {
public:
    AutoBlockHud();
    ~AutoBlockHud();
    AutoBlockHud(const AutoBlockHud&) = delete;
    AutoBlockHud& operator=(const AutoBlockHud&) = delete;
    Settings& Options() { return settings_; }
    void Draw(bool enabled,bool editing,const Snapshot& snapshot);
private:
    struct Impl;
    Settings settings_;
    std::unique_ptr<Impl> impl_;
};
}
