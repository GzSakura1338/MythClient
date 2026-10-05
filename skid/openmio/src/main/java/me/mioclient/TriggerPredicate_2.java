package me.mioclient;

import me.mioclient.module.combat.Trigger;

/* loaded from: mio-yarn.jar:me/mioclient/TriggerPredicate_2.class */
public class TriggerPredicate_2 implements java.util.function.Predicate {
    public Trigger trigger;

    public TriggerPredicate_2(Trigger trigger) {
        this.trigger = trigger;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.trigger.setting5.is623();
    }
}
