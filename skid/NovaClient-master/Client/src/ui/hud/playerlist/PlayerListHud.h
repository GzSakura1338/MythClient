#pragma once
#include <memory>
#include "config/settings/Properties.h"
#include "ui/hud/playerlist/PlayerListModel.h"

namespace nova::ui::playerlist {
struct Settings {
    Properties::NumberProp size{50,200,5,100,"{:.0f}%"};
    Properties::BoolProperty direction{true}, shadow{false}, numberRolling{true};
    std::vector<Properties::PropDesc> Rows() {
        return {{"Size",&size},{"Direction",&direction},{"Text Shadow",&shadow},{"Number Rolling",&numberRolling}};
    }
};
class PlayerListHud {
public:
    PlayerListHud();
    ~PlayerListHud();
    Settings& Options() { return settings_; }
    void Draw(bool enabled, bool editing, std::span<const Snapshot> entries,
              std::uint64_t generation, float cameraYaw);
private:
    struct Impl;
    Settings settings_;
    std::unique_ptr<Impl> impl_;
};
}
