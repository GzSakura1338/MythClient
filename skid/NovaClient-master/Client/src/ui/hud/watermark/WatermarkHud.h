#pragma once
#include <memory>
#include <string>
#include "config/settings/Properties.h"

namespace nova::ui::watermark {
struct Snapshot {
    std::string player="--",server="--",coordinates="--",version;
    int ping=-1;
    double speed=-1,tps=-1;
    // Hosts choose units and whether unavailable game data occupies a row item.
    // Defaults preserve existing Minecraft presentation.
    std::string speedUnit="b/s";
    bool hideUnavailable=false;
};
struct Settings {
    Properties::BoolProperty name{true},fps{true},clock{false},server{false},player{false},
        ping{false},tps{false},coordinates{false},speed{false},version{false};
    Properties::ModeProperty corner{0,{"Top left","Top right","Bottom left","Bottom right"}};
    Properties::NumberProp margin{0,64,1,12,"{:.0f} px"},animation{100,1000,50,300,"{:.0f} ms"};
    Properties::ColorProp background{0xCC331914,true},text{0xFFFFEAE5,false};
    std::vector<Properties::PropDesc> Rows() {
        std::vector<Properties::PropDesc> rows{{"Name",&name},{"FPS",&fps},{"Clock",&clock},
            {"Server IP",&server},{"Player ID",&player},{"Ping",&ping},{"TPS",&tps},
            {"Coordinates",&coordinates},{"Speed",&speed},{"Game version",&version},
            {"Corner",&corner},{"Margin",&margin},{"Animation time",&animation}};
        for(auto* color:{&background,&text})for(auto& row:color->descriptors())rows.push_back(std::move(row));
        return rows;
    }
};
class WatermarkHud {
public:
    WatermarkHud();
    ~WatermarkHud();
    WatermarkHud(const WatermarkHud&)=delete;
    WatermarkHud& operator=(const WatermarkHud&)=delete;
    Settings& Options() { return settings_; }
    void Draw(bool enabled,const Snapshot& snapshot);
private:
    struct Impl;
    Settings settings_;
    std::unique_ptr<Impl> impl_;
};
}
