package me.mioclient;

import me.mioclient.module.exploit.FastProjectile;

/* loaded from: mio-yarn.jar:me/mioclient/FastProjectilePredicate_6.class */
public class FastProjectilePredicate_6 implements java.util.function.Predicate {
    public FastProjectile fastProjectile;

    public FastProjectilePredicate_6(FastProjectile fastProjectile) {
        this.fastProjectile = fastProjectile;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.fastProjectile.whitelist.is623();
    }
}
