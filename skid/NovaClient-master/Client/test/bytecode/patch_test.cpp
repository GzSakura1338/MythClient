// Offline correctness harness for ClassFileEditor.
//
// The rewriter is the one part of the event system that cannot be debugged once
// it is inside the game: a bad StackMapTable is a VerifyError at class load, and
// the classes it edits are the ones the whole game is built out of. So it runs
// here against real bytes first, and the results are handed to the JVM's own
// verifier and executed.
//
// Two bodies of work:
//   Probe -- a Java class built for this test, patched with every edit shape the
//            event hookups use, then loaded and called. Behavioural, and the one
//            that proves the rewrite actually runs.
//   game  -- the real Entity/LivingEntity out of the srg jar, patched with the
//            edits the game itself will be patched with -- minecraft/hooks/Edits.cpp is
//            data and links in here -- then taken apart again by audit.h: every
//            branch target, switch target, exception range and stack map frame
//            has to still name an instruction boundary.
//
// Usage: patch_test <Probe.class> <outdir> <game-class-dir>

#include "minecraft/java/api/class_edit.h"
#include "minecraft/java/api/classfile.h"
#include "minecraft/hooks/Edits.h"
#include "audit.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// plumbing
// ---------------------------------------------------------------------------

static std::vector<u1> ReadFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return {};
    return std::vector<u1>(std::istreambuf_iterator<char>(in),
                           std::istreambuf_iterator<char>());
}

static bool WriteFile(const fs::path& p, const std::vector<u1>& bytes) {
    if (p.has_parent_path()) fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    return out.good();
}

static std::vector<u1> InvokeStatic(u2 ref) {
    return {0xB8, u1((ref >> 8) & 0xFF), u1(ref & 0xFF)};
}

static std::vector<u1> GetStatic(u2 ref) {
    return {0xB2, u1((ref >> 8) & 0xFF), u1(ref & 0xFF)};
}

template <typename... Parts>
static std::vector<u1> Cat(Parts... parts) {
    std::vector<u1> out;
    auto add = [&](const std::vector<u1>& v) { out.insert(out.end(), v.begin(), v.end()); };
    (add(parts), ...);
    return out;
}

static int g_failures = 0;

static void Expect(bool ok, const std::string& what) {
    std::printf("  %s %s\n", ok ? "ok  " : "FAIL", what.c_str());
    if (!ok) ++g_failures;
}

// The auditor is the whole point of the game-class half, so a class that fails
// it is a failure even when the edit itself reported success.
static void ExpectAudit(const std::vector<u1>& bytes, const std::string& what) {
    std::string err;
    const bool ok = !bytes.empty() && audit::AuditClass(bytes, err);
    Expect(ok, what + (ok ? "" : "  [" + err + "]"));
}

// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    if (argc < 4) {
        std::fprintf(stderr, "usage: patch_test <Probe.class> <outdir> <game-class-dir>\n");
        return 2;
    }
    const fs::path probeIn = argv[1];
    const fs::path outDir  = argv[2];
    const fs::path clsDir  = argv[3];

    // ---- the bridge class every hook points at ----------------------------
    //
    // Coded rather than native so the test can run without attaching an agent.
    // What the rewriter cares about is the descriptor; a body that returns a
    // recognisable constant is what proves the redirect took effect.
    ClassBuilder cb("Bridge", "java/lang/Object", 52);

    const u2 bumpRef = cb.methodRef("Verify", "bump", "()V");
    cb.addCodedMethod("mark", "()V", ACC_PUBLIC | ACC_STATIC,
                      Cat(InvokeStatic(bumpRef), std::vector<u1>{0xB1}), 0, 0);

    cb.addCodedMethod("hookYaw", "(LProbe;)I", ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x10, 99, 0xAC}, 1, 1);   // bipush 99; ireturn

    cb.addCodedMethod("sink", "(LProbe;)V", ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0xB1}, 1, 1);

    // The hooks the game-class half points at.
    //
    // Spelled out here by hand rather than pulled from the production hook
    // table, and that is the point of the duplication: the rewriter resolves
    // `Bridge.viewVector` by name out of the edited class's own constant pool
    // and never asks whether such a method exists, so a hook table and an edit
    // table that have drifted apart produce a class that loads clean and a call
    // site that resolves to nothing. Two lists checked against each other catch
    // that; one list generated from the other cannot. That check is below, and
    // `nova::bridge::Install` makes the same one at startup against the real
    // table.
    //
    // Receivers are all `Entity`. A call site whose static type is
    // LivingEntity or Player passes to an Entity parameter and not the other
    // way round, so the wide spelling is the only one safe at every site.
    const char* const kEntity = "net/minecraft/world/entity/Entity";

    const std::string kEntityVoid = std::string("(L") + kEntity + ";)V";
    const std::string kEntityF    = std::string("(L") + kEntity + ";)F";
    const std::string kEntityZ    = std::string("(L") + kEntity + ";)Z";
    const std::string kViewVec    = std::string("(L") + kEntity +
                                    ";FF)Lnet/minecraft/world/phys/Vec3;";

    // Declaring the descriptor is all that is needed; the rewriter resolves
    // these by name from the *edited* class's pool, and bridge-internal
    // references are what ClassBuilder has to be told about. Bodies are never
    // run -- no game class is loaded here -- so each returns a constant that
    // keeps the class well formed.
    cb.addCodedMethod("stuck", kEntityVoid, ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0xB1}, 1, 1);

    cb.addCodedMethod("strafeYaw", kEntityF, ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x0B, 0xAE}, 1, 1);       // fconst_0; freturn

    cb.addCodedMethod("jumpYaw", kEntityF, ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x0B, 0xAE}, 1, 1);

    cb.addCodedMethod("bodyYaw", kEntityF, ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x0B, 0xAE}, 1, 1);

    cb.addCodedMethod("usingItem", kEntityZ, ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x03, 0xAC}, 1, 1);       // iconst_0; ireturn

    // Three floats in, an object out: the widest of the shapes here, and the
    // only one whose descriptor names a class the harness never loads.
    cb.addCodedMethod("viewVector", kViewVec, ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x01, 0xB0}, 1, 3);       // aconst_null; areturn

    // The hooks the insertion tests point at, one per return category the
    // emitter has to choose a store opcode for. Each answers with something the
    // caller could not have passed in -- the receiver for the object case, one
    // more for the numbers -- so that where the field's value came from is
    // never a guess.
    const std::string kArg3 = "(Ljava/lang/Object;Ljava/lang/Object;";

    cb.addCodedMethod("routeObject", kArg3 + "Ljava/lang/Object;)Ljava/lang/Object;",
                      ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x2A, 0xB0}, 1, 3);       // aload_0; areturn
    // fload_2 is 0x24 and iload_2 is 0x1C -- the short forms run _0.._3 from
    // their base, so the slot and the opcode differ by exactly the slot number.
    // Getting this wrong is what the Probe.carried test is for: the verifier
    // rejects the class, and it says which index was invalid.
    // Two of stack, not one: the load and the constant are both there before
    // the add consumes them.
    cb.addCodedMethod("routeFloat", kArg3 + "F)F", ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x24, 0x0C, 0x62, 0xAE}, 2, 3);  // fload_2; fconst_1; fadd; freturn
    cb.addCodedMethod("routeInt", kArg3 + "I)I", ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x1C, 0x04, 0x60, 0xAC}, 2, 3);  // iload_2; iconst_1; iadd; ireturn

    // The rest of the production hooks. Declaring them here is what lets the
    // JVM's own verifier run over a patched game class: without the method in
    // this class the patched call site resolves to nothing, and a missing
    // method and a bad frame come back as different failures.
    cb.addCodedMethod("fallFlyingPitch", kEntityF, ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x0B, 0xAE}, 1, 1);

    cb.addCodedMethod("attackYaw", kEntityF, ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x0B, 0xAE}, 1, 1);

    cb.addCodedMethod("motionYaw", kEntityF, ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x0B, 0xAE}, 1, 1);

    cb.addCodedMethod("motionPitch", kEntityF, ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x0B, 0xAE}, 1, 1);

    cb.addCodedMethod("motionOnGround", kEntityZ, ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x03, 0xAC}, 1, 1);

    const std::string kLocalPlayer = "net/minecraft/client/player/LocalPlayer";

    cb.addCodedMethod("playerTick", std::string("(L") + kLocalPlayer + ";)Z",
                      ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x03, 0xAC}, 1, 1);

    cb.addCodedMethod("beginMotion", std::string("(L") + kLocalPlayer + ";)V",
                      ACC_PUBLIC | ACC_STATIC, std::vector<u1>{0xB1}, 1, 1);

    cb.addCodedMethod("endMotion", std::string("(L") + kLocalPlayer + ";)V",
                      ACC_PUBLIC | ACC_STATIC, std::vector<u1>{0xB1}, 1, 1);

    cb.addCodedMethod("stuckInBlock",
                      std::string("(L") + kEntity +
                          ";Lnet/minecraft/world/level/block/state/BlockState;"
                          "Lnet/minecraft/world/phys/Vec3;)"
                          "Lnet/minecraft/world/phys/Vec3;",
                      ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x01, 0xB0}, 1, 3);

    // No parameters at all, and that is the point of it: the hook reaches the
    // input through JNI rather than through the method's own locals, because
    // the frame on the return the append lands on has none of them live.
    cb.addCodedMethod("moveInput", "()V",
                      ACC_PUBLIC | ACC_STATIC, std::vector<u1>{0xB1}, 0, 0);

    cb.addCodedMethod("moveInputCapture", "(F)V",
                      ACC_PUBLIC | ACC_STATIC, std::vector<u1>{0xB1}, 1, 1);

    cb.addCodedMethod("gameTick", "()V",
                      ACC_PUBLIC | ACC_STATIC, std::vector<u1>{0xB1}, 0, 0);

    cb.addCodedMethod("useItemRayTraceYaw",
                      "(Lnet/minecraft/world/entity/player/Player;)F",
                      ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x0B, 0xAE}, 1, 1);

    cb.addCodedMethod("useItemRayTracePitch",
                      "(Lnet/minecraft/world/entity/player/Player;)F",
                      ACC_PUBLIC | ACC_STATIC,
                      std::vector<u1>{0x0B, 0xAE}, 1, 1);

    cb.addCodedMethod("packetOut",
                      "(Lnet/minecraft/network/Connection;"
                      "Lnet/minecraft/network/protocol/Packet;"
                      "Lnet/minecraft/network/PacketSendListener;)V",
                      ACC_PUBLIC | ACC_STATIC, std::vector<u1>{0xB1}, 1, 3);

    cb.addCodedMethod("packetIn",
                      "(Lnet/minecraft/network/protocol/Packet;Lnet/minecraft/network/PacketListener;)V",
                      ACC_PUBLIC | ACC_STATIC, std::vector<u1>{0xB1}, 1, 2);

    cb.addCodedMethod("positionItem",
                      "(Lnet/minecraft/client/multiplayer/ClientPacketListener;"
                      "Lnet/minecraft/network/protocol/Packet;)V",
                      ACC_PUBLIC | ACC_STATIC, std::vector<u1>{0xB1}, 1, 2);

    // The head guards. Both answer with Probe's own `frozen` field rather than
    // a constant, so one patched Probe runs both ways and the difference
    // between them is the guard and nothing else.
    const u2 probeFrozen = cb.fieldRef("Probe", "frozen", "Z");

    cb.addCodedMethod("freeze", "(LProbe;)Z", ACC_PUBLIC | ACC_STATIC,
                      Cat(GetStatic(probeFrozen), std::vector<u1>{0xAC}), 1, 1);

    cb.addCodedMethod("freezeArg", "(LProbe;ILjava/lang/Object;)Z",
                      ACC_PUBLIC | ACC_STATIC,
                      Cat(GetStatic(probeFrozen), std::vector<u1>{0xAC}), 1, 3);

    std::printf("== bridge ==\n");
    Expect(WriteFile(outDir / "Bridge.class", cb.build()), "write Bridge.class");

    // ---- Probe: every edit shape ------------------------------------------
    std::printf("== Probe (behavioural) ==\n");
    {
        std::vector<u1> bytes = ReadFile(probeIn);
        if (bytes.empty()) {
            std::fprintf(stderr, "cannot read %s\n", probeIn.string().c_str());
            return 2;
        }
        // The auditor is itself under test here: if it cannot accept a class
        // javac produced, its verdict on a rewritten one means nothing.
        ExpectAudit(bytes, "the unpatched Probe audits clean");

        ClassFileEditor ed(bytes.data(), bytes.size());

        // Resolved against Probe's own constant pool. A reference built for the
        // bridge class would be an index that means something else here, which
        // nothing in the rewriter checks and only the verifier would notice.
        const u2 markRef = ed.addMethodRef("Bridge", "mark", "()V");

        Expect(ed.hasMethod("useYaw", "()I"), "hasMethod finds useYaw");
        Expect(!ed.hasMethod("useYaw", "()J"), "hasMethod does not confuse descriptors");

        Expect(ed.redirectCalls("useYaw", "()I", "Probe", "yaw", "()I",
                                "Bridge", "hookYaw", "(LProbe;)I"),
               "redirect useYaw (single call, whole method)");
        Expect(ed.redirectCalls("useBoth", "()I", "Probe", "yaw", "()I",
                                "Bridge", "hookYaw", "(LProbe;)I", 0, 1),
               "redirect useBoth with limit 1");
        Expect(ed.redirectCalls("branchy", "(I)I", "Probe", "yaw", "()I",
                                "Bridge", "hookYaw", "(LProbe;)I"),
               "redirect branchy (StackMapTable)");
        Expect(ed.redirectCalls("tableswitch", "(I)I", "Probe", "yaw", "()I",
                                "Bridge", "hookYaw", "(LProbe;)I"),
               "redirect tableswitch");
        Expect(ed.redirectCalls("lookupswitch", "(I)I", "Probe", "yaw", "()I",
                                "Bridge", "hookYaw", "(LProbe;)I"),
               "redirect lookupswitch");
        Expect(ed.redirectCalls("guarded", "(I)I", "Probe", "yaw", "()I",
                                "Bridge", "hookYaw", "(LProbe;)I"),
               "redirect guarded (exception table)");
        Expect(ed.redirectCalls("loop", "(I)I", "Probe", "yaw", "()I",
                                "Bridge", "hookYaw", "(LProbe;)I"),
               "redirect loop (backward branch)");
        Expect(ed.redirectCalls("locals", "()Ljava/lang/String;", "Probe", "yaw", "()I",
                                "Bridge", "hookYaw", "(LProbe;)I"),
               "redirect locals (LocalVariableTable)");

        // A hook that does not return what the call returned. The rewrite has to
        // be refused: the class is left exactly as it was, and the edit reports
        // it did nothing.
        Expect(!ed.redirectCalls("usePitch", "()I", "Probe", "pitch", "()I",
                                 "Bridge", "sink", "(LProbe;)V"),
               "refuse a redirect whose stack effect differs");
        Expect(!ed.redirectCalls("usePitch", "()I", "Probe", "nonexistent", "()I",
                                 "Bridge", "hookYaw", "(LProbe;)I"),
               "refuse a redirect with no call site");
        Expect(!ed.redirectCalls("nosuchmethod", "()I", "Probe", "yaw", "()I",
                                 "Bridge", "hookYaw", "(LProbe;)I"),
               "refuse a redirect into a method that does not exist");

        // Insertions move code, so one at the head, one before a chosen call,
        // and one at the tail.
        Expect(ed.prependToMethodCode("loop", "(I)I", InvokeStatic(markRef), 0),
               "prepend to loop");
        // pitch() was deliberately left alone above, so it is still a call to
        // look for -- once a call has been redirected to the bridge, searching
        // for the original callee no longer finds it, and that is the intended
        // behaviour rather than an accident to work around.
        Expect(ed.insertBeforeCall("locals", "()Ljava/lang/String;", "Probe", "pitch",
                                   "()I", 0, InvokeStatic(markRef), 0),
               "insert before the first pitch() in locals");
        Expect(!ed.insertBeforeCall("locals", "()Ljava/lang/String;", "Probe", "pitch",
                                    "()I", 9, InvokeStatic(markRef), 0),
               "refuse an out-of-range ordinal");
        Expect(ed.insertBeforeCall("locals", "()Ljava/lang/String;", "Probe", "pitch",
                                   "()I", 0, InvokeStatic(markRef), 0, 0, true),
               "insert after the first pitch() in locals (after=true)");
        Expect(!ed.insertBeforeCall("locals", "()Ljava/lang/String;", "Probe", "yaw",
                                    "()I", 0, InvokeStatic(markRef), 0),
               "a redirected call is no longer found under its old name");
        // tail() has one way out, so a hook at the return covers every path.
        Expect(ed.appendBeforeLastReturn("tail", "(I)I", InvokeStatic(markRef), 0),
               "append before the return of tail");
        Expect(ed.appendBeforeLastReturn("tailVoid", "()V", InvokeStatic(markRef), 0),
               "append before the return of a void method");
        // branchy() has three. A TAIL hook there would fire on some callers and
        // silently not on others, so it has to be refused rather than half-work.
        Expect(!ed.appendBeforeLastReturn("branchy", "(I)I", InvokeStatic(markRef), 0),
               "refuse a TAIL insertion into a method with early returns");
        Expect(!ed.appendBeforeLastReturn("tail", "()I", InvokeStatic(markRef), 0),
               "refuse a TAIL insertion under the wrong descriptor");
        Expect(!ed.appendBeforeLastReturn("noSuchMethod", "()V", {}, 0),
               "refuse to append to a method that does not exist");

        // A three-byte invokestatic pads to four, so a switch shifted by the
        // insertion keeps its alignment and its length.
        Expect(ed.prependToMethodCode("tableswitch", "(I)I", InvokeStatic(markRef), 0),
               "prepend to tableswitch (padding)");

        std::vector<u1> patched = ed.serialize();
        ExpectAudit(patched, "the patched Probe audits clean");
        Expect(WriteFile(outDir / "Probe.patched.class", patched), "write Probe.patched.class");
    }

    // ---- the hook-call insertion, under the JVM's verifier ----------------
    //
    // The auditor above is structural: it knows a branch target when it sees
    // one and nothing at all about types. So it cannot tell `istore_2` from
    // `astore_2`, and those are exactly the two the emitter has to choose
    // between -- one of them is right and the other is a class that does not
    // load. The only thing that knows is the verifier, and the only way to hand
    // it the bytes is to give it a class it can define.
    //
    // Hence a second Probe, patched with nothing but these three insertions, so
    // a failure here is a failure of the emitter rather than of everything else
    // the file does.
    std::printf("== the hook-call insertion (behavioural) ==\n");
    {
        std::vector<u1> bytes = ReadFile(probeIn);
        if (bytes.empty()) {
            std::fprintf(stderr, "cannot read %s\n", probeIn.string().c_str());
            return 2;
        }

        ClassFileEditor ed(bytes.data(), bytes.size());

        struct Carry {
            const char* method;
            const char* methodDesc;
            const char* hook;
            const char* hookDesc;
        };
        const Carry kCarries[] = {
            {"carryObject", "(Ljava/lang/Object;Ljava/lang/Object;)V",
             "routeObject", "(Ljava/lang/Object;Ljava/lang/Object;"
                            "Ljava/lang/Object;)Ljava/lang/Object;"},
            {"carryFloat", "(Ljava/lang/Object;F)V",
             "routeFloat", "(Ljava/lang/Object;Ljava/lang/Object;F)F"},
            {"carryInt", "(Ljava/lang/Object;I)V",
             "routeInt", "(Ljava/lang/Object;Ljava/lang/Object;I)I"},
        };

        bool all = true;
        for (const Carry& c : kCarries) {
            all &= ed.insertHookCall(c.method, c.methodDesc, "Probe", "step", "()V",
                                     /*calleeIsStatic=*/false, 0, "Bridge",
                                     c.hook, c.hookDesc, /*argCount=*/3,
                                     /*storeSlot=*/2);
        }
        Expect(all, "insert a hook call into carryObject/carryFloat/carryInt");

        std::vector<u1> patched = ed.serialize();
        ExpectAudit(patched, "the carry Probe audits clean");
        Expect(WriteFile(outDir / "Probe.carried.class", patched),
               "write Probe.carried.class");
    }

    // ---- the cancellable head guard, under the JVM's verifier -------------
    //
    // The guard jumps *into* an instruction -- the final return -- rather than
    // past one, so the question it raises is not whether the bytes are well
    // formed but whether a frame with the receiver and the arguments live is
    // allowed to reach a frame whose locals were computed for a program point
    // partway through the method. Nothing the emitter can check answers that;
    // the verifier does, when the class is defined.
    std::printf("== the cancellable head guard (behavioural) ==\n");
    {
        std::vector<u1> bytes = ReadFile(probeIn);
        if (bytes.empty()) {
            std::fprintf(stderr, "cannot read %s\n", probeIn.string().c_str());
            return 2;
        }

        ClassFileEditor ed(bytes.data(), bytes.size());

        Expect(ed.insertCancelGuard("guardedTick", "([I)V", "Bridge", "freeze",
                                    "(LProbe;)Z", 1),
               "guard the head of guardedTick");
        Expect(ed.insertCancelGuard("guardedTickArg", "(ILjava/lang/Object;)V",
                                    "Bridge", "freezeArg",
                                    "(LProbe;ILjava/lang/Object;)Z", 3),
               "guard the head of guardedTickArg (three locals loaded)");

        // The shape the verifier will not take, and the reason the guard is
        // refused rather than written: the frame on the return names `box`,
        // which lives only inside the method, so the branch from offset 0 --
        // where that slot is `top` -- is
        //   Type top (current frame, locals[1]) is not assignable to '[I'
        // and the whole class fails to load.
        Expect(!ed.insertCancelGuard("guardedTickBusy", "()V", "Bridge", "freeze",
                                     "(LProbe;)Z", 1),
               "refuse a guard over a return frame with a local it cannot have");

        // And the refusals. A guard has to land on the method's last return, so
        // a method that returns a value, or one whose return nothing branches
        // to, cannot take one -- and a class that got either anyway would fail
        // to load rather than misbehave, which is the whole reason these are
        // checked for rather than attempted.
        Expect(!ed.insertCancelGuard("tail", "(I)I", "Bridge", "freeze",
                                     "(LProbe;)Z", 1),
               "refuse a guard on a method that returns a value");
        Expect(!ed.insertCancelGuard("tailVoid", "()V", "Bridge", "freeze",
                                     "(LProbe;)Z", 1),
               "refuse a guard whose return is not a branch target");
        Expect(!ed.insertCancelGuard("guardedTick", "()V", "Bridge", "freezeArg",
                                     "(LProbe;ILjava/lang/Object;)Z", 3),
               "refuse a guard whose arity does not match its hook");
        Expect(!ed.insertCancelGuard("guardedTick", "()V", "Bridge", "sink",
                                     "(LProbe;)V", 1),
               "refuse a guard whose hook does not answer yes or no");

        std::vector<u1> patched = ed.serialize();
        ExpectAudit(patched, "the guarded Probe audits clean");
        Expect(WriteFile(outDir / "Probe.guarded.class", patched),
               "write Probe.guarded.class");
    }

    // ---- game classes: the production edits -------------------------------
    //
    // The rules come from `minecraft/hooks/Edits.cpp` -- the same table the transformer
    // walks inside the game -- so a row added there is a row this runs, and
    // there is no second copy to fall behind. What is deliberately *not* shared
    // is the bridge side; see kBridgeHooks.
    std::printf("== game classes (the production edits) ==\n");

    struct HookDecl { const char* name; const char* desc; };
    const HookDecl kBridgeHooks[] = {
        {"strafeYaw",  "(Lnet/minecraft/world/entity/Entity;)F"},
        {"viewVector", "(Lnet/minecraft/world/entity/Entity;FF)"
                       "Lnet/minecraft/world/phys/Vec3;"},
        {"jumpYaw",    "(Lnet/minecraft/world/entity/Entity;)F"},
        {"bodyYaw",    "(Lnet/minecraft/world/entity/Entity;)F"},
        {"fallFlyingPitch", "(Lnet/minecraft/world/entity/Entity;)F"},
        {"attackYaw",  "(Lnet/minecraft/world/entity/Entity;)F"},
        {"usingItem",  "(Lnet/minecraft/world/entity/Entity;)Z"},
        {"playerTick", "(Lnet/minecraft/client/player/LocalPlayer;)Z"},
        {"packetOut",  "(Lnet/minecraft/network/Connection;"
                       "Lnet/minecraft/network/protocol/Packet;"
                       "Lnet/minecraft/network/PacketSendListener;)V"},
        {"packetIn", "(Lnet/minecraft/network/protocol/Packet;Lnet/minecraft/network/PacketListener;)V"},
        {"stuckInBlock",
                       "(Lnet/minecraft/world/entity/Entity;"
                       "Lnet/minecraft/world/level/block/state/BlockState;"
                       "Lnet/minecraft/world/phys/Vec3;)"
                       "Lnet/minecraft/world/phys/Vec3;"},
        {"moveInput",  "()V"},
        {"moveInputCapture", "(F)V"},
        {"gameTick",   "()V"},
        {"beginMotion", "(Lnet/minecraft/client/player/LocalPlayer;)V"},
        {"endMotion",  "(Lnet/minecraft/client/player/LocalPlayer;)V"},
        {"motionYaw",  "(Lnet/minecraft/world/entity/Entity;)F"},
        {"motionPitch", "(Lnet/minecraft/world/entity/Entity;)F"},
        {"motionOnGround", "(Lnet/minecraft/world/entity/Entity;)Z"},
        {"useItemRayTraceYaw",
                       "(Lnet/minecraft/world/entity/player/Player;)F"},
        {"useItemRayTracePitch",
                       "(Lnet/minecraft/world/entity/player/Player;)F"},
        {"positionItem", "(Lnet/minecraft/client/multiplayer/ClientPacketListener;"
                         "Lnet/minecraft/network/protocol/Packet;)V"},
        // LivingEntityRenderer is not extracted here, so these three are
        // declared and never asked for. Spelled out anyway: the day someone
        // drops that class into `classes/` the row starts being checked, and a
        // declaration that is only missing because it was never needed reads
        // exactly like one that has drifted.
        {"renderEntity", "(Lnet/minecraft/world/entity/LivingEntity;)V"},
        {"renderYaw",  "(FFF)F"},
        {"renderPitch", "(FFF)F"},
    };
    auto bridgeDeclares = [&](const char* name, const char* desc) {
        for (const HookDecl& h : kBridgeHooks) {
            if (std::strcmp(h.name, name) == 0 && std::strcmp(h.desc, desc) == 0)
                return true;
        }
        return false;
    };

    int editCount = 0;
    const nova::hook::ClassEdits* allEdits = nova::hook::AllEdits(editCount);
    for (int ci = 0; ci < editCount; ++ci) {
        const nova::hook::ClassEdits& ce = allEdits[ci];
        std::string fileName = ce.internalName;
        std::replace(fileName.begin(), fileName.end(), '/', '.');
        std::vector<u1> bytes = ReadFile(clsDir / (fileName + ".class"));
        if (bytes.empty()) {
            std::printf("  SKIP %s (class not extracted)\n", fileName.c_str());
            continue;
        }
        // The auditor first has to accept the class as it came out of the jar,
        // or its verdict on the edited one says nothing about the edit.
        ExpectAudit(bytes, "unpatched " + fileName + " audits clean");

        // All of a class's rules into one editor, which is how the transformer
        // does it: the failure worth catching is the second rule landing on
        // offsets the first one disturbed, and two editors never see it.
        ClassFileEditor ed(bytes.data(), bytes.size());
        for (int ri = 0; ri < ce.count; ++ri) {
            const nova::hook::Redirect& r = ce.redirects[ri];
            const std::string what = std::string(ce.internalName) + "." + r.method +
                                     " -> Bridge." + r.hook;

            // The one check the rewriter cannot make on its own: it writes the
            // name and the descriptor into the constant pool without ever
            // asking whether the bridge has such a method. This is that
            // question, put to a list written down separately.
            Expect(bridgeDeclares(r.hook, r.hookDesc), "the bridge declares " + what);

            u4 before = 0;
            const bool gotBefore =
                audit::CodeLength(ed.serialize(), r.method, r.methodDesc, before);

            Expect(ed.redirectCalls(r.method, r.methodDesc, r.calleeOwner,
                                    r.calleeName, r.calleeDesc, "Bridge",
                                    r.hook, r.hookDesc, 0, r.limit),
                   "redirect " + what);

            // A redirect substitutes three bytes in place. The class file grows
            // by the constant pool entries the new reference needs, but the
            // method's code array must not: a length that changed means an
            // offset moved, and nothing downstream would notice until load time.
            std::vector<u1> out = ed.serialize();
            u4 after = 0;
            const bool gotAfter =
                audit::CodeLength(out, r.method, r.methodDesc, after);
            Expect(gotBefore && gotAfter && before == after,
                   "  code length unchanged, " + std::to_string(before) + " -> " +
                       std::to_string(after) + " (" + what + ")");
            ExpectAudit(out, "audit " + what);

            std::string stem = fileName;
            std::replace(stem.begin(), stem.end(), '.', '_');
            WriteFile(outDir / (stem + "_" + r.method + "_" + r.hook + ".class"), out);
        }

        // Then the insertions, in the same order the transformer uses: the
        // redirects substitute three bytes each and move nothing, so they go
        // first and the call sites an insertion is looking for are still where
        // the pool says they are.
        for (int ii = 0; ii < ce.insertionCount; ++ii) {
            const nova::hook::Insertion& in = ce.insertions[ii];
            const std::string what = std::string(ce.internalName) + "." + in.method +
                                     " += Bridge." + in.hook;

            Expect(bridgeDeclares(in.hook, in.hookDesc), "the bridge declares " + what);

            u4 before = 0;
            const bool gotBefore =
                audit::CodeLength(ed.serialize(), in.method, in.methodDesc, before);

            Expect(ed.insertHookCall(in.method, in.methodDesc, in.calleeOwner,
                                     in.calleeName, in.calleeDesc,
                                     in.calleeIsStatic, in.ordinal, "Bridge",
                                     in.hook, in.hookDesc, in.argCount,
                                     in.storeSlot),
                   "insert " + what);

            // The opposite of the redirect check, and for the same reason: an
            // insertion is *supposed* to move code, so a length that did not
            // change means nothing was emitted. The exact figure is not asserted
            // -- the audit below is what says the result is well formed -- but a
            // growth of zero is the one answer that is certainly wrong.
            std::vector<u1> out = ed.serialize();
            u4 after = 0;
            const bool gotAfter =
                audit::CodeLength(out, in.method, in.methodDesc, after);
            Expect(gotBefore && gotAfter && after > before,
                   "  code grew, " + std::to_string(before) + " -> " +
                       std::to_string(after) + " (" + what + ")");
            ExpectAudit(out, "audit " + what);

            std::string stem = fileName;
            std::replace(stem.begin(), stem.end(), '.', '_');
            WriteFile(outDir / (stem + "_" + in.method + "_" + in.hook + ".class"), out);
        }

        // Then the guards, last, for the same reason the insertions came after
        // the redirects: this is the only rule that prepends to a method, and
        // every rule after it in the same editor would be working on offsets it
        // moved. The order here is the transformer's.
        for (int gi = 0; gi < ce.guardCount; ++gi) {
            const nova::hook::Guard& g = ce.guards[gi];
            const std::string what = std::string(ce.internalName) + "." + g.method +
                                     " guarded by Bridge." + g.hook;

            Expect(bridgeDeclares(g.hook, g.hookDesc), "the bridge declares " + what);

            u4 before = 0;
            const bool gotBefore =
                audit::CodeLength(ed.serialize(), g.method, g.methodDesc, before);

            Expect(ed.insertCancelGuard(g.method, g.methodDesc, "Bridge",
                                        g.hook, g.hookDesc, g.argCount),
                   "guard " + what);

            std::vector<u1> out = ed.serialize();
            u4 after = 0;
            const bool gotAfter =
                audit::CodeLength(out, g.method, g.methodDesc, after);
            Expect(gotBefore && gotAfter && after > before,
                   "  code grew, " + std::to_string(before) + " -> " +
                       std::to_string(after) + " (" + what + ")");
            ExpectAudit(out, "audit " + what);

            std::string stem = fileName;
            std::replace(stem.begin(), stem.end(), '.', '_');
            WriteFile(outDir / (stem + "_" + g.method + "_" + g.hook + ".class"), out);
        }

        // The appends come last, which is the transformer's order: an append at
        // the head moves every offset in the method, so anything after it in the
        // same editor is working on a method it has already disturbed.
        for (int ai = 0; ai < ce.appendCount; ++ai) {
            const nova::hook::Append& a = ce.appends[ai];
            const std::string what = std::string(ce.internalName) + "." + a.method +
                                     " += Bridge." + a.hook;

            Expect(bridgeDeclares(a.hook, a.hookDesc), "the bridge declares " + what);

            const u2 ref = ed.addMethodRef("Bridge", a.hook, a.hookDesc);
            Expect(ref != 0, std::string("the pool takes Bridge.") + a.hook +
                                 " (" + what + ")");
            if (ref == 0) continue;

            const std::vector<u1> code =
                nova::hook::AppendCode(ref, a.loads, a.loadCount);

            u4 before = 0;
            const bool gotBefore =
                audit::CodeLength(ed.serialize(), a.method, a.methodDesc, before);

            const bool placed =
                a.atHead ? ed.prependToMethodCode(a.method, a.methodDesc, code,
                                                  u2(a.loadCount))
                         : ed.appendBeforeLastReturn(a.method, a.methodDesc, code,
                                                     u2(a.loadCount));
            Expect(placed, "append " + what);

            std::vector<u1> out = ed.serialize();
            u4 after = 0;
            const bool gotAfter =
                audit::CodeLength(out, a.method, a.methodDesc, after);
            Expect(gotBefore && gotAfter && after > before,
                   "  code grew, " + std::to_string(before) + " -> " +
                       std::to_string(after) + " (" + what + ")");
            ExpectAudit(out, "audit " + what);

            std::string stem = fileName;
            std::replace(stem.begin(), stem.end(), '.', '_');
            WriteFile(outDir / (stem + "_" + a.method + "_" + a.hook + ".class"), out);
        }

        // Every rule for this class in one file, which is the only shape the
        // game ever sees: the transformer hands the JVM one class, not one per
        // rule, and a rule that is clean on its own can still leave a class
        // that is not. Written last, so it is the state all of them produced.
        std::vector<u1> full = ed.serialize();
        ExpectAudit(full, "audit " + fileName + " with every rule applied");
        // Dotted name in its own directory: the reader on the other side has to
        // hand this name back to defineClass, and a name it has to reconstruct
        // from an escaped file name is a name it can get wrong.
        WriteFile(outDir / "full" / (fileName + ".class"), full);
    }

    // The two insertion primitives, on a real method, with a real stack map to
    // fix up. The table-driven pass above already ran the production insertion;
    // what these add is the other two shapes -- a raw byte sequence rather than
    // a described one, at the tail rather than in front of a call -- because
    // those are what the next edit to need them will build on.
    {
        std::vector<u1> bytes = ReadFile(clsDir / "net.minecraft.world.entity.Entity.class");
        if (!bytes.empty()) {
            const char* kStuckDesc =
                "(Lnet/minecraft/world/level/block/state/BlockState;"
                "Lnet/minecraft/world/phys/Vec3;)V";

            // aload_0; invokestatic Bridge.stuck(Entity)V -- four bytes, so the
            // padding adds nothing.
            ClassFileEditor ed2(bytes.data(), bytes.size());
            const u2 stuckRef = ed2.addMethodRef("Bridge", "stuck", kEntityVoid);
            Expect(ed2.appendBeforeLastReturn(
                       "m_7601_", kStuckDesc,
                       Cat(std::vector<u1>{0x2A}, InvokeStatic(stuckRef)), 1),
                   "append stuck hook before the last return of m_7601_");
            ExpectAudit(ed2.serialize(), "audit the m_7601_ tail insertion");
            WriteFile(outDir / "Entity_stuck.class", ed2.serialize());

            // Five bytes, so the padding has to make it eight and keep whatever
            // follows aligned. The trailing byte is a nop rather than a real
            // instruction: this one is only audited, and an extra stack item
            // before `return` would be a verification failure the auditor does
            // not model.
            ClassFileEditor ed3(bytes.data(), bytes.size());
            const u2 stuckRef3 = ed3.addMethodRef("Bridge", "stuck", kEntityVoid);
            Expect(ed3.appendBeforeLastReturn(
                       "m_7601_", kStuckDesc,
                       Cat(std::vector<u1>{0x2A}, InvokeStatic(stuckRef3),
                           std::vector<u1>{0x00}), 1),
                   "append a five-byte insertion (needs padding)");
            ExpectAudit(ed3.serialize(), "audit the padded insertion");
            WriteFile(outDir / "Entity_stuck_padded.class", ed3.serialize());

            // The hook-call shape, spelled out by hand, so the test does not
            // depend on the emitter being right about it. Three locals in, one
            // store back, seven bytes padded to eight -- the exact sequence
            // verified against the real method's disassembly.
            ClassFileEditor ed4(bytes.data(), bytes.size());
            Expect(ed4.insertHookCall(
                       "m_7601_", kStuckDesc, "net/minecraft/world/entity/Entity",
                       "m_183634_", "()V", /*calleeIsStatic=*/false, 0,
                       "Bridge", "stuck",
                       "(Lnet/minecraft/world/entity/Entity;"
                       "Lnet/minecraft/world/level/block/state/BlockState;"
                       "Lnet/minecraft/world/phys/Vec3;)"
                       "Lnet/minecraft/world/phys/Vec3;",
                       /*argCount=*/3, /*storeSlot=*/2),
                   "insertHookCall emits the stuck hook by hand");
            // Seven bytes -- aload_0, aload_1, aload_2, invokestatic, astore_2
            // -- padded to eight, so exactly eight more than the ten the method
            // already had. The number is checked because the emitter derives the
            // whole sequence from two descriptors, and "it compiled and the
            // audit passed" would not notice it choosing a longer load form.
            // Measured into locals first, not read inside the Expect: the two
            // calls have to run before the message is built out of what they
            // wrote, and argument evaluation order is not something to rely on.
            u4 before = 0, grew = 0;
            const bool gotBefore = audit::CodeLength(bytes, "m_7601_", kStuckDesc, before);
            const bool gotAfter  = audit::CodeLength(ed4.serialize(), "m_7601_",
                                                     kStuckDesc, grew);
            Expect(gotBefore && gotAfter && grew == before + 8,
                   "  inserted exactly eight bytes (7 padded), " +
                       std::to_string(before) + " -> " + std::to_string(grew));
            ExpectAudit(ed4.serialize(), "audit the hand-built stuck insertion");
            WriteFile(outDir / "Entity_stuck_inserted.class", ed4.serialize());

            // And the two combinations the emitter has to refuse. Both produce
            // bytecode the verifier rejects at load, which is a whole class
            // failing to load rather than one event not firing -- so they are
            // refused here instead.
            ClassFileEditor ed5(bytes.data(), bytes.size());
            Expect(!ed5.insertHookCall(
                       "m_7601_", kStuckDesc, "net/minecraft/world/entity/Entity",
                       "m_183634_", "()V", false, 0, "Bridge", "stuck",
                       "(Lnet/minecraft/world/entity/Entity;"
                       "Lnet/minecraft/world/level/block/state/BlockState;"
                       "Lnet/minecraft/world/phys/Vec3;)"
                       "Lnet/minecraft/world/phys/Vec3;",
                       /*argCount=*/3, /*storeSlot=*/-1),
                   "refuse a value-returning hook whose result is dropped");
            ClassFileEditor ed6(bytes.data(), bytes.size());
            Expect(!ed6.insertHookCall(
                       "m_7601_", kStuckDesc, "net/minecraft/world/entity/Entity",
                       "m_183634_", "()V", false, 0, "Bridge", "stuck",
                       /*a void hook*/ "(Lnet/minecraft/world/entity/Entity;)V",
                       /*argCount=*/3, /*storeSlot=*/2),
                   "refuse a hook whose parameters are not the locals loaded");
        } else {
            std::printf("  SKIP Entity tail insertion (class not extracted)\n");
        }
    }

    // The stack map frame the walker used to step over.
    //
    // Type 247 is the extended spelling of "the locals are unchanged and one
    // item is on the stack", and it carries a verification type just like the
    // one-byte 64..127 range does. Read the header and not the item and the
    // next frame is parsed from a byte that is still frame data -- which is
    // the failure mode worth a test of its own, because what comes out the far
    // side is a table of the right length naming plausible offsets, and only a
    // reader that is right for its own reasons can tell. Minecraft.m_91383_
    // carries one, so the real class is the fixture.
    {
        std::vector<u1> bytes = ReadFile(clsDir / "net.minecraft.client.Minecraft.class");
        if (!bytes.empty()) {
            // What is asserted is the property, not a symptom: every frame in
            // the method moves by the four the insertion added, that shift is
            // carried entirely by the delta on the first frame, and the table
            // is otherwise the bytes it was. A walk that loses the type info
            // behind a 247 frame reads what follows one byte at a time, and
            // one byte in, one byte out is what a narrow frame is -- so on
            // this table the misread bytes happen to be re-emitted verbatim
            // and this check would not see it. It is here for the walk that
            // gets 247 wrong in a way that does move a byte, and for the next
            // edit to this table.
            std::vector<u1> before;
            Expect(audit::StackMapBody(bytes, "m_91383_", "(Z)V", before),
                   "m_91383_ has a StackMapTable");

            ClassFileEditor ed(bytes.data(), bytes.size());
            const u2 markRef = ed.addMethodRef("Bridge", "mark", "()V");
            Expect(ed.prependToMethodCode("m_91383_", "(Z)V", InvokeStatic(markRef), 0),
                   "prepend to m_91383_ (a method whose stack map has a 247 frame)");
            ExpectAudit(ed.serialize(), "audit the 247-frame insertion");

            std::vector<u1> after;
            Expect(audit::StackMapBody(ed.serialize(), "m_91383_", "(Z)V", after),
                   "the patched m_91383_ has a StackMapTable");
            int differing = 0;
            for (size_t i = 0; i < before.size() && i < after.size(); ++i) {
                if (before[i] != after[i]) ++differing;
            }
            Expect(!before.empty() && before.size() == after.size() && differing == 1,
                   "  the table kept its length and changed only the first delta (" +
                       std::to_string(before.size()) + " -> " +
                       std::to_string(after.size()) + ", " +
                       std::to_string(differing) + " byte" +
                       (differing == 1 ? "" : "s") + " differ)");

            WriteFile(outDir / "Minecraft_247.class", ed.serialize());
        } else {
            std::printf("  SKIP m_91383_ stack map (class not extracted)\n");
        }
    }

    std::printf("%s (%d failure%s)\n", g_failures ? "FAILED" : "PASSED",
                g_failures, g_failures == 1 ? "" : "s");
    return g_failures ? 1 : 0;
}
