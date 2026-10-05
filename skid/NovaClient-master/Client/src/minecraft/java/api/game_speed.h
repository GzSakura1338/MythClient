#pragma once

namespace nova::java {
struct SpeedEnvironment {
    bool ground = false;
    bool blocked = true;
};
// Read-only eligibility for walking/jumping; unknown mappings fail closed.
bool ReadSpeedEnvironment(SpeedEnvironment& out);
}
