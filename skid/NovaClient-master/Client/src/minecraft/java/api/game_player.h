
#pragma once

namespace nova::java {

bool PlayerOnGround();

// Clear the local player's vanilla jump cooldown before the living update.
bool ResetPlayerJumpDelay();

bool AddNightVision();

bool RemoveNightVision();

bool PlayerUsingItem();

// True only when the modern attack cooldown is fully charged. Missing player
// or JNI bindings fail closed; pre-1.9 clients have no cooldown.
bool PlayerAttackCooldownReady();

bool PlayerInCreative();

bool PlayerCanEdit();

bool HoldingWeapon();

}
