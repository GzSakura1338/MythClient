package me.mioclient;

import me.mioclient.module.combat.AntiPhase;

/* loaded from: mio-yarn.jar:me/mioclient/AntiPhasePredicate_2.class */
public class AntiPhasePredicate_2 implements java.util.function.Predicate {
    public AntiPhase antiPhase;

    public AntiPhasePredicate_2(AntiPhase antiPhase) {
        this.antiPhase = antiPhase;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.antiPhase.render.is623();
    }
}
