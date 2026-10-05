package me.mioclient;

import me.mioclient.module.movement.NoSlow;

/* loaded from: mio-yarn.jar:me/mioclient/NoSlowPredicate_2.class */
public class NoSlowPredicate_2 implements java.util.function.Predicate {
    public NoSlow noSlow;

    public NoSlowPredicate_2(NoSlow noSlow) {
        this.noSlow = noSlow;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.noSlow.blocks.is623();
    }
}
