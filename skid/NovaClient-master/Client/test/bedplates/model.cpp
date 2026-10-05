#include "minecraft/java/game/BedScanRules.h"
#include <cassert>
#include <limits>
#include <cstdio>
int main() {
    using namespace nova::java;
    for(unsigned facing=0;facing<4;++facing) {
        assert(!beds::Head189((26<<4)|facing));
        assert(beds::Head189((26<<4)|8|facing));
        assert(!beds::Head189((54<<4)|8|facing));
    }
    auto pos=beds::Cell189(-2,3,4,0x5ab);
    assert(pos.x==-21 && pos.y==69 && pos.z==58);
    BlockPos head{10,64,20},foot;
    assert(beds::Foot("minecraft:bed[facing=north,part=head]",head,foot) && foot.z==21);
    assert(beds::Foot("Block{minecraft:red_bed}[facing=south,part=head]",head,foot) && foot.z==19);
    assert(beds::Foot("bed[facing=east,part=head]",head,foot) && foot.x==9);
    assert(beds::Foot("bed[facing=west,part=head]",head,foot) && foot.x==11);
    assert(!beds::Foot("bed[facing=west,part=foot]",head,foot));
    assert(!beds::Foot("bed[part=head]",head,foot));
    BedPlate result;
    assert(!beds::Defense(-1,true,false,result));
    assert(!beds::Defense(0,false,false,result));
    assert(!beds::Defense(1,true,true,result));
    assert(!beds::Defense(std::numeric_limits<float>::quiet_NaN(),true,false,result));
    assert(result.covered==0);
    assert(beds::Defense(50,true,false,result)); // Obsidian initially selected.
    assert(beds::Defense(.8f,true,false,result)); // Wool becomes the weak point.
    assert(!beds::Defense(3,true,false,result)); // End stone does not replace it.
    assert(result.covered==3 && result.hardness==.8f);
    assert(beds::Defense(0,true,false,result)); // Zero-hardness solid blocks remain valid.
    puts("PASS: bed head deduplication, section decode, negative chunks, four orientations, invalid states and defense selection");
}
