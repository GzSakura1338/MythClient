package me.mioclient;

import me.mioclient.module.movement.NoSlow;

/* loaded from: mio-yarn.jar:me/mioclient/NoSlowPredicate_6.class */
public class NoSlowPredicate_6 implements java.util.function.Predicate {
    public NoSlow noSlow;

    public NoSlowPredicate_6(NoSlow noSlow) {
        this.noSlow = noSlow;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return (this.noSlow.mode.getValue() == NoSlow.NoSlowMode.GRIM || this.noSlow.mode.getValue() == NoSlow.NoSlowMode.GRIMV3) ? false : true;
    }
}
