#include "nova/class_edit.h"
#include "audit.h"
#include <fstream>
#include <iterator>
#include <iostream>
int main(int argc, char** argv) {
    if (argc != 4) return 2;
    std::ifstream in(argv[1], std::ios::binary);
    std::vector<u1> bytes{std::istreambuf_iterator<char>(in), {}};
    if (bytes.empty()) return 3;
    ClassFileEditor redirect(bytes.data(), bytes.size()), insertion(bytes.data(), bytes.size());
    const char* desc = "(LInterfaceProbe$Reader;I)I";
    if (!redirect.redirectCalls("invoke", desc, "InterfaceProbe$Reader", "get", "()I",
        "InterfaceProbe", "hook", "(LInterfaceProbe$Reader;)I")) return 4;
    auto ref = insertion.addMethodRef("InterfaceProbe", "mark", "()V");
    std::vector<u1> code{0xB8, static_cast<u1>(ref >> 8), static_cast<u1>(ref), 0};
    if (!insertion.insertBeforeCall("invoke", desc, "InterfaceProbe$Reader", "get", "()I",
        0, code, 0, 0xB9, true)) return 5;
    int index = 2;
    for (auto* editor : {&redirect, &insertion}) {
        auto out = editor->serialize();
        std::string error;
        if (!audit::AuditClass(out, error)) { std::cerr << error; return 6; }
        std::ofstream file(argv[index++], std::ios::binary);
        file.write(reinterpret_cast<const char*>(out.data()), out.size());
    }
}
