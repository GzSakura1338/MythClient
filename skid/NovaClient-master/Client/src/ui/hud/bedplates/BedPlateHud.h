#pragma once
#include <string>
#include "imgui.h"

namespace nova::ui {
struct BedPlateSnapshot {
    ImVec2 anchor;
    std::string material;
    double distance = 0;
    int covered = 0;
};
struct BedPlateStyle {
    float size = 1;
    bool minimal = false;
    bool distance = true;
    bool distanceScale = true;
    bool shadow = false;
};
void DrawBedPlate(const BedPlateSnapshot& plate,const BedPlateStyle& style);
}
