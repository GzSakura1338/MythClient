package me.mioclient;

import me.mioclient.module.movement.EntityControl;

/* loaded from: mio-yarn.jar:me/mioclient/EntityControlPredicate_3.class */
public class EntityControlPredicate_3 implements java.util.function.Predicate {
    public EntityControl entityControl;

    public EntityControlPredicate_3(EntityControl entityControl) {
        this.entityControl = entityControl;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.entityControl.entitySpeed.is623();
    }
}
