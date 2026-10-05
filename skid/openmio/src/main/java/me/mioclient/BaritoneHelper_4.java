package me.mioclient;

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import me.mioclient.module.Module;
import sun.misc.Unsafe;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/BaritoneHelper_4.class */
public class BaritoneHelper_4 {
    public Unsafe unsafe;
    public List<String> list = new ArrayList();

    public BaritoneHelper_4() {
        try {
            Field declaredField = Unsafe.class.getDeclaredField("theUnsafe");
            declaredField.setAccessible(true);
            this.unsafe = (Unsafe) declaredField.get(null);
        } catch (Exception e) {
        }
    }

    public <T extends Module> T getModule117(Class<T> cls) {
        if (this.unsafe == null || BaritoneHelper_3.keyPearlSearchHelper4 != null) {
            return (T) BaritoneHelper_3.keyPearlSearchHelper4.getEnumSettingHelper120(cls);
        }
        try {
            String str = null;
            StackTraceElement[] stackTrace = Thread.currentThread().getStackTrace();
            int length = stackTrace.length;
            int i = 0;
            while (true) {
                if (i >= length) {
                    break;
                }
                String className = stackTrace[i].getClassName();
                if (!className.contains("java.lang.Thread") && !className.contains(getClass().getName())) {
                    str = className;
                    break;
                }
                i++;
            }
            if (str == null) {
                throw new java.lang.RuntimeException();
            }
            if (!this.list.contains(str)) {
                this.list.add(str);
            }
            return null;
        } catch (Exception e) {
            return null;
        }
    }

    public void do1478() {
        Iterator<String> it = this.list.iterator();
        loop0: while (it.hasNext()) {
            try {
                Class<?> cls = Class.forName(it.next(), false, SearchHelper_4.class.getClassLoader());
                for (Field field : cls.getDeclaredFields()) {
                    if (Modifier.isStatic(field.getModifiers()) && Module.class.isAssignableFrom(field.getType())) {
                        try {
                            Module enumSettingHelper120 = (Module)(BaritoneHelper_3.keyPearlSearchHelper4.getEnumSettingHelper120((Class) field.getType()));
                            if (enumSettingHelper120 == null) {
                                throw new java.lang.RuntimeException();
                            }
                            this.unsafe.putObject(cls, this.unsafe.staticFieldOffset(field), enumSettingHelper120);
                        } catch (Exception e) {
                        }
                    }
                }
            } catch (Exception e2) {
                throw new java.lang.RuntimeException();
            }
        }
        this.list.clear();
        System.gc();
    }
}
