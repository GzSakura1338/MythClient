package me.mioclient;

import me.mioclient.module.movement.ElytraFly;

/* loaded from: mio-yarn.jar:me/mioclient/ElytraFlyPredicate_26.class */
public class ElytraFlyPredicate_26 implements java.util.function.Predicate {
    public ElytraFly elytraFly;

    public ElytraFlyPredicate_26(ElytraFly elytraFly) {
        this.elytraFly = elytraFly;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.elytraFly.mode.getValue() == ElytraFly.ElytraFlyPredicateMode.BOOST && this.elytraFly.inLava.is623();
    }
}
