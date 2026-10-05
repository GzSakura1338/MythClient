#pragma once

#include "minecraft/modules/combat/autoblock/AutoBlock.h"

namespace nova::module::autoblock {

// Both UI entry points feed one engine. The standalone module takes precedence.
void ConfigureAura(bool enabled, const AutoBlockSettings& settings);
bool Active();
void SuspendForInteraction();
// Game-thread only; includes pending attacks and pre-block target reservation.
bool OwnsItemUse();
bool HypixelActive();
// These modes own the attack cadence; KillAura still supplies targets and rotations.
bool AttackCycleActive();
double HypixelRange();
bool VelocityCanReduce(int phase);
bool VelocityShouldBlock();
void SubmitAttacks(int count);
bool QueueAttack(bool releaseEarly = false);

}
