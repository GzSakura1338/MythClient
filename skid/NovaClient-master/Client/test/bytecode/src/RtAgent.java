// Hands a plain `java` run the Instrumentation object.
//
// Verify.java defines its patched classes with defineClass, which only ever
// asks the verifier that runs on a *first* load. The game does not do that: it
// hands the bytes to RetransformClasses, and HotSpot runs a different, stricter
// pass over them there -- constant pool merge, class comparison, per-method
// re-verification against the already-loaded class. A class can be accepted by
// one and refused by the other, and JVMTI_ERROR_FAILS_VERIFICATION is what the
// second one says when it refuses.
//
//   java -javaagent:rt-agent.jar ...
//
// Build with the manifest in build/rt-agent.mf.

import java.lang.instrument.Instrumentation;

public class RtAgent {
    public static Instrumentation inst;

    public static void premain(String args, Instrumentation i) { inst = i; }
    public static void agentmain(String args, Instrumentation i) { inst = i; }
}
