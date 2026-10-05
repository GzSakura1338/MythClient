#include "minecraft/modules/combat/autoblock/AutoBlockPacketSync.h"
#include "minecraft/modules/combat/antikb/AntiKB.h"
#include "minecraft/compat/lunar189/LunarRuntime.h"
#include <utility>

namespace nova::module {
namespace autoblock {
namespace {
java::EntityHandle delayPlayer, delayWorld;
}

void SynchronizePacketDelays(bool discard) {
    const bool lagging = compat::lunar::SlinkyBlockLagging() || compat::lunar::HypixelBlockLagging();
    if (lagging) {
        auto player = java::PlayerEntity(), world = java::LevelEntity();
        const bool changed = !player || !world ||
            (delayPlayer && !delayPlayer.IsSame(player)) ||
            (delayWorld && !delayWorld.IsSame(world));
        if (discard || changed || antikb::BlocksAutoBlockBlink()) {
            compat::lunar::EndSlinkyBlockLag(!discard && !changed);
            compat::lunar::EndHypixelBlockLag(!discard && !changed);
        } else {
            delayPlayer = std::move(player);
            delayWorld = std::move(world);
            return;
        }
    }
    delayPlayer.Reset();
    delayWorld.Reset();
}
}

}
