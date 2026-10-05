package me.mioclient;

import me.mioclient.module.misc.AntiAim;

/* loaded from: mio-yarn.jar:me/mioclient/AntiAimPredicate_6.class */
public class AntiAimPredicate_6 implements java.util.function.Predicate {
    public AntiAim antiAim;

    public AntiAimPredicate_6(AntiAim antiAim) {
        this.antiAim = antiAim;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.antiAim.yaw.getValue() == AntiAim.AntiAimPredicateMode.antiAimPredicateMode3 || this.antiAim.yaw.getValue() == AntiAim.AntiAimPredicateMode.antiAimPredicateMode;
    }
}
