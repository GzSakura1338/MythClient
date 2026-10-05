#pragma once
#include "minecraft/java/api/game_entity.h"

namespace nova::java {
struct MaceState {
    double fall = 0, verticalSpeed = 0;
    float strength = 0;
    bool ground = false, gliding = false, obstructed = false, interacting = false;
};
bool MaceSupported();
bool ReadMaceState(MaceState& out);
bool MaceSlotAvailable(int slot);
bool MaceAxeSlotAvailable(int slot);
bool MaceTargetBlockingShield(const EntityHandle& target);
EntityHandle MaceSlotIdentity(int slot);
bool MaceSlotMatches(int slot, const EntityHandle& identity);
// Range is calculated for the actual weapon (mace or axe), including item components.
bool MaceReach(int slot, double& out);
bool MaceCanReach(int slot, const EntityHandle& target);
}
