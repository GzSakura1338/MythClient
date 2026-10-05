import java.nio.file.*;
import java.lang.classfile.ClassFile;
import java.lang.classfile.ClassHierarchyResolver;
import java.net.URLClassLoader;
import java.net.URL;
import java.util.ArrayList;

// Analyze the actual patched class bytes without loading Minecraft or its dependencies.
public final class VerifyHooks {
    public static void main(String[] args) throws Exception {
        int classes = 0, methods = 0;
        var resources = new ArrayList<URL>();
        resources.add(Path.of(args[0]).toUri().toURL());
        resources.add(Path.of(args[1]).toUri().toURL());
        try (var libraries = Files.walk(Path.of(args[2]))) {
            for (var jar : libraries.filter(p -> p.toString().endsWith(".jar")).sorted(java.util.Comparator.reverseOrder()).toList())
                resources.add(jar.toUri().toURL());
        }
        // Parse inheritance from resources; no game class is defined or initialized.
        try (var loader = new URLClassLoader(resources.toArray(URL[]::new), ClassLoader.getPlatformClassLoader())) {
        var verifier = ClassFile.of(ClassFile.ClassHierarchyResolverOption.of(
            ClassHierarchyResolver.ofResourceParsing(loader).cached()));
        try (var paths = Files.walk(Path.of(args[0]))) {
            for (var path : paths.filter(p -> p.toString().endsWith(".class")).toList()) {
                var bytes = Files.readAllBytes(path);
                var errors = verifier.verify(bytes);
                if (!errors.isEmpty()) throw new AssertionError(path + ": " + errors);
                methods += ClassFile.of().parse(bytes).methods().stream().filter(m -> m.code().isPresent()).count();
                classes++;
            }
        }
        }
        System.out.printf("26.3 JDK ClassFile verifier: %d classes, %d methods, 0 failures%n", classes, methods);
    }
}
