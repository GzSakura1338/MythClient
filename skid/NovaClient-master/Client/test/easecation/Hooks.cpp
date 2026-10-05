#include "minecraft/java/api/class_edit.h"
#include "minecraft/hooks/Edits.h"
#ifdef NOVA_TEST_EASECATION_PROXY
#include "minecraft/proxy/ProxyHooks.h"
#endif
#ifdef NOVA_TEST_LUNAR189
#include "minecraft/accounts/lunar/AccountEdits.h"
#endif
#ifdef NOVA_TEST_LUNAR12111
#include "minecraft/compat/lunar12111/Edits.h"
#include "minecraft/compat/lunar12111/Mapping.h"
#include "minecraft/accounts/lunar/AccountMapping.h"
#include <unordered_map>
#endif
#ifdef NOVA_TEST_EASECATION_PROXY
#include <unordered_map>
#endif
#include "../bytecode/audit.h"
#include <filesystem>
#include <fstream>
#include <iostream>
std::filesystem::path LongPath(std::filesystem::path path){
    return std::filesystem::path(L"\\\\?\\"+std::filesystem::absolute(path).make_preferred().wstring());
}

int main(int argc,char** argv) {
    if(argc!=2 && argc!=3 && argc!=4)return 2;
#ifdef NOVA_TEST_LUNAR12111
    namespace accounts=nova::minecraft::accounts::mapping;
    accounts::activeBuild=argc==4?(std::string(argv[3])=="1002"?accounts::Build::October2:std::string(argv[3])=="0930"?accounts::Build::September30:accounts::Build::Unknown):accounts::Build::September27;
    nova::compat::lunar12111::SelectEdits();
#elif defined(NOVA_TEST_LUNAR189)
    namespace accounts=nova::minecraft::accounts::mapping;
    nova::hook::SelectLunarEdits(true);
    accounts::ResolvedAccountEdits accountEdits;
    accountEdits.Reset(argc==4&&std::string(argv[3])=="1002"?accounts::Build::October2:argc==4&&std::string(argv[3])=="0930"?accounts::Build::September30:accounts::Build::September27);
    int baseCount=0;const auto* base=nova::hook::AllEdits(baseCount);
    std::vector<nova::hook::ClassEdits> combined(base,base+baseCount);
    combined.insert(combined.end(),accountEdits.classes.begin(),accountEdits.classes.end());
    nova::hook::SelectCustomEdits(combined.data(),static_cast<int>(combined.size()));
#elif defined(NOVA_TEST_NEOFORGE12110)
    nova::hook::SelectNeoForge12110Edits(true);
#else
    nova::hook::SelectEaseCationEdits(true);
#endif
    int count=0,failures=0,rules=0;
    const auto* edits=nova::hook::AllEdits(count);
#ifdef NOVA_TEST_EASECATION_PROXY
#ifdef NOVA_TEST_NEOFORGE12110
    edits=nova::minecraft::proxyadapter::ExtendEdits(edits,count,nova::minecraft::proxyadapter::Version::NeoForge12110);
#else
    edits=nova::minecraft::proxyadapter::ExtendEdits(edits,count,nova::minecraft::proxyadapter::Version::EaseCation);
#endif
#endif
#ifdef NOVA_TEST_LUNAR12111
    std::unordered_map<std::string,std::string> expected;
#include "native-hooks.inc"
#elif defined(NOVA_TEST_EASECATION_PROXY)
    std::unordered_map<std::string,std::string> expected;
#include "native-hooks.inc"
#endif
    for(int c=0;c<count;++c){
        const auto& e=edits[c];
        std::ifstream file(LongPath(std::filesystem::path(argv[1])/(std::string(e.internalName)+".class")),std::ios::binary);
        if(!file){
            std::string flat=e.internalName;for(char& ch:flat)if(ch=='/')ch='.';
            file.clear();file.open(LongPath(std::filesystem::path(argv[1])/(flat+".class")),std::ios::binary);
        }
        std::vector<u1> bytes{std::istreambuf_iterator<char>(file),{}};
        if(bytes.empty()){std::cerr<<"missing "<<e.internalName<<'\n';++failures;continue;}
        ClassFileEditor editor(bytes.data(),bytes.size());
        auto check=[&](bool ok,const char* name){++rules;if(!ok){++failures;std::cerr<<e.internalName<<": "<<name<<'\n';}};
#if defined(NOVA_TEST_LUNAR12111) || defined(NOVA_TEST_EASECATION_PROXY)
        auto signature=[&](const auto& r){auto it=expected.find(r.hook);check(it!=expected.end()&&it->second==r.hookDesc,r.hook);};
        for(int i=0;i<e.count;++i)signature(e.redirects[i]);
        for(int i=0;i<e.guardCount;++i)signature(e.guards[i]);
        for(int i=0;i<e.appendCount;++i)signature(e.appends[i]);
#endif
        for(int i=0;i<e.count;++i){const auto& r=e.redirects[i];
            check(editor.redirectCalls(r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc,
                "nova/test/EaseCation",r.hook,r.hookDesc,r.opcode,r.limit,r.ordinal),r.hook);}
        for(int i=0;i<e.guardCount;++i){const auto& g=e.guards[i];
            check(editor.insertCancelGuard(g.method,g.methodDesc,"nova/test/EaseCation",g.hook,g.hookDesc,g.argCount,g.entryReturn),g.hook);}
        for(int i=0;i<e.appendCount;++i){const auto& a=e.appends[i];
            auto ref=editor.addMethodRef("nova/test/EaseCation",a.hook,a.hookDesc);
            auto code=nova::hook::AppendCode(ref,a.loads,a.loadCount);
            check(a.atHead?editor.prependToMethodCode(a.method,a.methodDesc,code,a.loadCount):
                a.allVoidReturns?editor.appendBeforeVoidReturns(a.method,a.methodDesc,code,a.loadCount):
                a.allValueReturns?editor.appendBeforeValueReturns(a.method,a.methodDesc,code,a.loadCount):
                editor.appendBeforeLastReturn(a.method,a.methodDesc,code,a.loadCount),a.hook);}
        std::string error;auto bytesOut=editor.serialize();
        const bool valid=audit::AuditClass(bytesOut,error);check(valid,error.c_str());
        if(valid && argc>=3){
            auto destination=LongPath(std::filesystem::path(argv[2])/(std::string(e.internalName)+".class"));
            std::filesystem::create_directories(destination.parent_path());
            std::ofstream output(destination,std::ios::binary);
            output.write(reinterpret_cast<const char*>(bytesOut.data()),bytesOut.size());
        }
    }
    std::cout<<"Hooks: "<<count<<" classes, "<<rules<<" rewrite checks, "<<failures<<" failures\n";
    return failures?1:0;
}
