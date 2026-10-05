#pragma once
#include "minecraft/java/api/game_beds.h"
#include <cmath>
#include <string_view>

namespace nova::java::beds {
inline bool Head189(unsigned state) { return (state >> 4) == 26 && (state & 8) != 0; }
inline BlockPos Cell189(int chunkX, int chunkZ, int section, int index) {
    return {chunkX*16+(index&15),section*16+(index>>8),chunkZ*16+((index>>4)&15)};
}
inline bool Foot(std::string_view state, BlockPos head, BlockPos& foot) {
    if (state.find("part=head") == std::string_view::npos) return false;
    foot=head;
    if(state.find("facing=north")!=std::string_view::npos) ++foot.z;
    else if(state.find("facing=south")!=std::string_view::npos) --foot.z;
    else if(state.find("facing=east")!=std::string_view::npos) --foot.x;
    else if(state.find("facing=west")!=std::string_view::npos) ++foot.x;
    else return false;
    return true;
}
// Count all breakable solid neighbors, choosing the easiest one as Rise does.
inline bool Defense(float hardness, bool solid, bool bed, BedPlate& result) {
    if(!solid || bed || !std::isfinite(hardness) || hardness<0) return false;
    const bool best=result.covered==0 || hardness<result.hardness;
    ++result.covered;
    if(best) result.hardness=hardness;
    return best;
}
}
