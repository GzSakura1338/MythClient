package me.mioclient;

import me.mioclient.module.movement.ElytraFly;

/* loaded from: mio-yarn.jar:me/mioclient/ElytraFlyPredicate_6.class */
public class ElytraFlyPredicate_6 implements java.util.function.Predicate {
    public ElytraFly elytraFly;

    public ElytraFlyPredicate_6(ElytraFly elytraFly) {
        this.elytraFly = elytraFly;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.elytraFly.vertical.getValue() == ElytraFly.ElytraFlyMode.PLAIN && this.elytraFly.mode.getValue() == ElytraFly.ElytraFlyPredicateMode.CONTROL;
    }
}
