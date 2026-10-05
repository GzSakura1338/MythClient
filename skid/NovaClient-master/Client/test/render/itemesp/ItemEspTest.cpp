#include "minecraft/modules/render/itemesp/ItemEspStyle.h"
#include <cassert>
#include <map>
#include <iostream>
int main() {
    using namespace nova::module::itemesp;
    // Golden cases from Raven getColor/getColorForItem, including zero axes
    // and Java's negative half rounding (std::round would differ).
    assert(GroupKey(0,0,0,0) == 6.155);
    assert(GroupKey(-1.5,1,1,0) == -0.845);
    assert(GroupKey(2,64,3,2) == 960.355);
    std::map<double,int> stacks;
    stacks[GroupKey(2,64,3,0)] += 16;
    stacks[GroupKey(2,64,3,0)] += 32;
    stacks[GroupKey(2,64,3,1)] += 4;
    assert(stacks.size()==2 && stacks.at(960.155)==48 && stacks.at(960.255)==4);
    assert(Radius(0)==.2 && Radius(30)==.3 && Radius(256)==.4);
    assert(TextScale(0)==.02266667 && TextScale(256)==.07);
    assert((std::uint32_t(boxColors[0]) & 0xffffff)==0xffffff);
    std::cout << "PASS: Raven grouping, separate resource totals, Java rounding and distance scaling\n";
}
