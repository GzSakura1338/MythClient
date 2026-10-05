#include "minecraft/java/api/game_critical.h"
#include "minecraft/java/api/game_entity.h"
#include "minecraft/java/api/game_click.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/modules/combat/killaura/KillAura.h"
#include <cmath>
#include <atomic>

namespace nova::java {
namespace {
using compat::mc263::access::Frame;
using compat::mc263::access::Member;
struct Mapping {
    Member fall, ground, water, climbing, passenger, sprinting, blind, hasEffect;
    Member world, pos, state, block, sound, play;
};
#define NOVA_CRITICAL_MAPPING
#define NOVA_CRITICAL(id, owner, name, desc) .id = {owner, name, desc},
constexpr Mapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr Mapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_CRITICAL
#undef NOVA_CRITICAL_MAPPING
const Mapping& Bindings() { return compat::UsesModernRuntime() ? modern : forge; }
// Only the synchronous attacking thread can hold this override; no static TLS
// or game object survives the attack or leaks into a new world.
std::atomic<DWORD> interruptedThread{0};

struct CriticalAttackScope {
    DWORD previous = interruptedThread.exchange(GetCurrentThreadId(), std::memory_order_acq_rel);
    ~CriticalAttackScope() { interruptedThread.store(previous, std::memory_order_release); }
};

bool InCobweb(Frame& f, const Mapping& m, bool& web) {
    web = false;
    Box b;
    if (!PlayerEntity().BoundingBox(b)) return false;
    const double edges[]{b.minX,b.minY,b.minZ,b.maxX,b.maxY,b.maxZ};
    for (double edge : edges) if (!std::isfinite(edge) || std::abs(edge)>30000001) return false;
    if (b.maxX<=b.minX || b.maxY<=b.minY || b.maxZ<=b.minZ ||
        b.maxX-b.minX>4 || b.maxY-b.minY>4 || b.maxZ-b.minZ>4) return false;
    auto world = f.get(f.client(), m.world);
    if (!world) return false;
    // Webs have no solid collision shape. Inspect every cell intersecting the
    // player's body; checking only the block at their feet misses upper webs.
    for (int x=int(std::floor(b.minX+.001)); x<=int(std::floor(b.maxX-.001)); ++x)
        for (int y=int(std::floor(b.minY+.001)); y<=int(std::floor(b.maxY-.001)); ++y)
            for (int z=int(std::floor(b.minZ+.001)); z<=int(std::floor(b.maxZ-.001)); ++z) {
                auto pos = f.create(m.pos,x,y,z);
                auto state = f.call(world,m.state,pos);
                auto block = f.call(state,m.block);
                const bool hit = f.instance(block,"net.minecraft.world.level.block.WebBlock");
                if (block) f.env->DeleteLocalRef(block);
                if (state) f.env->DeleteLocalRef(state);
                if (pos) f.env->DeleteLocalRef(pos);
                if (!f || !block) return false;
                if (hit) { web=true; return true; }
            }
    return f.check();
}
}

bool ReadCriticalAttackState(CriticalAttackState& state) {
    if (compat::IsLegacyLunar()) return false;
    Frame f;
    if (!f) return false;
    auto player = f.player();
    if (!player) return false;
    const auto& m = Bindings();
    state = {};
    state.ground = f.call<bool>(player,m.ground);
    if (!f) return false;
    if (state.ground) return true;
    state.water = f.call<bool>(player,m.water);
    if (!f) return false;
    if (state.water) return true;
    if (!InCobweb(f,m,state.web)) return false;
    if (state.web) return true;
    state.fallDistance = compat::UsesModernRuntime() ? f.get<double>(player,m.fall) : f.get<float>(player,m.fall);
    state.climbing = f.call<bool>(player,m.climbing);
    state.passenger = f.call<bool>(player,m.passenger);
    state.sprinting = f.call<bool>(player,m.sprinting);
    auto blindness = f.stat(m.blind);
    if (!blindness) return false;
    state.blind = f.call<bool>(player,m.hasEffect,blindness);
    return f.check() && std::isfinite(state.fallDistance);
}

bool CriticalSprintInterrupted() {
    return interruptedThread.load(std::memory_order_acquire) == GetCurrentThreadId();
}

bool AttackWithCriticalWait(const EntityHandle& target) {
    CriticalAttackState state;
    if (!ReadCriticalAttackState(state)) return false;
    if (!state.AllowsAttack()) return false;
    if (state.ground || state.water || state.web) return AttackAndSwing(target);
    // Preserve KeepSprint's velocity settings, but do not let its hit callback
    // force sprint back on. The next ordinary input tick owns sprint recovery.
    CriticalAttackScope scope;
    return AttackAndSwing(target);
}

void JNICALL CriticalHitHook(JNIEnv* env, jclass, jobject player) {
    if (IsCircuitOpen() || !module::killaura::CriticalSoundEnabled()) return;
    Frame f(env);
    if (!f) return;
    auto local = f.player();
    if (!local || !player || !env->IsSameObject(local,player)) return;
    const auto& m = Bindings();
    auto sound = f.stat(m.sound);
    if (sound) f.invoke(player,m.play,sound,1.0f,0.9f);
}
}
