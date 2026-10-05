#pragma once
#include "minecraft/utils/rotation/Vec3.h"
#include <algorithm>
#include <cmath>
namespace nova::module::freecam {
// Raven updates at tick END. Independent key branches add, including diagonals.
inline rotation::Vec3 Step(rotation::Vec3 pos,float yaw,float forward,float strafe,bool up,bool down,double speed) {
    if(!std::isfinite(yaw+forward+strafe+speed))return pos;
    const double y=yaw*3.141592653589793/180.,vertical=int(up)-int(down);
    const double distance=.215*std::clamp(speed,.5,10.);
    return pos.Add((-std::sin(y)*forward+std::cos(y)*strafe)*distance,.93*vertical*distance,
                   (std::cos(y)*forward+std::sin(y)*strafe)*distance);
}
}
