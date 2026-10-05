#include "nova/class_edit.h"
#include "hook/Edits.h"
#include "audit.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <algorithm>
int main() {
    nova::hook::SelectLunarEdits(true);
    int count = 0, failed = 0;
    const auto* edits = nova::hook::AllEdits(count);
    for (int c = 0; c < count; ++c) {
        const auto& e = edits[c];
        std::string file(e.internalName);
        std::replace(file.begin(), file.end(), '/', '_');
        std::ifstream in(file + ".class", std::ios::binary);
        std::vector<u1> bytes{std::istreambuf_iterator<char>(in), {}};
        if (bytes.empty()) { ++failed; continue; }
        ClassFileEditor editor(bytes.data(), bytes.size());
        for (int i = 0; i < e.count; ++i) {
            const auto& r = e.redirects[i];
            bool ok = editor.redirectCalls(r.method, r.methodDesc, r.calleeOwner, r.calleeName,
                r.calleeDesc, "NovaLunarTestBridge", r.hook, r.hookDesc, r.opcode, r.limit, r.ordinal);
            std::cout << e.internalName << "." << r.method << " -> " << r.hook << " " << ok << "\n";
            if (!ok) ++failed;
        }
        for (int i = 0; i < e.guardCount; ++i) {
            const auto& g = e.guards[i];
            bool ok = editor.insertCancelGuard(g.method, g.methodDesc, "NovaLunarTestBridge", g.hook, g.hookDesc, g.argCount);
            std::cout << e.internalName << "." << g.method << " guard " << ok << "\n";
            if (!ok) ++failed;
        }
        for (int i = 0; i < e.appendCount; ++i) {
            const auto& a = e.appends[i];
            auto ref = editor.addMethodRef("NovaLunarTestBridge", a.hook, a.hookDesc);
            auto code = nova::hook::AppendCode(ref, a.loads, a.loadCount);
            bool ok = ref && (a.atHead ? editor.prependToMethodCode(a.method, a.methodDesc, code, a.loadCount) :
                editor.appendBeforeLastReturn(a.method, a.methodDesc, code, a.loadCount));
            std::cout << e.internalName << "." << a.method << " append " << ok << "\n";
            if (!ok) ++failed;
        }
        auto result = editor.serialize();
        std::string error;
        bool ok = audit::AuditClass(result, error);
        std::cout << e.internalName << " audit " << ok << " " << error << "\n";
        if (!ok) ++failed;
        std::ofstream out(file + ".patched.class", std::ios::binary);
        out.write(reinterpret_cast<const char*>(result.data()), result.size());
    }
    std::cout << "failures=" << failed << "\n";
    return failed ? 1 : 0;
}
