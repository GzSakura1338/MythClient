#pragma once
#include "cs2/sdk/weapons/Weapons.h"

namespace nova::cs2::sdk::weapons {
// Native evaluator maintains game-owned caches. Never invoke from Present.
bool EvaluatePunch(std::uintptr_t client, std::uintptr_t service, Time&, Angle&);
}
