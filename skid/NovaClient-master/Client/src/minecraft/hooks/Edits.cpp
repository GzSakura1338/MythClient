#include "minecraft/hooks/Edits.h"

#include <cstring>
#ifdef NOVA_PROXY_RUNTIME
#include "minecraft/proxy/ProxyHooks.h"
#include "minecraft/compat/Profile.h"
#endif

namespace nova::hook {
namespace {

#define NOVA_LUNAR189_EDITS
#include "minecraft/compat/mappings/Lunar189.inc"
#undef NOVA_LUNAR189_EDITS
bool lunarSelected = false;
bool lunar1710Selected = false;
#define NOVA_LUNAR1710_EDITS
#include "minecraft/compat/mappings/Lunar1710.inc"
#undef NOVA_LUNAR1710_EDITS
bool mc263Selected = false;
bool easecationSelected = false;
bool neoforge12110Selected = false;
#define NOVA_MC263_EDITS
#include "minecraft/compat/mappings/minecraft263.inc"
#undef NOVA_MC263_EDITS
#define NOVA_EASECATION_EDITS
#include "minecraft/compat/mappings/easecation.inc"
#undef NOVA_EASECATION_EDITS
#define NOVA_NEOFORGE12110_EDITS
#include "minecraft/compat/mappings/neoforge12110.inc"
#undef NOVA_NEOFORGE12110_EDITS

#define NOVA_FORGE1201_EDITS
#include "minecraft/compat/mappings/forge1201.inc"
#undef NOVA_FORGE1201_EDITS

const ClassEdits* customEdits=nullptr;
int customEditCount=0;

}

void EmitLoad(std::vector<unsigned char>& out, unsigned char opcode, unsigned char index) {
    unsigned char shortForm = 0;
    switch (opcode) {
        case 0x19: shortForm = 0x2A; break;
        case 0x15: shortForm = 0x1A; break;
        case 0x17: shortForm = 0x22; break;
        case 0x16: shortForm = 0x1E; break;
        case 0x18: shortForm = 0x26; break;
        default: break;
    }
    if (shortForm != 0 && index <= 3) {
        out.push_back(static_cast<unsigned char>(shortForm + index));
        return;
    }
    out.push_back(opcode);
    out.push_back(index);
}

std::vector<unsigned char> AppendCode(std::uint16_t methodRef, const StackLoad* loads,
                                      int loadCount) {
    std::vector<unsigned char> code;
    for (int i = 0; i < loadCount; ++i)
        EmitLoad(code, loads[i].opcode, loads[i].index);
    code.push_back(0xB8);
    code.push_back(static_cast<unsigned char>(methodRef >> 8));
    code.push_back(static_cast<unsigned char>(methodRef & 0xFF));
    return code;
}

const ClassEdits* EditsFor(const char* internalName) {
    if (!internalName) return nullptr;
    int count = 0;
    const ClassEdits* edits = AllEdits(count);
    for (int i = 0; i < count; ++i) {
        if (std::strcmp(edits[i].internalName, internalName) == 0)
            return &edits[i];
    }
    return nullptr;
}

const ClassEdits* BaseEdits(int& classCount) {
    if (customEdits) { classCount=customEditCount;return customEdits; }
    if (neoforge12110Selected) { classCount=static_cast<int>(std::size(neoforge12110Edits));return neoforge12110Edits; }
    if (easecationSelected) { classCount=static_cast<int>(std::size(easecationEdits)); return easecationEdits; }
    if (lunar1710Selected) { classCount = static_cast<int>(std::size(lunar1710Edits)); return lunar1710Edits; }
    if (mc263Selected) { classCount = static_cast<int>(sizeof(mc263Edits)/sizeof(mc263Edits[0])); return mc263Edits; }
    if (lunarSelected) {
        classCount = static_cast<int>(sizeof(lunarEdits) / sizeof(lunarEdits[0]));
        return lunarEdits;
    }
    classCount = kEditCount;
    return kEdits;
}

const ClassEdits* AllEdits(int& count) {
    const auto* base=BaseEdits(count);
#ifdef NOVA_PROXY_RUNTIME
    using namespace minecraft::proxyadapter;
    const auto version=compat::IsLunar189()?Version::Lunar189:compat::IsLunar1710()?Version::Lunar1710:
        compat::IsLunar12111()?Version::Lunar12111:compat::IsEaseCation()?Version::EaseCation:
        compat::IsNeoForge12110()?Version::NeoForge12110:
        compat::IsMinecraft263()?Version::Minecraft263:
        compat::UsesModernRuntime()?Version::Unsupported:Version::Forge1201;
    return ExtendEdits(base,count,version);
#else
    return base;
#endif
}

void SelectMinecraft263Edits(bool enabled) { mc263Selected = enabled; }
void SelectEaseCationEdits(bool enabled) { easecationSelected = enabled; }
void SelectNeoForge12110Edits(bool enabled) { neoforge12110Selected = enabled; }
const ClassEdits* GlfwModernEdits(int& count) {count=static_cast<int>(std::size(easecationEdits));return easecationEdits;}
void SelectCustomEdits(const ClassEdits* edits,int count) {customEdits=edits;customEditCount=count;}
void SelectLunar1710Edits(bool enabled) { lunar1710Selected = enabled; }

void SelectLunarEdits(bool lunar) { lunarSelected = lunar; mc263Selected = false; easecationSelected = false; neoforge12110Selected=false; customEdits=nullptr;customEditCount=0; }

}
