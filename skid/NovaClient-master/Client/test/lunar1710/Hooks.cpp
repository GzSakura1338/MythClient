#include "minecraft/java/api/class_edit.h"
#include "minecraft/hooks/Edits.h"
#include "audit.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <algorithm>
int main() {
    nova::hook::SelectLunar1710Edits(true);
    int count = 0, failures = 0;
    const auto* classes = nova::hook::AllEdits(count);
    for (int c = 0; c < count; ++c) {
        const auto& e = classes[c];
        std::string filename(e.internalName); std::replace(filename.begin(), filename.end(), '/', '.');
        std::ifstream file("classes/" + filename, std::ios::binary);
        std::vector<u1> bytes{std::istreambuf_iterator<char>(file), {}};
        if (bytes.empty()) { std::cerr << "missing " << filename << '\n'; ++failures; continue; }
        ClassFileEditor editor(bytes.data(), bytes.size());
        for (int i=0; i<e.count; ++i) {
            const auto& r=e.redirects[i];
            if (!editor.redirectCalls(r.method,r.methodDesc,r.calleeOwner,r.calleeName,r.calleeDesc,
                "NativeLunar1710Test",r.hook,r.hookDesc,r.opcode,r.limit,r.ordinal)) {
                std::cerr << filename << " redirect " << r.method << " -> " << r.hook << '\n'; ++failures;
            }
        }
        for (int i=0;i<e.guardCount;++i) {
            const auto& g=e.guards[i];
            if(!editor.insertCancelGuard(g.method,g.methodDesc,"NativeLunar1710Test",g.hook,g.hookDesc,g.argCount)) {
                std::cerr<<filename<<" guard "<<g.hook<<'\n'; ++failures;
            }
        }
        for(int i=0;i<e.appendCount;++i) {
            const auto& a=e.appends[i];
            auto ref=editor.addMethodRef("NativeLunar1710Test",a.hook,a.hookDesc);
            auto code=nova::hook::AppendCode(ref,a.loads,a.loadCount);
            bool ok=a.atHead?editor.prependToMethodCode(a.method,a.methodDesc,code,a.loadCount):
                a.allVoidReturns?editor.appendBeforeVoidReturns(a.method,a.methodDesc,code,a.loadCount):
                a.allValueReturns?editor.appendBeforeValueReturns(a.method,a.methodDesc,code,a.loadCount):
                editor.appendBeforeLastReturn(a.method,a.methodDesc,code,a.loadCount);
            if(!ok){std::cerr<<filename<<" append "<<a.hook<<'\n';++failures;}
        }
        std::string error;
        if(!audit::AuditClass(editor.serialize(),error)){std::cerr<<error<<'\n';++failures;}
    }
    std::cout<<count<<" classes, "<<failures<<" rewrite failures\n";
    return failures?1:0;
}
