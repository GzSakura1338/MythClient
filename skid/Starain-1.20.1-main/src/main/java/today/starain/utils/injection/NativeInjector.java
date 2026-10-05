package today.starain.utils.injection;

import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

public final class NativeInjector {

    private static final String RES_PATH = "/assets/starain/native/starain_inject.dll";

    private static boolean sLoaded;
    private static String sLoadError;

    public static synchronized boolean ensureLoaded() {
        if (sLoaded) return true;
        try {
            Path p = resolveLibraryPath();
            if (p == null) {
                sLoadError = "starain_inject.dll 资源未找到";
                return false;
            }
            System.load(p.toAbsolutePath().toString());
            sLoaded = true;
            return true;
        } catch (Throwable t) {
            sLoadError = t.getMessage();
            return false;
        }
    }

    public static boolean isLoaded() {
        return sLoaded;
    }

    public static String loadError() {
        return sLoadError;
    }

    private static Path resolveLibraryPath() {

        return extractResource(RES_PATH, "starain_inject_", ".dll");
    }

    static Path extractResource(String resPath, String prefix, String suffix) {
        try (InputStream is = NativeInjector.class.getResourceAsStream(resPath)) {
            if (is == null) return null;
            Path tmp = Files.createTempFile(prefix, suffix);
            tmp.toFile().deleteOnExit();
            Files.copy(is, tmp, StandardCopyOption.REPLACE_EXISTING);
            return tmp;
        } catch (Throwable t) {
            return null;
        }
    }

    public static native String injectProxyDll(String dllPath, int timeoutMs);

    public static native long getLastInjectedPid();

    private static String nz(String s) {
        return s == null ? "" : s;
    }

    private NativeInjector() {
    }
}
