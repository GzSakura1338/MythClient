#pragma once
#include "minecraft/java/api/game_entity.h"
#include <chrono>
#include <memory>
#include <mutex>

namespace nova::module::crystal {
// Game thread publishes a snapshot; render thread never reads the aura's state.
class PlacementEsp {
public:
    struct Snapshot {
        java::BlockPos block;
        std::shared_ptr<java::EntityHandle> world;
        std::chrono::steady_clock::time_point updated;
    };
    void Show(const java::BlockPos& block) {
        auto world=java::LevelEntity();
        if(!world){Clear();return;}
        Snapshot next{block,std::make_shared<java::EntityHandle>(std::move(world)),std::chrono::steady_clock::now()};
        std::lock_guard lock(mutex_);std::swap(snapshot_,next);
    }
    void Clear(){Snapshot old;{std::lock_guard lock(mutex_);std::swap(snapshot_,old);}}
    Snapshot Read(){std::lock_guard lock(mutex_);return snapshot_;}
    void Draw();
private:
    std::mutex mutex_;
    Snapshot snapshot_;
};
}
