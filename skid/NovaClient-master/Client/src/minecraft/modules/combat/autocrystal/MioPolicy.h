#pragma once
#include <algorithm>
#include <cmath>

namespace nova::module::autocrystal {
// AutoCrystalDataMode ordinal and AutoCrystalData.getAutoCrystalData11.
enum class PlacementStage { Direct, Occluded, Obsidian };
inline bool Better(PlacementStage next,double damage,PlacementStage old,double previous,bool retained=false) {
    if(!std::isfinite(damage)||damage<0)return false;
    if(next!=old)return next<old;
    // Java Math.round: a retained cell is replaced only above the rounded score.
    return retained?damage>std::floor(previous+.5):damage>=previous;
}
// AutoCrystalMode_3.get847: minimum cannot exceed remaining target health.
inline double Minimum(double configured,double health,double absorption) {
    return std::min(configured,std::max(0.,health+absorption));
}
inline constexpr double PredictionTicks=2;
inline constexpr double TargetRange=12;
}
