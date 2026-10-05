import java.nio.file.*;
import org.objectweb.asm.*;
import org.objectweb.asm.tree.*;

// Test-only shell: execute the exact patched helper/callback with inert game stubs.
// The real HeyPixel class initializer and constructor must never run in a test JVM.
public class ExtractJoinFixture {
    public static void main(String[] args) throws Exception {
        ClassNode source = new ClassNode();
        new ClassReader(Files.readAllBytes(Path.of(args[0]))).accept(source, 0);
        ClassWriter out = new ClassWriter(0);
        out.visit(source.version, Opcodes.ACC_PUBLIC, source.name, null, "java/lang/Object", null);
        source.fields.stream().filter(f -> f.name.equals("nova$clientLoaded")).forEach(f -> f.accept(out));
        source.methods.stream().filter(m -> m.name.equals("nova$loadEmbeddedClient") ||
            m.desc.equals("(Lnet/minecraftforge/client/event/ClientPlayerNetworkEvent$LoggingIn;)V"))
            .forEach(m -> m.accept(out));
        MethodVisitor ctor = out.visitMethod(Opcodes.ACC_PUBLIC, "<init>", "()V", null, null);
        ctor.visitCode(); ctor.visitVarInsn(Opcodes.ALOAD, 0);
        ctor.visitMethodInsn(Opcodes.INVOKESPECIAL, "java/lang/Object", "<init>", "()V", false);
        ctor.visitInsn(Opcodes.RETURN); ctor.visitMaxs(1, 1); ctor.visitEnd(); out.visitEnd();
        Path file = Path.of(args[1]).resolve(source.name + ".class");
        Files.createDirectories(file.getParent()); Files.write(file, out.toByteArray());
        Files.writeString(Path.of(args[1]).resolve("fixture-owner.txt"), source.name.replace('/', '.'));
        System.out.println("Extracted exact helper and join callback for isolated JVM verification.");
    }
}
