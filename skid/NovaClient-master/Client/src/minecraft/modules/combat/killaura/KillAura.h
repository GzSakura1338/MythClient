#pragma once

#include <vector>
#include "minecraft/java/api/game_entity.h"

namespace nova::module::killaura {

// True while KillAura has at least one candidate. Velocity uses it to decide
// whether it may aim on the client's behalf: if KillAura is already aiming,
// two writers would fight over the same rotation.
bool HasTargets();
bool IsEnabled();
bool CriticalSoundEnabled();
// Independent block-range scan, without selected-target, FOV or enable gates.
bool HasBlockTarget();
bool ReadHypixelGeometry(java::Box& box, rotation::Vec3& position);
bool QueueAutoBlockAttack(bool releaseEarly = false);

// The entity id the aura is currently facing, or 0 when it has none. An id rather
// than a handle, because the reader is a different module that has to re-resolve it
// against the level anyway -- and a handle outlives the entity it names, while an
// id stops resolving the moment the level drops it.
int TargetId();
double AttackRange();
bool ReadHypixelTarget(java::EntityHandle& target, double blockRange, bool& hasBlockTarget, int& normalDelay);

// Every target the aura is holding, in its own order, when it is in Switch mode.
// False when it is not switching -- Single keeps one target at a time and the target
// hud already draws that one -- so a caller can read it as "is there a row to show".
//
// Ids for the same reason TargetId is one: the reader has to re-resolve them against
// the level, and an id stops resolving the moment the level drops the entity.
bool SwitchTargets(std::vector<int>& out);

}
