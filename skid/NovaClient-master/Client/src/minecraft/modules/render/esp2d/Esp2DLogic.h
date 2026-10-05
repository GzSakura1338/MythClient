#pragma once
#include <limits>
#include <optional>
#include "minecraft/utils/rotation/Vec3.h"
#include "ui/hud/esp2d/Esp2DView.h"

namespace nova::module::esp2d {
using rotation::Vec3;
using ui::esp2d::Bounds;
using ui::esp2d::BoxMode;
using ui::esp2d::HealthMode;
using ui::esp2d::Render;
using ui::esp2d::Value;
struct Targets { bool players=true, invisibles=false, animals=false, mobs=false, teammates=true; };
inline bool Accept(const Targets& settings, bool player, bool animal, bool mob, bool invisible, bool teammate) {
    const bool selected=(player && settings.players) || (animal && settings.animals) || (mob && settings.mobs);
    return selected &&
        (!invisible || settings.invisibles) && (!player || !teammate || settings.teammates);
}

// Rise's interpolated, slightly expanded entity box. Project returns screen X/Y.
template<class Project> std::optional<Bounds> ProjectBounds(Vec3 now, Vec3 before, double width,
        double height, bool sneaking, float partial, double screenWidth, double screenHeight, Project&& project) {
    if (!std::isfinite(width) || !std::isfinite(height) || width<=0 || height<=0 || !std::isfinite(partial)) return {};
    const double t=std::clamp(double(partial),0.0,1.0);
    Vec3 at{before.x+(now.x-before.x)*t,before.y+(now.y-before.y)*t,before.z+(now.z-before.z)*t};
    if (!std::isfinite(at.x)||!std::isfinite(at.y)||!std::isfinite(at.z)) return {};
    const double half=(width+.14)/2, top=height+(sneaking?-.1:.2)+.01;
    Bounds bounds{std::numeric_limits<double>::infinity(),std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()};
    int count=0;
    for (int i=0;i<8;++i) {
        double x=0,y=0;
        if (!project(Vec3{at.x+((i&1)?half:-half),at.y+((i&2)?top:0),at.z+((i&4)?half:-half)},x,y) ||
            !std::isfinite(x)||!std::isfinite(y)) continue;
        bounds.left=std::min(bounds.left,x);bounds.right=std::max(bounds.right,x);
        bounds.top=std::min(bounds.top,y);bounds.bottom=std::max(bounds.bottom,y);++count;
    }
    if (count<2 || bounds.right<=bounds.left || bounds.bottom<=bounds.top ||
        bounds.right<0 || bounds.left>screenWidth || bounds.bottom<0 || bounds.top>screenHeight) return {};
    return bounds;
}
}
