#pragma once
#include "minecraft/modules/combat/antikb/mix/MixVelocity.h"

namespace nova::module::mix {
void Initialize();
void Configure(bool enabled, const Settings& settings);
void Shutdown();
bool OwnsIncoming();
}
