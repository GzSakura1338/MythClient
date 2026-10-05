#include "minecraft/java/api/class_edit.h"
#include "minecraft/hooks/Edits.h"
#include "audit.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>

int main() {
    nova::hook::SelectLunarEdits(true);
    int failed = 0;
    for (const char* name : {"net/minecraft/client/Minecraft", "net/minecraft/client/entity/EntityPlayerSP",
                             "net/minecraft/entity/player/EntityPlayer"}) {
        const auto* edits = nova::hook::EditsFor(name);
        std::string filename(name); std::replace(filename.begin(), filename.end(), '/', '_');
        std::ifstream input("classes/" + filename + ".class", std::ios::binary);
        std::vector<u1> data{std::istreambuf_iterator<char>(input), {}};
        if (data.empty()) { std::cerr << "Missing class: " << name << '\n'; return 1; }
        ClassFileEditor editor(data.data(), data.size());
        if (std::string(name) == "net/minecraft/client/Minecraft") {
            // Test the registered table, not just the declarations: a stale count
            // silently excluded these hooks from the transformed Minecraft class.
            for (const char* required : {"blockVanillaUse", "blockVanillaRelease", "blockVanillaHitBlock"}) {
                bool found = false;
                for (int i = 0; i < edits->count; ++i)
                    found |= std::string(edits->redirects[i].hook) == required;
                if (!found) { std::cerr << "Unregistered redirect " << required << '\n'; ++failed; }
            }
            const auto before = editor.serialize();
            const char* owner = "net/minecraft/client/multiplayer/PlayerControllerMP";
            const char* desc = "(Lnet/minecraft/entity/player/EntityPlayer;Lnet/minecraft/world/World;Lnet/minecraft/item/ItemStack;)Z";
            // Wildcard redirects retain the normal stack checks and cannot partially rewrite a class.
            if (editor.redirectCalls("*", "", owner, "sendUseItem", desc,
                                     "NativeHypixelTest", "badUse", "()Z")) ++failed;
            if (before != editor.serialize()) ++failed;
        }
        if (std::string(name) == "net/minecraft/entity/player/EntityPlayer") {
            // PUTFIELD double consumes receiver + two slots. Reject a read-like
            // descriptor without changing the class, then apply the real hooks.
            const auto before = editor.serialize();
            if (editor.redirectCalls("attackTargetEntityWithCurrentItem", "(Lnet/minecraft/entity/Entity;)V",
                name, "motionX", "D", "NativeHypixelTest", "badWrite", "(Lnet/minecraft/entity/player/EntityPlayer;)D", 0xB5))
                ++failed;
            if (before != editor.serialize()) ++failed;
            for (const char* field : {"motionX", "motionZ"}) {
                if (editor.findCalls("attackTargetEntityWithCurrentItem", "(Lnet/minecraft/entity/Entity;)V",
                    name, field, "D", 0xB5).size() != 1) {
                    std::cerr << "Unexpected attack velocity write count\n"; ++failed;
                }
            }
        }
        for (int i = 0; i < edits->count; ++i) {
            const auto& r = edits->redirects[i];
            if (!editor.redirectCalls(r.method, r.methodDesc, r.calleeOwner, r.calleeName, r.calleeDesc,
                "NativeHypixelTest", r.hook, r.hookDesc, r.opcode, r.limit, r.ordinal)) {
                std::cerr << "Missing redirect " << r.hook << '\n'; ++failed;
            }
        }
        auto append = [&](const nova::hook::Append& a) {
            const auto ref = editor.addMethodRef("NativeHypixelTest", a.hook, a.hookDesc);
            const auto code = nova::hook::AppendCode(ref, a.loads, a.loadCount);
            if (!(a.atHead ? editor.prependToMethodCode(a.method, a.methodDesc, code, a.loadCount) :
                a.allVoidReturns ? editor.appendBeforeVoidReturns(a.method, a.methodDesc, code, a.loadCount) :
                editor.appendBeforeLastReturn(a.method, a.methodDesc, code, a.loadCount))) {
                std::cerr << name << " append failed: " << a.hook << '\n'; ++failed;
            }
        };
        for (int i = 0; i < edits->guardCount; ++i) {
            const auto& g = edits->guards[i];
            if (!editor.insertCancelGuard(g.method, g.methodDesc, "NativeHypixelTest", g.hook, g.hookDesc, g.argCount)) {
                std::cerr << name << " guard failed: " << g.hook << '\n'; ++failed;
            }
        }
        for (int i = 0; i < edits->appendCount; ++i) {
            const auto& a = edits->appends[i];
            append(a);
        }
        std::string error;
        if (!audit::AuditClass(editor.serialize(), error)) { std::cerr << error << '\n'; ++failed; }
    }
    std::cout << "Lunar Hypixel hook rewrites and class structure: " << failed << " failures\n";
    return failed ? 1 : 0;
}
