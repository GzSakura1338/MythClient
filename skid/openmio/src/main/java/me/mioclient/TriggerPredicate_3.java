package me.mioclient;

import me.mioclient.module.combat.Trigger;

/* loaded from: mio-yarn.jar:me/mioclient/TriggerPredicate_3.class */
public class TriggerPredicate_3 implements java.util.function.Predicate {
    public Trigger trigger;

    public TriggerPredicate_3(Trigger trigger) {
        this.trigger = trigger;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.trigger.setting5.is623() && this.trigger.setting6.is623();
    }
}
