package me.mioclient;

import me.mioclient.module.movement.EntityControl;

/* loaded from: mio-yarn.jar:me/mioclient/EntityControlPredicate_5.class */
public class EntityControlPredicate_5 implements java.util.function.Predicate {
    public EntityControl entityControl;

    public EntityControlPredicate_5(EntityControl entityControl) {
        this.entityControl = entityControl;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.entityControl.entitySpeed.is623();
    }
}
