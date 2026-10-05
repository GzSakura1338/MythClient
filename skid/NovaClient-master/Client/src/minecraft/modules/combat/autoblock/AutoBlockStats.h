#pragma once

namespace nova::module::autoblock {

struct CoverageStats {
    bool active = false;
    float percent = 0;
};

void SetCoverageActive(bool active);
void RecordBlockState(bool blocking);
void ResetCoverage();
CoverageStats ReadCoverage();

}
