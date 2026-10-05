package me.mioclient;

import me.mioclient.module.combat.Trigger;

/* loaded from: mio-yarn.jar:me/mioclient/TriggerPredicate.class */
public class TriggerPredicate implements java.util.function.Predicate {
    public Trigger trigger;

    public TriggerPredicate(Trigger trigger) {
        this.trigger = trigger;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.trigger.setting5.is623();
    }
}
