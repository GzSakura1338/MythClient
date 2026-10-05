package me.mioclient;

import me.mioclient.module.exploit.ChorusControl;

/* loaded from: mio-yarn.jar:me/mioclient/ChorusControlPredicate.class */
public class ChorusControlPredicate implements java.util.function.Predicate {
    public ChorusControl chorusControl;

    public ChorusControlPredicate(ChorusControl chorusControl) {
        this.chorusControl = chorusControl;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.chorusControl.render.is623();
    }
}
