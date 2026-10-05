#pragma once
namespace nova::module::invmove {
// Shared arbitration with Sprint; InventoryMove owns this only during clicks/chests.
bool SuppressSprint();
bool PrepareManagerClick();
void ManagerClicked();
bool BlockAcceleration();
bool ZeroHorizontalDisplacement();
void ReleaseManager();
}
