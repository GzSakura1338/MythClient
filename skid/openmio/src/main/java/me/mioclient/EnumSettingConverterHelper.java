package me.mioclient;

import java.lang.reflect.AnnotatedElement;
import java.lang.reflect.Field;
import me.mioclient.api.Setting;
import me.mioclient.module.Module;
import net.fabricmc.loader.api.FabricLoader;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/EnumSettingConverterHelper.class */
public class EnumSettingConverterHelper {
    public static boolean flag;

    public static void do1628(Module module) {
        for (Field field : module.getClass().getDeclaredFields()) {
            if (is1631(field)) {
                field.setAccessible(true);
                try {
                    module.unregister((Setting<?>) field.get(module));
                } catch (IllegalAccessException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public static boolean is1629(Class<?> cls) {
        return is1632() && is1633(cls);
    }

    public static boolean is1630(Enum<?> r3) {
        try {
            if (is1632()) {
                if (is1633(r3.getDeclaringClass().getDeclaredField(r3.name()))) {
                    return true;
                }
            }
            return false;
        } catch (NoSuchFieldException e) {
            return false;
        }
    }

    public static boolean is1631(Field field) {
        return is1632() && is1633(field);
    }

    public static boolean is1632() {
        return !flag;
    }

    public static boolean is1633(AnnotatedElement annotatedElement) {
        return annotatedElement.isAnnotationPresent(Annotation.class);
    }

    static {
        flag = false;
        String[] launchArguments = FabricLoader.getInstance().getLaunchArguments(true);
        int length = launchArguments.length;
        int i = 0;
        while (true) {
            if (i >= length) {
                break;
            }
            if ("--mio-allow-experimental".equals(launchArguments[i])) {
                flag = true;
                break;
            }
            i++;
        }
        if (System.getProperties().get("mio.allowExperimental") != null && "true".equalsIgnoreCase((String) System.getProperties().get("mio.allowExperimental"))) {
            flag = true;
        }
    }
}
