#include "minecraft/java/api/game_keepsprint.h"
#include "minecraft/java/api/game_critical.h"
#include "minecraft/compat/minecraft263/Minecraft263Access.h"
#include "minecraft/events/GameEvents.h"
#include <exception>

namespace nova::java {
namespace {
using compat::mc263::access::Frame;
using compat::mc263::access::Member;
struct Mapping {
    Member ground, hurt, velocity, multiply, setVelocity, sprint;
};
#define NOVA_KEEP_SPRINT_MAPPING
constexpr Mapping lunar{
#include "minecraft/compat/mappings/Lunar189.inc"
};
constexpr Mapping forge{
#include "minecraft/compat/mappings/forge1201.inc"
};
constexpr Mapping modern{
#include "minecraft/compat/mappings/minecraft263.inc"
};
#undef NOVA_KEEP_SPRINT_MAPPING
const Mapping& Names() { return compat::IsLegacyLunar() ? lunar : compat::UsesModernRuntime() ? modern : forge; }
struct Scratch { double slowdown = 0.6; bool sprint = false; bool pending = false; };
Scratch hit;
bool Local(Frame& f, jobject player) {
    auto self = f.player();
    return self && player && f.env->IsSameObject(self, player);
}
}

double BeginHitSlowdown(JNIEnv* env, jobject player) {
    Frame f(env);
    if (!Local(f, player)) return 0.6;
    hit = {};
    const auto& n = Names();
    const bool ground = compat::IsLegacyLunar() ? f.get<bool>(player,n.ground) : f.call<bool>(player,n.ground);
    const int hurt = f.get<int>(player,n.hurt);
    if (!f.check()) return 0.6;
    event::AttackSlowdownEvent e(ground,hurt);
    try { (void)event::EventBus::Get().Post(e); }
    catch (const std::exception& ex) { LogTo("KeepSprint: event exception: %s",ex.what()); return 0.6; }
    catch (...) { LogTo("KeepSprint: event exception"); return 0.6; }
    hit = {e.slowdown,e.sprint,true};
    return hit.slowdown;
}
double HitSlowdown() { return hit.pending ? hit.slowdown : 0.6; }
bool EndHitSprint(bool original) {
    const bool sprint = hit.pending ? hit.sprint : original;
    hit.pending = false;
    return sprint;
}

void JNICALL AttackVelocityHook(JNIEnv* env, jclass, jobject player, jobject velocity) {
    Frame f(env);
    const auto& n = Names();
    if (Local(f,player)) {
        const double slowdown = BeginHitSlowdown(env,player);
        if (slowdown != 0.6) {
            // Read the pre-hit vector, so zero slowdown and very small values
            // remain exact; never divide an already rounded vanilla vector.
            auto original = f.call(player,n.velocity);
            auto modified = f.call(original,n.multiply,slowdown,1.0,slowdown);
            if (modified && f.check()) velocity = modified;
        }
    }
    // A failed optional read must still execute the vanilla write.
    Frame write(env);
    write.invoke(player,n.setVelocity,velocity);
}
void JNICALL AttackSprintHook(JNIEnv* env, jclass, jobject player, jboolean sprint) {
    Frame f(env);
    if (Local(f,player)) {
        const bool requested = EndHitSprint(sprint != JNI_FALSE);
        sprint = requested && !CriticalSprintInterrupted() ? JNI_TRUE : JNI_FALSE;
    }
    f.invoke(player,Names().sprint,sprint);
}
}
