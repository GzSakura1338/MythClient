package me.mioclient;

import me.mioclient.module.exploit.FastProjectile;

/* loaded from: mio-yarn.jar:me/mioclient/FastProjectilePredicate_4.class */
public class FastProjectilePredicate_4 implements java.util.function.Predicate {
    public FastProjectile fastProjectile;

    public FastProjectilePredicate_4(FastProjectile fastProjectile) {
        this.fastProjectile = fastProjectile;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.fastProjectile.whitelist.is623();
    }
}
