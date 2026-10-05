#pragma once
#include <memory>
#include <span>
#include "config/settings/Properties.h"
#include "ui/overlay/Portrait.h"

namespace nova::ui::targethud {
struct Snapshot {
    bool valid=false;
    int id=0;
    std::string name;
    float health=0,absorption=0,maxHealth=20,distance=0;
    int hurtTime=0;
    Portrait portrait;
};
struct Settings {
    Properties::NumberProp fade{0,2000,50,400,"{:.0f} ms"},width{60,240,5,130,"{:.0f}"};
    Properties::ModeProperty health{0,{"Value","Percent","Both","None"}};
    Properties::BoolProperty distance{false};
    Properties::ModeProperty anchor{0,{"Left","Right","Auto"}},stack{0,{"Down","Up","Auto"}};
    Properties::ColorProp background{0xCC331914,true},text{0xFFFFEAE5,false},bar{0xFFFFEAE5,false};
    Properties::BoolProperty shadow{false},hurtFlash{true};
    std::vector<Properties::PropDesc> Rows() {
        std::vector<Properties::PropDesc> rows{{"Fade",&fade},{"Width",&width},{"Health",&health},
            {"Distance",&distance},{"Anchor",&anchor},{"Stack",&stack}};
        for(auto* color:{&background,&text})for(auto& row:color->descriptors())rows.push_back(std::move(row));
        rows.emplace_back("Text Shadow",&shadow);
        for(auto& row:bar.descriptors())rows.push_back(std::move(row));
        rows.emplace_back("Hurt Flash",&hurtFlash);
        return rows;
    }
};
class TargetHud {
public:
    TargetHud();
    ~TargetHud();
    TargetHud(const TargetHud&)=delete;
    TargetHud& operator=(const TargetHud&)=delete;
    Settings& Options() {return settings_;}
    void RestartTiming();
    // Borrowed only during this synchronous call. Cards own their exit snapshots.
    void Draw(bool editing,std::span<const Snapshot> snapshots);
private:
    struct Impl;
    Settings settings_;
    std::unique_ptr<Impl> impl_;
};
}
