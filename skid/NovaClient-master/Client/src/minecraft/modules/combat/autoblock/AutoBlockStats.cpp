#include "minecraft/modules/combat/autoblock/AutoBlockStats.h"
#include "minecraft/modules/combat/autoblock/BlockCoverage.h"
#include "ui/overlay/OverlaySpin.h"
#include <windows.h>

namespace nova::module::autoblock {
namespace {
ui::SpinLock statsLock;
BlockCoverage coverage;
bool active = false;
bool blocking = false;
float lastPercent = 0;
ULONGLONG lastSeen = 0;
}

void SetCoverageActive(bool value) {
    ui::ScopedSpin lock(statsLock);
    const auto now = GetTickCount64();
    if (value && (!active || now - lastSeen > 500)) {
        coverage.Start(now, blocking);
        lastPercent = 0;
    } else if (active && !value) {
        lastPercent = coverage.Percent(now);
    }
    active = value;
    if (value) lastSeen = now;
}

void RecordBlockState(bool value) {
    ui::ScopedSpin lock(statsLock);
    blocking = value;
    if (active) coverage.Record(GetTickCount64(), value);
}

void ResetCoverage() {
    ui::ScopedSpin lock(statsLock);
    active = blocking = false;
    lastSeen = 0;
    lastPercent = 0;
    coverage = BlockCoverage{};
}

CoverageStats ReadCoverage() {
    ui::ScopedSpin lock(statsLock);
    const auto now = GetTickCount64();
    if (active && now - lastSeen > 500) {
        lastPercent = coverage.Percent(now);
        active = false;
    }
    if (active) lastPercent = coverage.Percent(now);
    return {active, lastPercent};
}

}
