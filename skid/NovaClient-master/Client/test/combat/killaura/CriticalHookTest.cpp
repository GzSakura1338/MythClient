#include "minecraft/java/api/class_edit.h"
#include "minecraft/hooks/Edits.h"
#include "../../bytecode/audit.h"
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    nova::hook::SelectMinecraft263Edits(std::string(argv[1]) == "modern");
    auto rules = nova::hook::EditsFor("net/minecraft/client/player/LocalPlayer");
    if (!rules) return 3;
    std::ifstream file(argv[2],std::ios::binary);
    std::vector<u1> bytes{std::istreambuf_iterator<char>(file),{}};
    if (bytes.empty()) return 4;
    ClassFileEditor editor(bytes.data(),bytes.size());
    for (int i=0;i<rules->appendCount;++i) {
        const auto& a=rules->appends[i];
        if (std::string(a.hook)!="criticalHit") continue;
        const auto ref=editor.addMethodRef("nova/test/Critical",a.hook,a.hookDesc);
        if (!a.atHead || !editor.prependToMethodCode(a.method,a.methodDesc,
            nova::hook::AppendCode(ref,a.loads,a.loadCount),a.loadCount)) return 5;
        std::string error;
        if (!audit::AuditClass(editor.serialize(),error)) { std::cerr<<error; return 6; }
        const auto calls=editor.findCalls(a.method,a.methodDesc,"nova/test/Critical",a.hook,a.hookDesc);
        if (calls.size()!=1) return 7;
        std::cout<<argv[1]<<": vanilla critical callback instrumented once; bytecode audit passed\n";
        return 0;
    }
    return 8;
}
