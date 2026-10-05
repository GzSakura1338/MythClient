#include <cassert>
#include <cstdio>
#include <fstream>
#include <cstring>
#include "minecraft/hooks/Edits.h"
#include "minecraft/java/api/class_edit.h"
#include "test/bytecode/audit.h"
int main(int argc,char** argv){
    assert(argc==2);
    std::ifstream file(argv[1],std::ios::binary);
    std::vector<u1> bytes{std::istreambuf_iterator<char>(file),{}};
    assert(!bytes.empty());
    ClassFileEditor editor(bytes.data(),bytes.size());
    auto edits=nova::hook::EditsFor("net/minecraft/client/Minecraft");assert(edits);
    for(int i=0;i<edits->count;++i){const auto& r=edits->redirects[i];
        assert(editor.redirectCalls(r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc,"Bridge",r.hook,r.hookDesc,r.opcode,r.limit,r.ordinal));
    }
    for(int i=0;i<edits->insertionCount;++i){const auto& r=edits->insertions[i];
        assert(editor.insertHookCall(r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc,r.calleeIsStatic,r.ordinal,"Bridge",r.hook,r.hookDesc,r.argCount,r.storeSlot));
    }
    for(int i=0;i<edits->guardCount;++i){const auto& r=edits->guards[i];
        assert(editor.insertCancelGuard(r.method,r.methodDesc,"Bridge",r.hook,r.hookDesc,r.argCount,r.entryReturn));
    }
    bool interaction=false;
    for(int i=0;i<edits->appendCount;++i){const auto& r=edits->appends[i];
        auto code=nova::hook::AppendCode(editor.addMethodRef("Bridge",r.hook,r.hookDesc),r.loads,r.loadCount);
        assert(r.atHead?editor.prependToMethodCode(r.method,r.methodDesc,code,r.loadCount):
            r.allVoidReturns?editor.appendBeforeVoidReturns(r.method,r.methodDesc,code,r.loadCount):
            r.allValueReturns?editor.appendBeforeValueReturns(r.method,r.methodDesc,code,r.loadCount):
            editor.appendBeforeLastReturn(r.method,r.methodDesc,code,r.loadCount));
        interaction|=std::strcmp(r.hook,"preInteract")==0;
    }
    std::string error;assert(interaction&&audit::AuditClass(editor.serialize(),error));
    std::puts("PASS: Forge Minecraft hooks including preInteract; branches, stack maps and exception ranges valid");
}
