// Defines one class file taken straight out of the game's own hook dump.
//
// The harness's Verify.java only ever sees classes the rewriter produced from
// the vanilla SRG jar. This one takes bytes that the game actually handed to
// RetransformClasses and hands them to the same verifier, which is the only
// way to tell a rewriter bug from a bug that only shows up on input the SRG
// jar does not have (a Mixin-transformed class, for instance).
//
//   java Diag <class-file> <internal-name> <libraries-or-srg-jar>

import java.io.PrintWriter;
import java.net.URL;
import java.net.URLClassLoader;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

public class Diag {

    static final class Loader extends ClassLoader {
        Loader(ClassLoader parent) { super(parent); }
        Class<?> define(String name, byte[] bytes) {
            return defineClass(name, bytes, 0, bytes.length);
        }
    }

    public static void main(String[] args) throws Exception {
        Path file = Paths.get(args[0]);
        String name = args[1];
        Path jar = Paths.get(args[2]);
        byte[] bytes = Files.readAllBytes(file);

        List<URL> urls = new ArrayList<>();
        urls.add(jar.toUri().toURL());
        Path root = jar.getParent();
        while (root != null && !"libraries".equals(root.getFileName().toString()))
            root = root.getParent();
        if (root != null) {
            try (var walk = Files.walk(root, 12)) {
                walk.filter(p -> p.toString().endsWith(".jar")).forEach(p -> {
                    try { urls.add(p.toUri().toURL()); } catch (Exception ignored) { }
                });
            }
        }

        // ASM's data-flow pass, printed rather than counted: it reports an
        // unresolvable type the same way it reports a bad frame.
        try (URLClassLoader cl = new URLClassLoader(urls.toArray(new URL[0]),
                                                    Diag.class.getClassLoader())) {
            var reader = new org.objectweb.asm.ClassReader(bytes);
            var sw = new java.io.StringWriter();
            try (PrintWriter pw = new PrintWriter(sw)) {
                org.objectweb.asm.util.CheckClassAdapter.verify(reader, cl, false, pw);
            }
            String report = sw.toString().trim();
            System.out.println("asm: " + (report.isEmpty() ? "(clean)" :
                    report.replace('\n', ' ')));
        } catch (Throwable t) {
            System.out.println("asm: " + t);
        }

        try (URLClassLoader parent = new URLClassLoader(urls.toArray(new URL[0]),
                                                        Diag.class.getClassLoader())) {
            Loader loader = new Loader(parent);
            loader.define(name, bytes);
            System.out.println("jvm: accepted");
        } catch (Throwable t) {
            System.out.println("jvm: " + t);
        }
    }
}
