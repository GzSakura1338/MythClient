// Retransforms a real patched Minecraft class the way the game does.
//
//   java -javaagent:build/rt-agent.jar ... RtTest <patched-dir> <srg-jar>
//
// Verify.java's defineGameClasses answers "would this class file be accepted if
// the JVM read it off disk?". This answers the question the game actually asks:
// "will RetransformClasses take these bytes for a class that is already
// loaded?". The two disagree -- that is the whole reason this file exists --
// and when they do, the failure the game logs is
// `Transformer: <name> was not retransformed (JVMTI error 62)`.

import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.Instrumentation;
import java.net.URL;
import java.net.URLClassLoader;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.ProtectionDomain;
import java.util.ArrayList;
import java.util.List;

public class RtTest {

    static final class Loader extends ClassLoader {
        Loader(ClassLoader parent) { super(parent); }
        Class<?> define(String name, byte[] bytes) {
            return defineClass(name, bytes, 0, bytes.length);
        }
    }

    static final class Swap implements ClassFileTransformer {
        final String internal;
        final byte[] bytes;
        Swap(String internal, byte[] bytes) { this.internal = internal; this.bytes = bytes; }

        public byte[] transform(ClassLoader loader, String name, Class<?> beingRedefined,
                                ProtectionDomain pd, byte[] buf) {
            return internal.equals(name) ? bytes : null;
        }
        public byte[] transform(ClassLoader loader, String name, Class<?> beingRedefined,
                                ProtectionDomain pd, byte[] buf, Throwable t) {
            return transform(loader, name, beingRedefined, pd, buf);
        }
    }

    public static void main(String[] args) throws Exception {
        Path dir = Paths.get(args[0]);
        Path srgJar = Paths.get(args[1]);
        Instrumentation inst = RtAgent.inst;
        if (inst == null) {
            System.out.println("no Instrumentation; the agent jar was not passed");
            System.exit(2);
        }
        System.out.println("canRetransformClasses = " + inst.isRetransformClassesSupported());

        List<URL> urls = new ArrayList<>();
        urls.add(srgJar.toUri().toURL());
        Path root = srgJar.getParent();
        while (root != null && !"libraries".equals(root.getFileName().toString()))
            root = root.getParent();
        if (root != null) {
            try (var walk = Files.walk(root, 12)) {
                walk.filter(p -> p.toString().endsWith(".jar")).forEach(p -> {
                    try { urls.add(p.toUri().toURL()); } catch (Exception ignored) { }
                });
            }
        }

        List<Path> pending = new ArrayList<>();
        try (var s = Files.list(dir)) {
            for (Path f : s.sorted().toList())
                if (f.getFileName().toString().endsWith(".class")) pending.add(f);
        }

        try (URLClassLoader parent = new URLClassLoader(urls.toArray(new URL[0]),
                                                        RtTest.class.getClassLoader())) {
            Loader loader = new Loader(parent);

            // Superclasses have to be in before their subclasses, and a class
            // whose supertype is still missing is not an error -- it is the
            // next round's work.
            boolean progress = true;
            while (progress && !pending.isEmpty()) {
                progress = false;
                for (var it = pending.iterator(); it.hasNext(); ) {
                    Path f = it.next();
                    String name = f.getFileName().toString();
                    name = name.substring(0, name.length() - ".class".length());
                    try {
                        loader.define(name, Files.readAllBytes(f));
                        it.remove();
                        progress = true;
                    } catch (NoClassDefFoundError e) {
                        continue;
                    } catch (Throwable t) {
                        System.out.println("  !!   " + name + " would not even define: " + t);
                        it.remove();
                        progress = true;
                    }
                }
            }
            for (Path f : pending)
                System.out.println("  --   " + f.getFileName() + " was not defined");

            int pass = 0, fail = 0, skipped = 0;
            for (Path f : Files.list(dir).sorted().toList()) {
                String file = f.getFileName().toString();
                if (!file.endsWith(".class")) continue;
                String name = file.substring(0, file.length() - ".class".length());
                String internal = name.replace('.', '/');
                Class<?> c;
                try {
                    c = Class.forName(name, false, loader);
                } catch (Throwable t) {
                    skipped++;
                    continue;
                }
                if (c.getClassLoader() != loader) { skipped++; continue; }

                byte[] patched = Files.readAllBytes(f);
                Swap swap = new Swap(internal, patched);
                inst.addTransformer(swap, true);
                try {
                    inst.retransformClasses(c);
                    System.out.println("  ok   retransform " + name);
                    pass++;
                } catch (Throwable t) {
                    System.out.println("  FAIL retransform " + name + "  [" + t + "]");
                    fail++;
                } finally {
                    inst.removeTransformer(swap);
                }
            }
            System.out.printf("%d retransformed, %d refused, %d not loadable here%n",
                    pass, fail, skipped);
            if (fail != 0) System.exit(1);
        }
    }
}
