#pragma once

namespace nova::java {
enum class KnownItem : int;
class EntityHandle;
// Right-click the selected equipment only if it still matches this snapshot.
bool UseSelectedEquipment(const EntityHandle& expected);
// Main game thread only. Uses the vanilla controller without changing rotation.
bool SyncSelectedItem();
// Rechecks the selected hotbar stack before use; expected must identify an item.
bool UseSelectedItem(KnownItem expected);
// attempted becomes true when entering vanilla use, even if its result is PASS
// or JNI fails afterwards. False means retrying cannot duplicate this use.
bool UseSelectedItem(KnownItem expected, bool& attempted);
// Effective silent slot, validated before native use; angle comes from manager.
bool UseSilentItem(KnownItem expected, float yaw, float pitch);
// Exclusive use also suppresses later native attacks through this tick. The
// caller releases its attack lease at the following game tick or on reset.
bool UseSilentItem(KnownItem expected, float yaw, float pitch, const void* owner, bool& attempted);
}
