package me.mioclient;

import me.mioclient.module.movement.EntityControl;

/* loaded from: mio-yarn.jar:me/mioclient/EntityControlPredicate.class */
public class EntityControlPredicate implements java.util.function.Predicate {
    public EntityControl entityControl;

    public EntityControlPredicate(EntityControl entityControl) {
        this.entityControl = entityControl;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.entityControl.entitySpeed.is623();
    }
}
