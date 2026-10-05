
#pragma once

namespace nova::java {

bool SetSprintKeyDown(bool down);
// Resample the bound sprint key; resume asks vanilla to resume a previous sprint.
bool RestoreSprintKey(bool resume = false);
// Main game thread, no screen open: resample only the four movement bindings.
bool RestoreMovementKeys();
// Game thread: drain vanilla right-click presses while a native action owns use.
void SuppressUseKey();
void RestoreUseKey();
// InventoryMove's UI-thread input sampling. Uses actual rebound keys.
struct InventoryMoveKeys { float forward=0,strafe=0; bool jump=false; int directions=0; };
bool ReadInventoryMoveKeys(InventoryMoveKeys& out);

bool GetGamma(double* out);

bool SetGamma(double value);

}
