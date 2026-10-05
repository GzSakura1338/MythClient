// Runs the classes patch_test.cpp produced.
//
// Defining a class is what makes the JVM's bytecode verifier run over it, and
// this is a non-boot loader, so there is no flag to remember: a StackMapTable
// the rewriter got wrong comes back as a VerifyError out of defineClass. That is
// the authority; the C++ side's own audit is only there for the Minecraft
// classes, which cannot be loaded outside the game.
//
// Bridge.mark() calls Verify.bump() so that an inserted instruction is not just
// survivable but observably executed.

import java.io.IOException;
import java.io.PrintWriter;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.URL;
import java.net.URLClassLoader;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

public class Verify {

    // Called from the bridge class's mark(), which the rewriter inserts into
    // Probe: the counter is how the test sees an insertion run.
    public static int bumps = 0;
    public static void bump() { bumps++; }

    private static int failures = 0;

    private static void check(String what, boolean ok) {
        System.out.printf("  %s %s%n", ok ? "ok  " : "FAIL", what);
        if (!ok) failures++;
    }

    private static void eq(String what, Object want, Object got) {
        boolean ok = want == null ? got == null : want.equals(got);
        check(what + (ok ? "" : "  [want " + want + ", got " + got + "]"), ok);
    }

    static final class Loader extends ClassLoader {
        Loader(ClassLoader parent) { super(parent); }
        Class<?> define(String name, byte[] bytes) {
            return defineClass(name, bytes, 0, bytes.length);
        }
    }

    private static Object call(Object target, String name, Class<?>[] sig, Object... args)
            throws Exception {
        Method m = target.getClass().getMethod(name, sig);
        return m.invoke(target, args);
    }

    private static int callI(Object target, String name, Class<?>[] sig, Object... args)
            throws Exception {
        return (Integer) call(target, name, sig, args);
    }

    public static void main(String[] args) throws Exception {
        final Path out = Paths.get(args[0]);
        final Path srgJar = args.length > 1 ? Paths.get(args[1]) : null;

        // The patched Probe must come from the loader below and nowhere else. If
        // it were also reachable from the application class path this whole test
        // would pass while running the unpatched class.
        try {
            ClassLoader.getSystemClassLoader().loadClass("Probe");
            check("Probe is not visible to the application class loader", false);
            return;
        } catch (ClassNotFoundException expected) {
            check("Probe is not visible to the application class loader", true);
        }

        final Loader loader = new Loader(Verify.class.getClassLoader());

        System.out.println("== defining classes (the verifier runs here) ==");
        Class<?> bridge = loader.define("Bridge",
                Files.readAllBytes(out.resolve("Bridge.class")));
        check("Bridge defined", bridge != null);

        byte[] probeBytes = Files.readAllBytes(out.resolve("Probe.patched.class"));
        Class<?> probe = loader.define("Probe", probeBytes);
        check("Probe.patched defined (verifier accepted it)", probe != null);
        eq("Probe came from the test loader", loader, probe.getClassLoader());

        if (srgJar != null) {
            checkAsmAgrees("Probe.patched", probeBytes, new URL[]{srgJar.toUri().toURL()});
        }

        System.out.println("== behaviour ==");
        Object p = probe.getDeclaredConstructor().newInstance();

        eq("useYaw (one call, redirected)", 100,
                callI(p, "useYaw", new Class<?>[0]));
        eq("useBoth (limit 1: yaw hooked, pitch not)", 9911,
                callI(p, "useBoth", new Class<?>[0]));
        eq("usePitch (a refused redirect left it alone)", 11,
                callI(p, "usePitch", new Class<?>[0]));

        eq("branchy(+1)", 99, callI(p, "branchy", new Class[]{int.class}, 1));
        eq("branchy(-1)", -99, callI(p, "branchy", new Class[]{int.class}, -1));
        eq("branchy(0) does not call the hook", 0,
                callI(p, "branchy", new Class[]{int.class}, 0));

        eq("tableswitch case 1", 109, callI(p, "tableswitch", new Class[]{int.class}, 1));
        eq("tableswitch case 2", 119, callI(p, "tableswitch", new Class[]{int.class}, 2));
        eq("tableswitch case 3", 129, callI(p, "tableswitch", new Class[]{int.class}, 3));
        eq("tableswitch default", 99, callI(p, "tableswitch", new Class[]{int.class}, 9));

        eq("lookupswitch case 100", 98, callI(p, "lookupswitch", new Class[]{int.class}, 100));
        eq("lookupswitch case 555", 97, callI(p, "lookupswitch", new Class[]{int.class}, 555));
        eq("lookupswitch default", 99, callI(p, "lookupswitch", new Class[]{int.class}, 7));

        eq("guarded (falls through the try)", 104,
                callI(p, "guarded", new Class[]{int.class}, 5));
        eq("guarded (the catch still catches)", -1,
                callI(p, "guarded", new Class[]{int.class}, 0));

        eq("loop(4) (prepended hook, then four hooked reads)", 396,
                callI(p, "loop", new Class[]{int.class}, 4));
        eq("loop(0) (the prepended hook still runs)", 0,
                callI(p, "loop", new Class[]{int.class}, 0));

        eq("locals (locals span the insertion)", "99:11",
                call(p, "locals", new Class<?>[0]));

        // ---- the hook-call insertion --------------------------------------
        //
        // A second Probe, patched with nothing but the three insertions. The
        // point is not that the method computes the right number: it is that
        // the JVM verifier accepted the store opcode the emitter picked. Get
        // that wrong -- an istore where the result is a reference -- and the
        // class does not load at all, which is the failure this whole file
        // exists to catch before the game sees it.
        // A loader of its own, and that is not tidiness: the second Probe is
        // still called `Probe` inside the class file, and one loader will not
        // hold two classes under one name. It gets its own Bridge for the same
        // reason -- a class defined here resolves its own references against
        // this loader, and the parent has neither.
        System.out.println("== the hook-call insertion ==");
        byte[] carriedBytes = Files.readAllBytes(out.resolve("Probe.carried.class"));
        final Loader carriedLoader = new Loader(Verify.class.getClassLoader());
        carriedLoader.define("Bridge", Files.readAllBytes(out.resolve("Bridge.class")));
        Class<?> carried = carriedLoader.define("Probe", carriedBytes);
        check("Probe.carried defined (verifier accepted the emitted opcodes)",
                carried != null);

        Object cp = carried.getDeclaredConstructor().newInstance();
        Object sentinel = new Object();

        carried.getMethod("carryObject", Object.class, Object.class)
                .invoke(cp, "ignored", sentinel);
        eq("carryObject: the hook's answer is what the field got",
                cp, carried.getField("carriedObject").get(cp));

        // Each hook answers with something the caller did not pass, so a field
        // holding the argument is the insertion having done nothing rather than
        // a coincidence.
        carried.getMethod("carryFloat", Object.class, float.class)
                .invoke(cp, "ignored", 1.5f);
        eq("carryFloat: the hook's answer is what the field got",
                2.5f, carried.getField("carriedFloat").getFloat(cp));

        carried.getMethod("carryInt", Object.class, int.class)
                .invoke(cp, "ignored", 42);
        eq("carryInt: the hook's answer is what the field got",
                43, carried.getField("carriedInt").getInt(cp));

        // ---- the cancellable head guard -----------------------------------
        //
        // The one edit whose correctness the verifier alone can settle: the
        // guard branches from offset 0, where only the receiver and the
        // arguments have a type, into the method's final return -- a frame
        // computed for a program point further in, with a different set of live
        // locals. Defining the class is the question being asked. What follows
        // it is whether the branch goes where it was aimed.
        System.out.println("== the cancellable head guard ==");
        byte[] guardedBytes = Files.readAllBytes(out.resolve("Probe.guarded.class"));
        final Loader guardedLoader = new Loader(Verify.class.getClassLoader());
        guardedLoader.define("Bridge", Files.readAllBytes(out.resolve("Bridge.class")));
        Class<?> guarded = guardedLoader.define("Probe", guardedBytes);
        check("Probe.guarded defined (verifier accepted the head guard)", guarded != null);

        Object gp = guarded.getDeclaredConstructor().newInstance();
        java.lang.reflect.Field frozen = guarded.getField("frozen");
        java.lang.reflect.Field touched = guarded.getField("touched");

        // Both ways round, because a guard that refused the class and a guard
        // that cancelled every call would look the same from one side.
        int[] box = new int[] { 5 };

        frozen.setBoolean(null, false);
        guarded.getMethod("guardedTick", int[].class).invoke(gp, (Object) box);
        eq("guardedTick runs its body when the hook answers no", 18, touched.getInt(gp));

        frozen.setBoolean(null, true);
        touched.setInt(gp, 0);
        guarded.getMethod("guardedTick", int[].class).invoke(gp, (Object) box);
        eq("guardedTick does nothing when the hook answers yes", 0, touched.getInt(gp));

        frozen.setBoolean(null, false);
        touched.setInt(gp, 0);
        guarded.getMethod("guardedTickArg", int.class, Object.class).invoke(gp, 2, "ab");
        eq("guardedTickArg runs its body when the hook answers no", 5,
                touched.getInt(gp));

        frozen.setBoolean(null, true);
        touched.setInt(gp, 0);
        guarded.getMethod("guardedTickArg", int.class, Object.class).invoke(gp, 2, "ab");
        eq("guardedTickArg does nothing when the hook answers yes", 0,
                touched.getInt(gp));

        // And the two methods the guard was refused on are untouched. A
        // refusal that had written half the sequence before giving up would
        // leave a method that still runs and returns the wrong thing, which is
        // worse than one that fails to load only because it is quieter.
        eq("tailVoid, which the guard was refused, still runs", null,
                guarded.getMethod("tailVoid").invoke(gp));
        eq("tail, which a guard was refused, still returns what it did", 10,
                callI(gp, "tail", new Class[]{int.class}, 3));

        System.out.println("== the insertions actually ran ==");
        bumps = 0;
        call(p, "loop", new Class[]{int.class}, 3);
        eq("loop(3) called mark() once", 1, bumps);

        bumps = 0;
        call(p, "locals", new Class<?>[0]);
        eq("locals() called mark() twice (before and after the call)", 2, bumps);

        eq("tail (one way out, so the hook covers it)", 10,
                callI(p, "tail", new Class[]{int.class}, 3));

        bumps = 0;
        call(p, "tail", new Class[]{int.class}, 3);
        eq("tail() called mark() once (tail insertion)", 1, bumps);

        bumps = 0;
        call(p, "tailVoid", new Class<?>[0]);
        eq("tailVoid() called mark() once", 1, bumps);

        // branchy() has three exits, so the rewriter refuses a TAIL insertion
        // into it -- and the class it refused to touch still behaves.
        bumps = 0;
        call(p, "branchy", new Class[]{int.class}, 1);
        eq("branchy is untouched by the refused TAIL insertion", 0, bumps);

        call(p, "tableswitch", new Class[]{int.class}, 2);
        eq("tableswitch still switches after a prepend", 119,
                callI(p, "tableswitch", new Class[]{int.class}, 2));

        // ---- the real classes: structure only -----------------------------
        //
        // Their edits have already been audited on the C++ side, against every
        // branch, switch, exception range and stack map frame the rewrite moved.
        // ASM could go further -- a full data-flow pass -- but the Minecraft
        // classes reference half of Forge, and until every one of those jars is
        // on the class path all it reports is which type it could not find.
        // Opt in with ASM_GAME=1 when the class path is worth building up.
        System.out.println("== Minecraft classes (structural audit only) ==");
        int n = 0;
        for (Object[] pair : patchedClasses(out)) {
            String name = (String) pair[0];
            // The two Probes are this harness's own classes, already defined and
            // already run by the time this loop starts; only the Minecraft ones
            // are structure-only.
            if (name.equals("Probe.patched.class")) continue;
            if (name.equals("Probe.carried.class")) continue;
            if (name.equals("Probe.guarded.class")) continue;
            n++;
            if (System.getenv("ASM_GAME") != null && srgJar != null) {
                checkAsmAgrees(name, Files.readAllBytes((Path) pair[1]),
                        new URL[]{srgJar.toUri().toURL()});
            }
        }
        System.out.printf("  ( %d patched Minecraft class%s, ASM pass %s )%n",
                n, n == 1 ? "" : "es",
                System.getenv("ASM_GAME") != null ? "on" : "off");

        // ---- the real classes: the JVM verifier itself ---------------------
        //
        // The structural audit above reads the bytes; this one hands them to
        // defineClass and lets the verifier that the game will use rule on
        // them. Everything the Minecraft classes reference is in the libraries
        // directory, so the types resolve and a frame the rewriter got wrong
        // comes back as a VerifyError -- which is exactly the failure the game
        // reports, and the one no amount of reading the class back can find.
        //
        // A VerifyError is the bytecode's fault and counts. Anything else --
        // a Forge-only type this class path does not carry -- is the harness's
        // limit and is only reported.
        if (srgJar != null) defineGameClasses(out, srgJar);

        System.out.printf("%s (%d failure%s)%n", failures == 0 ? "PASSED" : "FAILED",
                failures, failures == 1 ? "" : "s");
        if (failures != 0) System.exit(1);
    }

    private static void defineGameClasses(Path out, Path srgJar) {
        Path full = out.resolve("full");
        if (!Files.isDirectory(full)) return;

        List<Path> pending = new ArrayList<>();
        try (var stream = Files.list(full)) {
            for (Path f : stream.sorted().toList()) {
                if (!f.getFileName().toString().endsWith(".class")) continue;
                String n = f.getFileName().toString();
                pending.add(f);
            }
        } catch (IOException e) {
            System.out.println("== Minecraft classes (not defined) ==  [" + e + "]");
            return;
        }
        if (pending.isEmpty()) return;

        System.out.println("== Minecraft classes (the JVM verifier) ==");
        try (URLClassLoader parent = new URLClassLoader(
                     libraryUrls(srgJar), Verify.class.getClassLoader())) {
            Loader loader = new Loader(parent);
            try {
                loader.define("Bridge", Files.readAllBytes(out.resolve("Bridge.class")));
            } catch (Throwable t) {
                System.out.println("  --   Bridge did not define  [" + t + "]");
                return;
            }

            // defineClass loads the superclass, so a subclass cannot go in
            // before its parent. Order is not something this file should have
            // to know, so it retries until nothing more can be placed.
            boolean progress = true;
            while (progress && !pending.isEmpty()) {
                progress = false;
                for (var it = pending.iterator(); it.hasNext(); ) {
                    Path f = it.next();
                    String name = f.getFileName().toString();
                    name = name.substring(0, name.length() - ".class".length());
                    try {
                        loader.define(name, Files.readAllBytes(f));
                        check("verifier accepts " + name, true);
                        it.remove();
                        progress = true;
                    } catch (VerifyError e) {
                        check("verifier accepts " + name + "  [" + e.getMessage() + "]", false);
                        it.remove();
                        progress = true;
                    } catch (NoClassDefFoundError e) {
                        // Still waiting on a supertype; leave it for the next
                        // pass, which the classes that can define will earn.
                        continue;
                    } catch (Throwable t) {
                        System.out.printf("  --   %s  [%s]%n", name, t);
                        it.remove();
                        progress = true;
                    }
                }
            }
            for (Path f : pending) {
                System.out.printf("  --   %s was not defined (a supertype is missing)%n",
                        f.getFileName());
            }
        } catch (Throwable t) {
            System.out.println("== Minecraft classes (not defined) ==  [" + t + "]");
        }
    }

    private static URL[] libraryUrls(Path srgJar) {
        List<URL> urls = new ArrayList<>();
        try {
            urls.add(srgJar.toUri().toURL());
            Path root = srgJar.getParent();
            while (root != null && !"libraries".equals(root.getFileName().toString()))
                root = root.getParent();
            if (root != null) {
                try (var walk = Files.walk(root, 12)) {
                    walk.filter(p -> p.toString().endsWith(".jar"))
                        .forEach(p -> {
                            try {
                                urls.add(p.toUri().toURL());
                            } catch (Exception ignored) {
                            }
                        });
                }
            }
        } catch (Exception ignored) {
        }
        return urls.toArray(new URL[0]);
    }

    private static List<Object[]> patchedClasses(Path out) throws IOException {
        List<Object[]> found = new ArrayList<>();
        if (!Files.isDirectory(out)) return found;
        try (var stream = Files.list(out)) {
            for (Path f : stream.sorted().toList()) {
                String n = f.getFileName().toString();
                if (n.endsWith(".class") && !n.equals("Bridge.class")) found.add(new Object[]{n, f});
            }
        }
        return found;
    }

    // ASM's verifier is an independent data-flow pass over the same bytes. It
    // needs a loader that can see the supertypes, which is why the srg jar goes
    // in.
    //
    // Reported, not counted. The Minecraft classes reference types that live in
    // jars this test does not put on the class path, and ASM says "cannot
    // resolve" the same way it says "bad frame" -- so its verdict on those is
    // worth reading but cannot be the thing that decides pass or fail. The JVM
    // verifier does that, and the C++ auditor covers what it cannot reach.
    private static void checkAsmAgrees(String label, byte[] bytes, URL[] cp) {
        try (URLClassLoader cl = new URLClassLoader(cp, Verify.class.getClassLoader())) {
            var reader = new org.objectweb.asm.ClassReader(bytes);
            var sw = new java.io.StringWriter();
            try (PrintWriter pw = new PrintWriter(sw)) {
                org.objectweb.asm.util.CheckClassAdapter.verify(reader, cl, false, pw);
            }
            String report = sw.toString().trim();
            System.out.printf("  asm  %s%s%n", label,
                    report.isEmpty() ? "" : ("  [" + report.replace('\n', ' ') + "]"));
        } catch (Throwable t) {
            System.out.printf("  asm  %s  [%s]%n", label, t);
        }
    }
}
