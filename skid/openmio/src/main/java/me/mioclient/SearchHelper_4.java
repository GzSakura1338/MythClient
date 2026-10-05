package me.mioclient;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import java.lang.invoke.MethodHandles;
import java.lang.reflect.InvocationTargetException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import net.minecraft.client.MinecraftClient;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/SearchHelper_4.class */
public interface SearchHelper_4 {
    public static final Gson gson = new GsonBuilder().setLenient().setPrettyPrinting().create();
    public static final MinecraftClient minecraftClient = MinecraftClient.getInstance();
    public static final BaritoneHelper baritoneHelper = new Inner();
    public static final ExecutorService executorService = Executors.newCachedThreadPool();

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/SearchHelper_4$Inner.class */
    class Inner extends BaritoneHelper {
        public Inner() {
            do1793("me.mioclient", (method, cls) -> {
                Object[] objArr = new Object[2];
                objArr[0] = cls;
                try {
                    objArr[1] = (MethodHandles.Lookup) MethodHandles.class.getDeclaredMethod("lookup", new Class[0]).invoke(null, new Object[0]);
                    return (MethodHandles.Lookup) method.invoke(null, objArr);
                } catch (Exception unused) {
                    throw new java.lang.RuntimeException(unused);
                }
            });
        }
    }

    default boolean is1469() {
        return minecraftClient.player == null || minecraftClient.world == null;
    }

    static boolean is1470(Object obj) {
        return minecraftClient.player == obj;
    }

    static boolean is1471() {
        try {
            Class.forName("me.mioclient.loader.Globals", false, SearchHelper_4.class.getClassLoader());
            return true;
        } catch (Exception e) {
            return false;
        }
    }
}
