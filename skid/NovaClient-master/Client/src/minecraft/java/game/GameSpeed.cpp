#include "minecraft/java/api/game_speed.h"
#include "minecraft/java/api/game_mlg.h"
#include "minecraft/java/api/game_player.h"
#include "minecraft/compat/mappings/BridgeAssistMapping.h"

namespace nova::java {
namespace {
using compat::mc263::access::Frame;
using compat::mc263::access::Member;
#define NOVA_BRIDGEASSIST_MAPPING
#define NOVA_BRIDGE(id, owner, name, desc) .id = {owner, name, desc},
constexpr compat::BridgeAssistMapping legacy{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr compat::BridgeAssistMapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr compat::BridgeAssistMapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_BRIDGE
#undef NOVA_BRIDGEASSIST_MAPPING
}

bool ReadSpeedEnvironment(SpeedEnvironment& out) {
    out = {};
    auto player = PlayerEntity();
    if (!player || !LevelEntity() || !player.IsAlive() || !player.OnGround(out.ground)) return false;
    if (player.IsSpectator() || PlayerUsingItem()) return true;
    MlgEnvironment environment;
    if (!ReadMlgEnvironment(environment)) return false;
    if (environment.water || environment.lava || environment.flying || environment.passenger) return true;
    Frame f;
    auto object = static_cast<jobject>(player.raw());
    const auto& m = compat::IsLegacyLunar() ? legacy : compat::UsesModernRuntime() ? modern : forge;
    const bool climbing = f.call<bool>(object, m.ladder);
    const bool sleeping = f.call<bool>(object, m.sleeping);
    bool gliding = false;
    if (!compat::IsLegacyLunar()) {
        const Member flying = compat::UsesModernRuntime() ? compat::mc263::names::LivingEntity_isFallFlying :
            Member{"net.minecraft.world.entity.LivingEntity", "m_21255_", "()Z"};
        gliding = f.call<bool>(object, flying);
    }
    if (!f.check()) return false;
    out.blocked = climbing || sleeping || gliding;
    return true;
}
}
