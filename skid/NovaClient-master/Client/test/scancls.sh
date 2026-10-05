#!/usr/bin/env bash
# Constant-pool scan of HeyPixelMod for JVM-introspection / reporting capability.
D=/tmp/hp
scan () {
    echo "=== $1"
    grep -rlaE "$2" "$D" --include='*.class' | sed "s#$D/##" | head -25
    echo
}
scan "java.lang.instrument / JVMTI / attach" 'java/lang/instrument|ClassFileTransformer|retransformClasses|getAllLoadedClasses|VirtualMachine|loadAgent|javaagent'
scan "thread stacks / MXBean / management" 'getStackTrace|getAllStackTraces|ThreadMXBean|ManagementFactory|java/lang/management'
scan "classloader introspection" 'findLoadedClass|getDefinedClasses|getClassLoadingLock|jdk/internal/loader|ClassLoader'
scan "reflection / unsafe" 'sun/misc/Unsafe|jdk/internal/misc/Unsafe|AccessController|setAccessible|PrivateLookupIn'
scan "forge / mixin internals" 'net/minecraftforge/fml|FMLLoader|ModList|Launch|spongepowered/asm|mixin'
scan "process / exec / native load" 'ProcessBuilder|Runtime.*exec|System.*load|System.*loadLibrary|java/lang/Process'
scan "network reporting" 'HttpURLConnection|java/net/Socket|URLConnection|okhttp|WebSocket|apache/http'
scan "throwable / stack walking" 'getOurStackTrace|getStackTraceElement|StackWalker|Thread/getStackTrace'
