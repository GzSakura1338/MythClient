package me.mioclient;

import java.util.Collection;
import java.util.Optional;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/ModuleListHelper.class */
public abstract class ModuleListHelper<E, T extends Collection<E>> implements Helper_9<E, T> {
    public final T registry;

    public ModuleListHelper(T t) {
        this.registry = t;
    }

    public Optional<E> getOptional2404(java.util.function.Predicate<E> predicate) {
        for (E e : this.registry) {
            if (predicate.test(e)) {
                return Optional.of(e);
            }
        }
        return Optional.empty();
    }

    public E getObject2405(java.util.function.Predicate<E> predicate) {
        for (E e : this.registry) {
            if (predicate.test(e)) {
                return e;
            }
        }
        return null;
    }

    @Override // me.mioclient.Helper_9
    public T getRegistry() {
        return this.registry;
    }
}
