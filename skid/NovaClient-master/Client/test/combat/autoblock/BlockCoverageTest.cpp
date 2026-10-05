#include "minecraft/modules/combat/autoblock/BlockCoverage.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

using nova::module::autoblock::BlockCoverage;

void Expect(float actual, float expected, const char* scenario) {
    if (std::abs(actual - expected) <= 0.01f) return;
    std::fprintf(stderr, "%s: expected %.2f, got %.2f\n", scenario, expected, actual);
    std::exit(1);
}

int main() {
    BlockCoverage coverage;
    Expect(coverage.Percent(1000), 0, "no samples");
    coverage.Start(1000, false);
    Expect(coverage.Percent(1000), 0, "zero duration");
    coverage.Record(1100, true);
    Expect(coverage.Percent(1200), 50, "first block");
    coverage.Record(1500, false);
    coverage.Record(1500, true);
    Expect(coverage.Percent(2000), 90, "release and reblock in one replay batch");
    coverage.Record(2000, false);
    Expect(coverage.Percent(6000), 18, "whole window");
    Expect(coverage.Percent(6500), 10, "partial expired interval");
    Expect(coverage.Percent(7000), 0, "all blocked intervals expired");
    coverage.Record(7100, true);
    Expect(coverage.Percent(12100), 100, "continuous block beyond window");
    coverage.Start(13000, false);
    Expect(coverage.Percent(13100), 0, "new session clears previous fight");
    coverage.Record(13100, true);
    coverage.Record(13200, true);
    Expect(coverage.Percent(13500), 80, "duplicate block packet");
    coverage.Start(15000, true);
    coverage.Record(15000, false);
    Expect(coverage.Percent(15100), 0, "same timestamp replaces state");
    std::puts("Block coverage: 11 scenarios passed");
}
