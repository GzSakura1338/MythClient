#pragma once
#include "minecraft/modules/combat/antikb/prediction/PredictionVelocity.h"

namespace nova::module::prediction {
void Initialize();
void Configure(bool enabled, const PredictionSettings& settings);
void Shutdown();
bool OwnsIncoming();
bool StoppedBlock();
bool CancelsAura();
bool ConsumeExtraAttack(int& mode);
void ClearExtraAttack();
}
