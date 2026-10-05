#include "cs2/sdk/weapons/AimPunch.h"
#include "cs2/sdk/weapons/Offsets.h"
#include <windows.h>
#include <cmath>

namespace nova::cs2::sdk::weapons {
bool EvaluatePunch(std::uintptr_t client, std::uintptr_t service, Time& time, Angle& angle) {
    using GetTime = Time* (__fastcall*)(void*, Time*);
    using Evaluate = Angle* (__fastcall*)(void*, Angle*, const Time*, bool);
    __try {
        reinterpret_cast<GetTime>(client + offsets::punchTime)(reinterpret_cast<void*>(service), &time);
        if (time.tick < 0 || !std::isfinite(time.fraction) || time.fraction < 0 || time.fraction >= 1) return false;
        // R9B=true: native result is already scaled. No extra *2 here.
        reinterpret_cast<Evaluate>(client + offsets::effectivePunch)(reinterpret_cast<void*>(service), &angle, &time, true);
        return std::isfinite(angle.pitch) && std::isfinite(angle.yaw) && std::isfinite(angle.roll) &&
            std::abs(angle.pitch) <= 180 && std::abs(angle.yaw) <= 180 && std::abs(angle.roll) <= 180;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
}
