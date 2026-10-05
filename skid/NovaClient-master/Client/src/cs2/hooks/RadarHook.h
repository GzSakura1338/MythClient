#pragma once

namespace nova::cs2::hooks::radar {
// The host owns MinHook initialization. Only pass a version-verified target.
bool Install(void* verifiedTarget);
bool Available();
void SetEnabled(bool enabled);
bool Stop(unsigned timeoutMs);
void Finalize(); // After Stop succeeds, before the host uninitializes MinHook.
}
