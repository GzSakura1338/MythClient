package me.mioclient;

import java.util.Collection;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/Helper_9.class */
public interface Helper_9<E, T extends Collection<E>> {
    T getRegistry();

    boolean register(E e);

    boolean unregister(E e);
}
