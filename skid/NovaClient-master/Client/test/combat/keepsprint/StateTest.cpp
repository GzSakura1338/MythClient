#include "minecraft/modules/combat/keepsprint/KeepSprint.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
using namespace nova::module::keepsprint;
struct Reference {
    Settings settings;
    bool ground = false;
    int hurt = 0;
    bool resetting = false;
    double combo = 0;
    // Generated mechanically from the existing Rise Java listener, never
    // compiled as Java. Keeps its control flow and comparison operators.
    // @RISE_LISTENER@
};
int main() {
    State state;
    Settings settings;
    assert(state.Hit(settings,false,0).slowdown == 0.6);
    settings.normal = 1; settings.normalSprint = true; settings.buffer = true;
    const bool active[]{true,true,true,true,true,false,false,false,false,false,true,true};
    for (bool expected : active) {
        auto r = state.Hit(settings,false,0);
        assert(r.sprint == expected && r.slowdown == (expected ? 1.0 : 0.6));
    }
    Reference reference;
    state = {};
    std::mt19937 random(0x6a95);
    for (int i=0;i<200000;++i) {
        if (i%17==0) {
            settings = {double(random()%21)/20, double(random()%21)/20,
                double(random()%100+1)/10,int(random()%10+1),
                bool(random()%2),bool(random()%2),bool(random()%2),bool(random()%2)};
        }
        reference.settings = settings;
        reference.ground = random()%2;
        reference.hurt = int(random()%11);
        Result expected, actual;
        // A disabled module receives no listener call, and does not reset state.
        if (random()%5) {
            reference.Listener(expected);
            actual = state.Hit(settings,reference.ground,reference.hurt);
        }
        assert(expected.slowdown == actual.slowdown && expected.sprint == actual.sprint);
        assert(state.combo == reference.combo && state.resetting == reference.resetting);
    }
    std::cout << "PASS: 200000 Rise-listener comparisons, toggles, air gate, hurt branch, buffer transitions\n";
}
