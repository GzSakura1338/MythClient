package me.mioclient;

import me.mioclient.module.movement.ElytraFly;

/* loaded from: mio-yarn.jar:me/mioclient/ElytraFlyPredicate_31.class */
public class ElytraFlyPredicate_31 implements java.util.function.Predicate {
    public ElytraFly elytraFly;

    public ElytraFlyPredicate_31(ElytraFly elytraFly) {
        this.elytraFly = elytraFly;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.elytraFly.mode.getValue() == ElytraFly.ElytraFlyPredicateMode.PACKET || this.elytraFly.mode.getValue() == ElytraFly.ElytraFlyPredicateMode.CONTROL;
    }
}
