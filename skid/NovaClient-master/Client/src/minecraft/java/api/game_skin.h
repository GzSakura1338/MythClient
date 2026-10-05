#pragma once

#include "minecraft/java/api/game_entity.h"
#include <cstdint>

namespace nova::java {

// --- target hud --------------------------------------------------------------
//
// A skin handle: legacy GL texture name or an owned renderer-neutral skin.
//
// The skin sheet is 64x64 and holds the head twice: the 8x8 face at (8, 8) and the
// 8x8 hat layer at (40, 8), which is drawn over it. Both are sampled by the caller,
// so all this has to hand back is the texture they are sampled from.
//
// The read happens on the game thread and the answer travels to the render thread
// as a plain integer, because this client's frame loop is not the game thread and
// nothing JNI-shaped may be touched while a frame is being drawn. Zero means there
// is nothing to draw: the entity is not a client player, the skin is not registered
// with the texture manager yet, or its texture has not been uploaded.

std::uint64_t PlayerSkinTexture(const EntityHandle& entity);

}
