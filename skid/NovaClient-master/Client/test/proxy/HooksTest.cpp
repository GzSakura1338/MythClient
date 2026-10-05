#include "minecraft/proxy/ProxyHooks.h"
#include "minecraft/java/api/class_edit.h"
#include "../bytecode/audit.h"
#include <filesystem>
#include <fstream>
#include <cassert>
#include <iostream>
int main(int argc,char** argv){
    using namespace nova::minecraft::proxyadapter;
    assert(argc==3);const std::string name=argv[1];
    const auto version=name=="lunar189"?Version::Lunar189:name=="lunar1710"?Version::Lunar1710:
        name=="lunar12111"?Version::Lunar12111:name=="forge"?Version::Forge1201:
        name=="263"?Version::Minecraft263:Version::EaseCation;
    const nova::hook::ClassEdits dummy{};int count=0;const auto* edits=ExtendEdits(&dummy,count,version);assert(count==2);
    int rules=0;
    for(int c=0;c<count;++c){const auto& e=edits[c];
        std::ifstream file(std::filesystem::path(argv[2])/(std::string(e.internalName)+".class"),std::ios::binary);
        if(!file){auto flat=std::string(e.internalName);std::replace(flat.begin(),flat.end(),'/','.');file.clear();file.open(std::filesystem::path(argv[2])/(flat+".class"),std::ios::binary);}
        std::vector<u1> bytes{std::istreambuf_iterator<char>(file),{}};assert(!bytes.empty());
        ClassFileEditor editor(bytes.data(),bytes.size());
        for(int r=0;r<e.count;++r){const auto& rule=e.redirects[r];
            assert(editor.findCalls(rule.method,rule.methodDesc,rule.calleeOwner,rule.calleeName,rule.calleeDesc).size()==1);
            assert(editor.redirectCalls(rule.method,rule.methodDesc,rule.calleeOwner,rule.calleeName,rule.calleeDesc,"nova/ExistingNativeHooks",rule.hook,rule.hookDesc,rule.opcode,rule.limit,rule.ordinal));++rules;
        }
        for(int i=0;i<e.appendCount;++i){const auto& rule=e.appends[i];const auto ref=editor.addMethodRef("nova/ExistingNativeHooks",rule.hook,rule.hookDesc);
            const auto code=nova::hook::AppendCode(ref,rule.loads,rule.loadCount);
            assert(rule.atHead?editor.prependToMethodCode(rule.method,rule.methodDesc,code,rule.loadCount):editor.appendBeforeVoidReturns(rule.method,rule.methodDesc,code,rule.loadCount));++rules;
        }
        std::string error;assert(audit::AuditClass(editor.serialize(),error));
    }
    assert(rules==3);
    // Reproduce the legacy account adapter composing an already extended table.
    // Re-extension must preserve exactly one proxy rule and its backing memory.
    std::vector<nova::hook::ClassEdits> accountComposition(edits,edits+count);
    int composedCount=int(accountComposition.size());
    const auto* twice=ExtendEdits(accountComposition.data(),composedCount,version);
    assert(twice==accountComposition.data()&&composedCount==count);
    int transport=0,begins=0,ends=0;
    for(int c=0;c<composedCount;++c){for(int r=0;r<twice[c].count;++r)transport+=std::string(twice[c].redirects[r].hook)=="proxyConnect";
        for(int a=0;a<twice[c].appendCount;++a){begins+=std::string(twice[c].appends[a].hook)=="proxyJoinBegin";ends+=std::string(twice[c].appends[a].hook)=="proxyJoinEnd";}
    }
    assert(transport==1&&begins==1&&ends==1);
    std::cout<<name<<": join scope, exact Bootstrap redirect and repeated account composition passed\n";
}
