package me.mioclient;

import me.mioclient.module.misc.AntiAim;

/* loaded from: mio-yarn.jar:me/mioclient/AntiAimPredicate_2.class */
public class AntiAimPredicate_2 implements java.util.function.Predicate {
    public AntiAim antiAim;

    public AntiAimPredicate_2(AntiAim antiAim) {
        this.antiAim = antiAim;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.antiAim.yaw.getValue() == AntiAim.AntiAimPredicateMode.antiAimPredicateMode;
    }
}
