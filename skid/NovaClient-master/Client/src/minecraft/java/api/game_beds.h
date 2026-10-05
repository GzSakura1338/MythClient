#pragma once
#include "minecraft/java/api/game_entity.h"
#include <memory>
#include <string>
#include <vector>

namespace nova::java {
struct BedPlate {
    BlockPos head;
    rotation::Vec3 center;
    std::string material;
    float hardness{};
    int covered{};
    bool own{};
};
// Called only on the game thread. Publishes native values, never JNI references.
class BedScanner {
public:
    BedScanner();
    ~BedScanner();
    bool Tick(const rotation::Vec3& player, int range, int refreshTicks, std::vector<BedPlate>& out);
    void Reset();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
