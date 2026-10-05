package me.mioclient;

import me.mioclient.module.movement.ElytraFly;

/* loaded from: mio-yarn.jar:me/mioclient/ElytraFlyPredicate_5.class */
public class ElytraFlyPredicate_5 implements java.util.function.Predicate {
    public ElytraFly elytraFly;

    public ElytraFlyPredicate_5(ElytraFly elytraFly) {
        this.elytraFly = elytraFly;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.elytraFly.mode.getValue() == ElytraFly.ElytraFlyPredicateMode.CONTROL || this.elytraFly.mode.getValue() == ElytraFly.ElytraFlyPredicateMode.PACKET;
    }
}
