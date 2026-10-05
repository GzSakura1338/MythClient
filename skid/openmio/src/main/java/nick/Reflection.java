/*
 * Decompiled with CFR 0.2.2 (FabricMC 7c48b8c4).
 */
package nick;

import java.lang.reflect.AccessibleObject;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Arrays;
import sun.misc.Unsafe;

public class Reflection {
    private static Unsafe unsafe;
    private static Method[] table;

    public static <T extends AccessibleObject> boolean patched(T obj) {
        return unsafe.getBoolean(obj, 12L);
    }

    public static <T extends AccessibleObject> void patch(T obj) {
        if (!Reflection.patched(obj)) {
            unsafe.putBoolean(obj, 12L, true);
        }
    }

    public static Method getMethod(Class<?> klass, String name, Class<?> ... parameters) {
        Method[] methods;
        if (klass == null) {
            throw new RuntimeException("no method found");
        }
        for (Method method : methods = (Method[])Reflection.getMembers(klass, (Mask)Mask.METHODS)) {
            if (!method.getName().equals(name) || !Arrays.equals(method.getParameterTypes(), parameters)) continue;
            Reflection.patch(method);
            return method;
        }
        return Reflection.getMethod(klass.getSuperclass(), name, parameters);
    }

    public static Field getField(Class<?> klass, String name) {
        Field[] fields;
        if (klass == null) {
            throw new RuntimeException("no field found");
        }
        for (Field field : fields = (Field[])Reflection.getMembers(klass, (Mask)Mask.FIELDS)) {
            if (!field.getName().equals(name)) continue;
            Reflection.patch(field);
            return field;
        }
        return Reflection.getField(klass.getSuperclass(), name);
    }

    public static <T> Constructor<T> getConstructors(Class<?> klass, Class<?> ... parameters) {
        Constructor[] constructors;
        for (Constructor constructor : constructors = (Constructor[])Reflection.getMembers(klass, (Mask)Mask.CONSTRUCTORS)) {
            if (!Arrays.equals(constructor.getParameterTypes(), parameters)) continue;
            Reflection.patch(constructor);
            return constructor;
        }
        throw new RuntimeException("no constructor found");
    }

    public static <T extends AccessibleObject> T[] getMembers(Class<?> klass, Mask mask) {
        try {
            return (T[])(Object) table[mask.ordinal()].invoke(klass, false);
        } catch (Throwable _t) {
            _t.printStackTrace(System.err);
            throw new RuntimeException();
        }
    }

    static {
        try {
            Field f = Unsafe.class.getDeclaredField("theUnsafe");
            f.setAccessible(true);
            unsafe = (Unsafe)f.get(null);
            for (Method method : table = new Method[]{Class.class.getDeclaredMethod("getDeclaredMethods0", Boolean.TYPE), Class.class.getDeclaredMethod("getDeclaredFields0", Boolean.TYPE), Class.class.getDeclaredMethod("getDeclaredConstructors0", Boolean.TYPE)}) {
                Reflection.patch(method);
            }
        } catch (Throwable throwable) {
            // empty catch block
        }
    }

    public static enum Mask {
        METHODS,
        FIELDS,
        CONSTRUCTORS;

    }
}

