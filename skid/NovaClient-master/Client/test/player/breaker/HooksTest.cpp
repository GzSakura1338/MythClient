#include "minecraft/java/api/class_edit.h"
#include "minecraft/hooks/Edits.h"
#include "../../bytecode/audit.h"
#include <fstream>
#include <iostream>
#include <string>
int main(int argc, char **argv) {
    if (argc != 3)
        return 2;
    const std::string profile = argv[1];
    nova::hook::SelectLunarEdits(profile == "lunar");
    nova::hook::SelectMinecraft263Edits(profile == "263");
    const auto *edits = nova::hook::EditsFor("net/minecraft/client/Minecraft");
    std::ifstream file(argv[2], std::ios::binary);
    std::vector<u1> bytes{std::istreambuf_iterator<char>(file), {}};
    if (!edits || bytes.empty())
        return 3;
    ClassFileEditor editor(bytes.data(), bytes.size());
    int count = 0;
    for (int i = 0; i < edits->count; ++i) {
        const auto &r = edits->redirects[i];
        if (std::string(r.hook) != (profile == "lunar" ? "blockVanillaHitBlock" : "breakerContinue"))
            continue;
        if (!editor.redirectCalls(r.method, r.methodDesc, r.calleeOwner, r.calleeName, r.calleeDesc,
                                  "nova/test/BreakerBridge", r.hook, r.hookDesc, r.opcode, r.limit,
                                  r.ordinal))
            return 4;
        ++count;
    }
    std::string error;
    if (count != 1 || !audit::AuditClass(editor.serialize(), error)) {
        std::cerr << count << " " << error;
        return 6;
    }
    std::cout << "PASS: " << profile << " mining hook on real Minecraft class / stack-map audit\n";
}
