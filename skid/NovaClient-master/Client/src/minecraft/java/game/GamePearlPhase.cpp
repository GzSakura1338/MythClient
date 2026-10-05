#include "minecraft/java/api/game_pearlphase.h"
#include "minecraft/java/api/game_place.h"
#include "minecraft/java/api/game_rotation.h"
#include "minecraft/compat/mappings/BridgeAssistMapping.h"
#include "minecraft/compat/lunar1710/Access.h"
#include <cmath>

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

bool ReadPearlPhaseState(PearlPhaseState& out) {
    out = {};
    Frame f;
    auto player = PlayerEntity();
    Box bounds;
    rotation::Vec3 motion;
    float pitch = 0;
    if (!f || !player || !player.Position(out.position) || !player.BoundingBox(bounds) ||
        !player.OnGround(out.ground) || !PlayerRotation(out.yaw,pitch) || !PlayerDeltaMovement(motion)) return false;
    for (double value : {out.position.x,out.position.y,out.position.z})
        if (!std::isfinite(value) || std::abs(value) > 30000001) return false;
    const auto& m = compat::IsLegacyLunar() ? legacy : compat::UsesModernRuntime() ? modern : forge;
    auto object = static_cast<jobject>(player.raw());
    auto input = f.get(object,m.input);
    if (!input) return false;
    out.horizontalCollision = f.get<bool>(object,m.horizontal);
    if (compat::UsesModernRuntime()) {
        auto vector = f.call(input,m.moveVector);
        if (!vector) return false;
        out.forward = f.get<float>(vector,compat::mc263::names::Vec2_Field_y);
        out.strafe = f.get<float>(vector,compat::mc263::names::Vec2_Field_x);
    } else {
        out.forward = f.get<float>(input,m.forward);
        const Member side = compat::IsLegacyLunar()
            ? Member{"net.minecraft.util.MovementInput","moveStrafe","F"}
            : Member{"net.minecraft.client.player.Input","f_108566_","F"};
        out.strafe = f.get<float>(input,side);
    }
    // Mio's environment helper rounds Y, whereas its aiming cell floors Y.
    const BlockPos feet{int(std::floor(out.position.x)),int(std::floor(out.position.y+.5)),int(std::floor(out.position.z))};
    auto world = f.get(f.client(),m.world);
    if (!world) return false;
    auto block = compat::IsLunar1710() ? compat::lunar1710::Block(f,world,feet.x,feet.y,feet.z) :
        f.call(f.call(world,m.state,f.create(m.posCtor,feet.x,feet.y,feet.z)),m.block);
    if (!block || !f.check()) return false;
    out.ladder = f.instance(block,compat::IsLegacyLunar() ? "net.minecraft.block.BlockLadder" : "net.minecraft.world.level.block.LadderBlock");
    out.blocked = !out.ladder && !BlockStateCollisionEmpty(feet) &&
        BlockStateCollisionEmpty({feet.x,feet.y+1,feet.z});
    // Older versions have no swimming pose. Modern compact swimming/crawling
    // poses use a 0.6-high box; do not throw during their upward transition.
    out.risingLowPose = !compat::IsLegacyLunar() && bounds.maxY-bounds.minY < .7 &&
        !out.ground && motion.y > 0;
    return f.check() && std::isfinite(out.yaw) && std::isfinite(out.forward) && std::isfinite(out.strafe);
}
}
