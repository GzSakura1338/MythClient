package me.mioclient;

import me.mioclient.module.movement.FakeLag;

/* loaded from: mio-yarn.jar:me/mioclient/FakeLagPredicate_2.class */
public class FakeLagPredicate_2 implements java.util.function.Predicate {
    public FakeLag fakeLag;

    public FakeLagPredicate_2(FakeLag fakeLag) {
        this.fakeLag = fakeLag;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.fakeLag.render.is623();
    }
}
