package me.mioclient;

import me.mioclient.module.movement.Warp;

/* loaded from: mio-yarn.jar:me/mioclient/WarpPredicate_2.class */
public class WarpPredicate_2 implements java.util.function.Predicate {
    public Warp warp;

    public WarpPredicate_2(Warp warp) {
        this.warp = warp;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.warp.mode.getValue() == Warp.WarpMode.PLAIN;
    }
}
