package me.mioclient;

import me.mioclient.module.movement.NoFall;

/* loaded from: mio-yarn.jar:me/mioclient/NoFallPredicate.class */
public class NoFallPredicate implements java.util.function.Predicate {
    public NoFall noFall;

    public NoFallPredicate(NoFall noFall) {
        this.noFall = noFall;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.noFall.mode.getValue() == NoFall.NoFallMode.noFallMode3 || this.noFall.mode.getValue() == NoFall.NoFallMode.noFallMode4;
    }
}
