package me.mioclient;

import me.mioclient.module.misc.AntiAim;

/* loaded from: mio-yarn.jar:me/mioclient/AntiAimPredicate.class */
public class AntiAimPredicate implements java.util.function.Predicate {
    public AntiAim antiAim;

    public AntiAimPredicate(AntiAim antiAim) {
        this.antiAim = antiAim;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.antiAim.yaw.getValue() == AntiAim.AntiAimPredicateMode.antiAimPredicateMode4;
    }
}
