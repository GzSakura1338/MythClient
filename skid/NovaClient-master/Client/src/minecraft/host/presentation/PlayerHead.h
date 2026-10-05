#pragma once

#include "minecraft/host/presentation/PlayerPortrait.h"

namespace nova::overlay {

inline void DrawPlayerHead(ImDrawList* draws, ImTextureID texture, ImVec2 min, ImVec2 max,
                           ImU32 tint, float rounding) {
    ui::DrawPortrait(draws, minecraft::PlayerPortrait(texture), min, max, tint, rounding);
}

}
