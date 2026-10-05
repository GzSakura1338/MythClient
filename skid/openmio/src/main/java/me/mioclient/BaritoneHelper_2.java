package me.mioclient;

import java.lang.invoke.LambdaMetafactory;
import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.function.Consumer;
import me.mioclient.event.Listen;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/BaritoneHelper_2.class */
public class BaritoneHelper_2 implements BaritoneHelper_5 {
    public static boolean flag;
    public static Constructor<MethodHandles.Lookup> constructor;
    public static Method method;
    public final Class<?> class_;
    public final boolean flag2;
    public final int num;
    public Consumer<Object> consumer;

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/BaritoneHelper_2$Inner.class */
    public interface Inner {
        MethodHandles.Lookup create(Method method, Class<?> cls);
    }

    public BaritoneHelper_2(Inner inner, Class<?> cls, Object obj, Method method2) {
        MethodHandles.Lookup create;
        MethodHandle findVirtual;
        MethodType methodType;
        this.class_ = method2.getParameters()[0].getType();
        this.flag2 = Modifier.isStatic(method2.getModifiers());
        this.num = ((Listen) method2.getAnnotation(Listen.class)).get219();
        try {
            String name = method2.getName();
            if (flag) {
                boolean isAccessible = constructor.isAccessible();
                constructor.setAccessible(true);
                create = constructor.newInstance(cls);
                constructor.setAccessible(isAccessible);
            } else {
                create = inner.create(method, cls);
            }
            MethodType methodType2 = MethodType.methodType((Class<?>) Void.TYPE, method2.getParameters()[0].getType());
            if (this.flag2) {
                findVirtual = create.findStatic(cls, name, methodType2);
                methodType = MethodType.methodType(Consumer.class);
            } else {
                findVirtual = create.findVirtual(cls, name, methodType2);
                methodType = MethodType.methodType((Class<?>) Consumer.class, cls);
            }
            MethodHandle target = LambdaMetafactory.metafactory(create, "accept", methodType, MethodType.methodType((Class<?>) Void.TYPE, (Class<?>) Object.class), findVirtual, methodType2).getTarget();
            if (this.flag2) {
                this.consumer = (Consumer) target.invoke();
            } else {
                this.consumer = (Consumer) target.invoke(obj);
            }
        } catch (Throwable th) {
            th.printStackTrace();
        }
    }

    @Override // me.mioclient.BaritoneHelper_5
    public void do2109(Object obj) {
        this.consumer.accept(obj);
    }

    @Override // me.mioclient.BaritoneHelper_5
    public Class<?> getClass2110() {
        return this.class_;
    }

    @Override // me.mioclient.BaritoneHelper_5
    public int get888() {
        return this.num;
    }

    @Override // me.mioclient.BaritoneHelper_5
    public boolean is2111() {
        return this.flag2;
    }

    static {
        try {
            flag = System.getProperty("java.version").startsWith("1.8");
            if (flag) {
                constructor = MethodHandles.Lookup.class.getDeclaredConstructor(Class.class);
            } else {
                method = MethodHandles.class.getDeclaredMethod("privateLookupIn", Class.class, MethodHandles.Lookup.class);
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        }
    }
}
