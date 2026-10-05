package me.mioclient;

import me.mioclient.module.movement.ElytraFly;

/* loaded from: mio-yarn.jar:me/mioclient/ElytraFlyPredicate_4.class */
public class ElytraFlyPredicate_4 implements java.util.function.Predicate {
    public ElytraFly elytraFly;

    public ElytraFlyPredicate_4(ElytraFly elytraFly) {
        this.elytraFly = elytraFly;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.elytraFly.mode.getValue() == ElytraFly.ElytraFlyPredicateMode.CONTROL;
    }
}
