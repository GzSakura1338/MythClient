#pragma once

#include "minecraft/java/api/game_teams.h"

namespace nova::module::teams {

enum class Mode { Scoreboard = 0, Color = 1, Armor = 2 };

inline bool Matches(const java::TeamInfo& self, const java::TeamInfo& other, Mode mode) {
    if (mode == Mode::Armor)
        return self.headColor && other.headColor && self.chestColor && other.chestColor &&
               self.headColor == other.headColor && self.chestColor == other.chestColor;
    return (mode == Mode::Scoreboard || mode == Mode::Color) && !self.name.empty() && !other.name.empty() && self.color.has_value() && other.color.has_value() && self.color == other.color;
}

bool IsTeammate(const java::EntityHandle& self, const java::EntityHandle& other);
bool IsTeammate(const java::EntityHandle& other);

}
