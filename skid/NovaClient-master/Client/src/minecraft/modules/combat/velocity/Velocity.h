#pragma once

namespace nova::module::velocity {

// While a knockback is being held Velocity keeps its own attacks coming and aims
// at whoever sent it. KillAura defers to that: it stops feeding its own attack
// queue, and it re-sorts its targets around the incoming motion.
bool IsDelaying();

// Attacks Velocity still owes the game, drained one per player tick.
int AttackQueue();

// The yaw the held knockback is coming from, wrapped.
float Yaw();

}
