package me.mioclient;

import me.mioclient.module.movement.ElytraFly;

/* loaded from: mio-yarn.jar:me/mioclient/ElytraFlyPredicate_19.class */
public class ElytraFlyPredicate_19 implements java.util.function.Predicate {
    public ElytraFly elytraFly;

    public ElytraFlyPredicate_19(ElytraFly elytraFly) {
        this.elytraFly = elytraFly;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.elytraFly.mode.getValue() == ElytraFly.ElytraFlyPredicateMode.CONTROL && this.elytraFly.spoofPitch.is623();
    }
}
