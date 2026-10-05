#include "core/Hosts.h"
#include "minecraft/host/MinecraftHost.h"
#include "cs2/host/Cs2Host.h"
#include "../../../shared/Bootstrap.h"

namespace nova {
const GameHost* SelectGameHost(unsigned profile) {
    switch(profile) {
#define NOVA_PROFILE(symbol,value,key,label,game) \
    case NovaClient_##symbol: { \
        static constexpr GameHost host{#game,label,&game::RunHost}; \
        return &host; \
    }
#include "../../../shared/ClientProfiles.inc"
#undef NOVA_PROFILE
    default:return nullptr;
    }
}
}
