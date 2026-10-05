#pragma once
namespace nova::compat {
struct BedMapping {
    const char *posClass, *bedClass, *posSig, *stateSig, *blockSig, *chunkSig;
    const char *getState, *getBlock, *getChunk, *chunkMap, *getX, *getY, *getZ;
    const char *hardness, *hardnessDesc, *solid;
    const char *sections = "", *sectionsDesc = "", *sectionData = "", *material = "", *materialDesc = "";
};
}
