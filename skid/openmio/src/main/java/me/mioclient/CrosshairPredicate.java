package me.mioclient;

import me.mioclient.module.render.Crosshair;

/* loaded from: mio-yarn.jar:me/mioclient/CrosshairPredicate.class */
public class CrosshairPredicate implements java.util.function.Predicate {
    public Crosshair crosshair;

    public CrosshairPredicate(Crosshair crosshair) {
        this.crosshair = crosshair;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.crosshair.dynamic.is623();
    }
}
