#include "minecraft/java/api/class_edit.h"
#include "minecraft/hooks/Edits.h"
#include "../../bytecode/audit.h"
#include <atomic>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cassert>
#include <fstream>
#include <iostream>
#include <filesystem>
using jfloat = float;
using jdouble = double;
using jboolean = unsigned char;
using jclass = void*;
struct JNIEnv {};
#define JNICALL
#define JNI_TRUE 1
std::atomic<bool> g_enabled{false};
std::atomic<float> g_fixedFov{0};
// @FOV_METHODS@
int main(int argc, char** argv) {
    assert(argc == 3);
    // Start without a jump, converge monotonically, retarget without snapping,
    // and animate back to vanilla even after Scaffold is disabled.
    assert(AnimateKeepFov(70, 0, 0) == 70);
    assert(AnimateKeepFov(70, 120, 1) == 70);
    double value = AnimateKeepFov(70, 120, 1.05);
    assert(value > 70 && value < 120);
    assert(AnimateKeepFov(70, 120, 1.05) == value);
    double next = AnimateKeepFov(70, 90, 1.06);
    assert(next > 90 && next < value);
    value = next;
    for (int i = 1; i <= 100; ++i) {
        next = AnimateKeepFov(70, 0, 1.06 + i * .01);
        assert(next <= value && next >= 70);
        value = next;
    }
    assert(value == 70);
    // Equal wall-clock duration gives equal animation progress at 30/120 FPS.
    auto sample = [](int fps, double start) {
        AnimateKeepFov(70, 120, start);
        double out = 70;
        for (int i = 1; i <= fps / 5; ++i)
            out = AnimateKeepFov(70, 120, start + static_cast<double>(i) / fps);
        for (int i = 1; i <= 20; ++i) AnimateKeepFov(70, 0, start + .2 + i * .1);
        return out;
    };
    assert(std::abs(sample(30, 5) - sample(120, 10)) < 1e-9);
    g_fixedFov = 120;
    assert(KeepFovFloatHook(nullptr, nullptr, 82, 1) == 82);
    g_enabled = true;
    assert(KeepFovFloatHook(nullptr, nullptr, 82, 1) == 82);
    assert(KeepFovFloatHook(nullptr, nullptr, 70, 0) == 70);
    assert(KeepFovDoubleHook(nullptr, nullptr, 70.125, 0) == 70.125);
    nova::hook::SelectLunarEdits(std::string(argv[1]) == "lunar");
    nova::hook::SelectMinecraft263Edits(std::string(argv[1]) == "263");
    int count = 0, checked = 0;
    const auto edits = nova::hook::AllEdits(count);
    for (int c = 0; c < count; ++c) {
        const auto& e = edits[c];
        for (int i = 0; i < e.appendCount; ++i) {
            const auto& a = e.appends[i];
            if (std::string(a.hook) != "keepFov") continue;
            std::ifstream in(std::filesystem::path(argv[2]) / (std::string(e.internalName) + ".class"), std::ios::binary);
            std::vector<u1> bytes{std::istreambuf_iterator<char>(in), {}};
            assert(!bytes.empty());
            ClassFileEditor editor(bytes.data(), bytes.size());
            const auto ref = editor.addMethodRef("nova/test/FovBridge", a.hook, a.hookDesc);
            assert(ref && a.allValueReturns);
            assert(editor.appendBeforeValueReturns(a.method, a.methodDesc,
                nova::hook::AppendCode(ref, a.loads, a.loadCount), static_cast<u2>(a.loadCount)));
            std::string error;
            if (!audit::AuditClass(editor.serialize(), error)) {
                std::cerr << error; return 1;
            }
            ++checked;
        }
    }
    assert(checked == 1);
    std::cout << "PASS: " << argv[1] << " original camera class FOV return hook; enable/disable and hand-FOV isolation\n";
}
