#pragma once

#include <cstdint>
#include <vector>

namespace nova::hook {

struct Redirect {
    const char* method;
    const char* methodDesc;

    const char* calleeOwner;
    const char* calleeName;
    const char* calleeDesc;

    const char* hook;
    const char* hookDesc;

    int limit;
    int ordinal = -1;
    unsigned char opcode = 0;
};

struct Insertion {
    const char* method;
    const char* methodDesc;

    const char* calleeOwner;
    const char* calleeName;
    const char* calleeDesc;
    bool calleeIsStatic;
    int ordinal;

    const char* hook;
    const char* hookDesc;

    int argCount;
    int storeSlot;
};

struct Guard {
    const char* method;
    const char* methodDesc;

    const char* hook;
    const char* hookDesc;

    int argCount;
    bool entryReturn = false;
};

struct StackLoad {
    unsigned char opcode;
    unsigned char index;
};

struct Append {
    const char* method;
    const char* methodDesc;

    const char* hook;
    const char* hookDesc;

    const StackLoad* loads;
    int loadCount;

    bool atHead;
    bool allVoidReturns = false;
    bool allValueReturns = false;
};

struct ClassEdits {
    const char* internalName;
    const Redirect* redirects;
    int count;
    const Insertion* insertions = nullptr;
    int insertionCount = 0;
    const Guard* guards = nullptr;
    int guardCount = 0;
    const Append* appends = nullptr;
    int appendCount = 0;
};

void EmitLoad(std::vector<unsigned char>& out, unsigned char opcode, unsigned char index);

std::vector<unsigned char> AppendCode(std::uint16_t methodRef, const StackLoad* loads,
                                      int loadCount);

const ClassEdits* EditsFor(const char* internalName);

const ClassEdits* AllEdits(int& classCount);
// Composers (e.g. account adapters) use the unextended selected table. AllEdits
// is the final installation view and may include host-owned runtime rules.
const ClassEdits* BaseEdits(int& classCount);
void SelectLunarEdits(bool lunar);
void SelectLunar1710Edits(bool enabled);
void SelectMinecraft263Edits(bool enabled);
void SelectEaseCationEdits(bool enabled);
void SelectNeoForge12110Edits(bool enabled);
// Adapter-owned rules must outlive hook installation and transformer teardown.
const ClassEdits* GlfwModernEdits(int& count);
void SelectCustomEdits(const ClassEdits* edits,int count);

}
