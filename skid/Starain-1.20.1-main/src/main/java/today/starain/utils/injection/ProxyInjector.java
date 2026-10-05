package today.starain.utils.injection;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.Socket;
import java.nio.file.Path;
import java.util.Collections;
import java.util.Enumeration;

public final class ProxyInjector {

    private ProxyInjector() {
    }

    public static final int PROXY_PORT = 25565;

    private static volatile String sFailReason;
    private static volatile boolean sInjected;
    private static volatile Path sDllPath;

    public static long lastBujiPid() {
        if (!NativeInjector.isLoaded()) return 0L;
        try {
            return NativeInjector.getLastInjectedPid();
        } catch (Throwable t) {
            return 0L;
        }
    }

    private static final String PROXY_RES_PATH = "/assets/starain/native/MinecraftProxy_msvc.dll";

    public static Path findProxyDllPath() {

        return NativeInjector.extractResource(PROXY_RES_PATH,
                "MinecraftProxy_msvc_", ".dll");
    }

    public static void markFailed(String reason) {
        sFailReason = reason;
        sInjected = false;
    }

    public static void markInjectedIfListening() {
        if (isProxyListening()) {
            sInjected = true;
            sFailReason = null;
        }
    }

    private static String mapNativeErr(String err) {
        if (err == null) return "unknown";
        if ("NO_TARGET_WINDOW".equals(err)) return "未找到布吉岛窗口";
        return err;
    }

    public static synchronized boolean tryInject() {
        if (sInjected) return true;

        if (isProxyListening()) {
            System.out.println("[Starain] tryInject: proxy already listening on 25565, skip");
            sInjected = true;
            sFailReason = null;
            return true;
        }

        try {
            System.out.println("[Starain] tryInject: ensureLoaded()");
            if (!NativeInjector.ensureLoaded()) {
                sFailReason = "[loadJNI] " + NativeInjector.loadError();
                System.err.println("[Starain] tryInject failed at ensureLoaded: " + sFailReason);
                return false;
            }

            System.out.println("[Starain] tryInject: findProxyDllPath()");
            Path dll = findProxyDllPath();
            if (dll == null) {
                sFailReason = "[findProxy] MinecraftProxy_msvc.dll 资源缺失";
                System.err.println("[Starain] tryInject failed at findProxyDllPath");
                return false;
            }
            sDllPath = dll;
            System.out.println("[Starain] tryInject: proxy dll at " + dll);

            System.out.println("[Starain] tryInject: calling native injectProxyDll (infinite)");
            String err = NativeInjector.injectProxyDll(dll.toAbsolutePath().toString(), 0);
            if (err != null) {
                sFailReason = "[native] " + mapNativeErr(err);
                System.err.println("[Starain] tryInject failed at native: " + err);
                return false;
            }
            System.out.println("[Starain] tryInject: SUCCESS pid=" + lastBujiPid());
        } catch (Throwable t) {
            sFailReason = "[exception] " + t.getClass().getSimpleName() + ": " + t.getMessage();
            System.err.println("[Starain] tryInject THREW: " + sFailReason);
            t.printStackTrace();
            return false;
        }

        sInjected = true;
        sFailReason = null;
        return true;
    }

    public static boolean isInjected() {
        return sInjected;
    }

    public static String failReason() {
        return sFailReason;
    }

    public static Path resolvedDll() {
        return sDllPath;
    }

    public static boolean isProxyListening() {
        try (Socket s = new Socket()) {
            s.connect(new InetSocketAddress("127.0.0.1", PROXY_PORT), 200);
            return true;
        } catch (IOException e) {
            return false;
        }
    }

    public static String getLanAddress() {
        try {
            Enumeration<NetworkInterface> ifs = NetworkInterface.getNetworkInterfaces();
            for (NetworkInterface ni : Collections.list(ifs)) {
                if (!ni.isUp() || ni.isLoopback() || ni.isVirtual()) continue;
                String name = ni.getDisplayName() == null ? "" : ni.getDisplayName().toLowerCase();
                if (name.contains("virtual") || name.contains("vethernet") ||
                        name.contains("vmware") || name.contains("virtualbox") ||
                        name.contains("wsl") || name.contains("hyper-v")) continue;
                for (java.net.InetAddress a : Collections.list(ni.getInetAddresses())) {
                    if (a instanceof java.net.Inet4Address &&
                            !a.isLoopbackAddress() && !a.isLinkLocalAddress()) {
                        return a.getHostAddress();
                    }
                }
            }
        } catch (Exception ignored) {
        }
        return null;
    }

    private static String nz(String s) {
        return s == null ? "" : s;
    }
}
