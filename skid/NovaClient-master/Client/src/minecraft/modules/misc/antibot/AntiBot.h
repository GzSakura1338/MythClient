#pragma once

#include "minecraft/java/api/game_entity.h"

namespace nova::module::antibot {

// Starain 1.20.1: enabled && !onlinePlayerIds.contains(entity.uuid).
// Deliberately no player-only, name, age, health or invisibility heuristic.
bool IsBot(const java::EntityHandle& entity);

}
