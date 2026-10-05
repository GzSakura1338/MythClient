package me.mioclient;

import me.mioclient.module.movement.Velocity;

/* loaded from: mio-yarn.jar:me/mioclient/VelocityPredicate.class */
public class VelocityPredicate implements java.util.function.Predicate {
    public Velocity velocity;

    public VelocityPredicate(Velocity velocity) {
        this.velocity = velocity;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.velocity.mode.getValue() == Velocity.VelocityMode.PLAIN;
    }
}
