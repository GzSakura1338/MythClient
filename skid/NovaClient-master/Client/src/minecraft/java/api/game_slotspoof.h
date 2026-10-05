#pragma once
#include "minecraft/java/api/base.h"

namespace nova::java {
// Physical hotbar selection is never overwritten. One module request owns
// the effective interaction slot; render callbacks explicitly read physical.
bool SetSilentSlot(const void* owner, int slot);
void ReleaseSilentSlot(const void* owner);
bool SetScaffoldSlot(int slot);
void ReleaseScaffoldSlot();
bool EffectiveSelectedSlot(int& out);
// Real equip uses the same lease as silent slots. Manual vanilla scrolling is
// untouched; native modules must acquire/retain this lease before switching.
bool AcquirePhysicalSlot(const void* owner);
bool OwnsPhysicalSlot(const void* owner);
bool OwnsSlot(const void* owner);
// Keep the selected/effective slot, but let the next module acquire the lease.
void YieldSlot(const void* owner);
bool SetOwnedPhysicalSlot(const void* owner, int slot);
bool CanChangePhysicalSlot();
jint JNICALL SpoofSlotHook(JNIEnv*, jclass, jobject inventory);
jobject JNICALL VisualInventoryHandHook(JNIEnv*, jclass, jobject inventory);
jobject JNICALL VisualPlayerHandHook(JNIEnv*, jclass, jobject player);
}
