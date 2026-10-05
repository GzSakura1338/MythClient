#pragma once
#include "cs2/modules/combat/recoilcontrol/RecoilController.h"
#include "cs2/modules/combat/aimbot/AimController.h"
#include "cs2/modules/combat/autofire/AutoFireController.h"

namespace nova::cs2::hooks::input {
// MinHook is owned by Dx11Hooks. Aim uses a verified commit/caller.
bool Install(sdk::weapons::Binding, sdk::aim::Capture aimCapture = nullptr, sdk::aim::ShotCapture shotCapture = nullptr);
bool Available();
bool AimAvailable();
bool FireAvailable();
// Empty while available; otherwise the installation stage or latched runtime fault.
const char* FireUnavailableReason();
struct FireDebug {
    const char* status = "idle";
    const char* lastDeferred = "none";
    unsigned frames = 0, samples = 0, targets = 0, writes = 0;
    unsigned deferred = 0, exceptionCode = 0;
    std::uintptr_t exceptionAddress = 0;
    double time = 0, nextAttack = 0;
};
FireDebug ReadFireDebug();
void PublishFire(autofire::Settings, bool enabled, bool ignoreTeam, bool hold, int key);
// Coherent, short-lived display correction for the input we actually committed.
bool ReadVisualOffset(std::uintptr_t pawn, std::uint32_t handle, sdk::weapons::Angle raw, sdk::weapons::Angle& offset);
void Publish(recoil::Settings, bool enabled);
void PublishAim(aim::Settings, bool enabled, bool ignoreTeam, bool hold, int key, bool onlyClick = false);
bool Stop(unsigned timeoutMs);
void Finalize();
}
