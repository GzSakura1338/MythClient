#pragma once

#include <optional>
#include "minecraft/java/api/base.h"
#include "minecraft/java/api/game_entity.h"

namespace nova::java {

// Membership in the connection's complete UUID list, including unlisted players.
// Missing connection/entity or JNI failure is unknown, not an empty player list.
std::optional<bool> OnlinePlayerListContains(const EntityHandle& entity);
void ReleasePlayerListAccess(JNIEnv* env);

}
