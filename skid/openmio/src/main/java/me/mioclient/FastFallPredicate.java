package me.mioclient;

import me.mioclient.module.movement.FastFall;

/* loaded from: mio-yarn.jar:me/mioclient/FastFallPredicate.class */
public class FastFallPredicate implements java.util.function.Predicate {
    public FastFall fastFall;

    public FastFallPredicate(FastFall fastFall) {
        this.fastFall = fastFall;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.fastFall.mode.getValue() == FastFall.FastFallMode.PLAIN;
    }
}
