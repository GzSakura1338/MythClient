#include "minecraft/java/api/class_edit.h"
#include "minecraft/java/api/classfile.h"
#include "minecraft/hooks/Edits.h"
#include "../bytecode/audit.h"
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <set>

int main(int argc,char** argv){
    if(argc!=3)return 2;
    nova::hook::SelectMinecraft263Edits(true);
    int count=0;auto edits=nova::hook::AllEdits(count);
    int failures=0,rules=0;
    ClassBuilder bridge("nova/test/Bridge263","java/lang/Object",69);
    std::set<std::string> signatures;
    auto hook=[&](const char* name,const char* desc){
        if(signatures.insert(std::string(name)+desc).second)bridge.addNativeMethod(name,desc,ACC_PUBLIC|ACC_STATIC|ACC_NATIVE);
    };
    for(int c=0;c<count;++c){
        auto& e=edits[c];auto path=std::filesystem::path(argv[1])/(std::string(e.internalName)+".class");
        std::ifstream file(path,std::ios::binary);std::vector<u1> bytes{std::istreambuf_iterator<char>(file),{}};
        if(bytes.empty()){std::printf("missing %s\n",e.internalName);++failures;continue;}
        ClassFileEditor editor(bytes.data(),bytes.size());
        auto check=[&](bool ok,const char* name){++rules;if(!ok){++failures;std::printf("FAIL %s %s\n",e.internalName,name);}};
        for(int i=0;i<e.count;++i){auto& r=e.redirects[i];hook(r.hook,r.hookDesc);
            check(editor.redirectCalls(r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc,"nova/test/Bridge263",r.hook,r.hookDesc,r.opcode,r.limit,r.ordinal),r.hook);}
        for(int i=0;i<e.guardCount;++i){auto& g=e.guards[i];hook(g.hook,g.hookDesc);
            check(editor.insertCancelGuard(g.method,g.methodDesc,"nova/test/Bridge263",g.hook,g.hookDesc,g.argCount),g.hook);}
        for(int i=0;i<e.insertionCount;++i){auto& v=e.insertions[i];hook(v.hook,v.hookDesc);
            check(editor.insertHookCall(v.method,v.methodDesc,v.calleeOwner,v.calleeName,v.calleeDesc,v.calleeIsStatic,v.ordinal,"nova/test/Bridge263",v.hook,v.hookDesc,v.argCount,v.storeSlot),v.hook);}
        for(int i=0;i<e.appendCount;++i){auto& a=e.appends[i];hook(a.hook,a.hookDesc);auto ref=editor.addMethodRef("nova/test/Bridge263",a.hook,a.hookDesc);auto code=nova::hook::AppendCode(ref,a.loads,a.loadCount);
            const auto depth=static_cast<u2>(a.loadCount);
            check(a.atHead?editor.prependToMethodCode(a.method,a.methodDesc,code,depth):
                a.allValueReturns?editor.appendBeforeValueReturns(a.method,a.methodDesc,code,depth):
                a.allVoidReturns?editor.appendBeforeVoidReturns(a.method,a.methodDesc,code,depth):
                editor.appendBeforeLastReturn(a.method,a.methodDesc,code,depth),a.hook);}
        auto result=editor.serialize();std::string err;check(audit::AuditClass(result,err),err.c_str());
        auto target=std::filesystem::path(argv[2])/(std::string(e.internalName)+".class");std::filesystem::create_directories(target.parent_path());
        std::ofstream out(target,std::ios::binary);out.write(reinterpret_cast<const char*>(result.data()),result.size());
    }
    auto target=std::filesystem::path(argv[2])/"nova/test/Bridge263.class";std::filesystem::create_directories(target.parent_path());
    auto bytes=bridge.build();std::ofstream out(target,std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    std::printf("26.3: %d classes, %d checks, %d failures\n",count,rules,failures);return failures?1:0;
}
