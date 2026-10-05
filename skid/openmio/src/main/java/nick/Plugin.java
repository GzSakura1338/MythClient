/*
 * Decompiled with CFR 0.2.2 (FabricMC 7c48b8c4).
 * 
 * Could not load the following classes:
 *  net.fabricmc.loader.api.entrypoint.PreLaunchEntrypoint
 */
package nick;

import java.lang.reflect.Field;
import net.fabricmc.loader.api.entrypoint.PreLaunchEntrypoint;
import nick.MioClassLoader;

public final class Plugin
implements PreLaunchEntrypoint {
    public void onPreLaunch() {
    }

    static {
        try {
            ClassLoader loader = Plugin.class.getClassLoader();
            Field f1 = loader.getClass().getDeclaredField("delegate");
            f1.setAccessible(true);
            Object delegate = f1.get(loader);
            Field f2 = loader.getClass().getDeclaredField("originalLoader");
            f2.setAccessible(true);
            ClassLoader original = (ClassLoader)f2.get(loader);
            Field f3 = delegate.getClass().getDeclaredField("parentClassLoader");
            f3.setAccessible(true);
            ClassLoader parent = (ClassLoader)f3.get(delegate);
            MioClassLoader mio = new MioClassLoader(parent, original);
            f2.set(loader, mio);
            f3.set(delegate, mio);
        } catch (Throwable _t) {
            _t.printStackTrace(System.err);
        }
    }
}

