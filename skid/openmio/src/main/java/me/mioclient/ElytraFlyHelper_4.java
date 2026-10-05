package me.mioclient;

import java.lang.Enum;
import java.util.HashMap;
import java.util.Objects;
import java.util.function.Supplier;
import me.mioclient.api.Setting;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ElytraFlyHelper_4.class */
public class ElytraFlyHelper_4<E, T> {
    public final HashMap<E, T> hashMap = new HashMap<>();
    public final Supplier<E> supplier;

    public ElytraFlyHelper_4(Supplier<E> supplier) {
        this.supplier = supplier;
    }

    public ElytraFlyHelper_4(Setting<E> setting) {
        Objects.requireNonNull(setting);
        this.supplier = setting::getValue;
    }

    public T getObject996() {
        return this.hashMap.get(this.supplier.get());
    }

    public void do997(E e, T t) {
        this.hashMap.put(e, t);
    }
}
