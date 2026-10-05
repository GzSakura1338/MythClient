#pragma once
#include "minecraft/java/api/base.h"

namespace nova::java {
struct CriticalAttackState {
    double fallDistance = 0;
    bool water = false, web = false, ground = true;
    bool climbing = false, blind = false, passenger = false, sprinting = false;

    bool AllowsAttack() const {
        return ground || water || web || (fallDistance > 0 && !climbing &&
            !blind && !passenger && !sprinting);
    }
};

class EntityHandle;
bool ReadCriticalAttackState(CriticalAttackState& state);
// Ground/water/web use normal attacks; airborne attacks wait for a critical.
// No sprint packets are emitted here; vanilla movement owns sprint transitions.
bool AttackWithCriticalWait(const EntityHandle& target);
bool CriticalSprintInterrupted();
// Called by the local player's vanilla critical-hit callback, not by a swing.
void JNICALL CriticalHitHook(JNIEnv* env, jclass, jobject player);
}
