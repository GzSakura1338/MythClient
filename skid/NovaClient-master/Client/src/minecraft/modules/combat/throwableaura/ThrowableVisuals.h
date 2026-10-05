#pragma once
#include <chrono>
#include <mutex>
#include "minecraft/java/api/game_entity.h"
#include "minecraft/modules/combat/throwableaura/Trajectory.h"

namespace nova::module::throwableaura {
class Visuals {
public:
    void Clear();
    void Update(const java::EntityHandle& self, const java::EntityHandle& target,
                float yaw, float pitch, bool trajectory);
    void Render(bool esp, bool trajectory);
private:
    std::mutex mutex_;
    java::EntityHandle world_;
    int targetId_ = -1;
    Path path_;
    std::chrono::steady_clock::time_point updated_{};
};
}
