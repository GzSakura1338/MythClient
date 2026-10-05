package me.mioclient;

import me.mioclient.module.misc.AntiAFK;

/* loaded from: mio-yarn.jar:me/mioclient/AntiAFKPredicate.class */
public class AntiAFKPredicate implements java.util.function.Predicate {
    public AntiAFK antiAFK;

    public AntiAFKPredicate(AntiAFK antiAFK) {
        this.antiAFK = antiAFK;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.antiAFK.attack.getValue().booleanValue();
    }
}
