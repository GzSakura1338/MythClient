package me.mioclient;

import java.util.function.Consumer;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ClassBaritoneHelper5.class */
public class ClassBaritoneHelper5<T> implements BaritoneHelper_5 {
    public final Class<?> class_;
    public final int num;
    public final Consumer<T> consumer;

    public ClassBaritoneHelper5(Class<?> cls, int i, Consumer<T> consumer) {
        this.class_ = cls;
        this.num = i;
        this.consumer = consumer;
    }

    public ClassBaritoneHelper5(Class<?> cls, Consumer<T> consumer) {
        this(cls, 0, consumer);
    }

    @Override // me.mioclient.BaritoneHelper_5
    public void do2109(Object obj) {
        this.consumer.accept((T) obj);
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
        return false;
    }
}
