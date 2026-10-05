package me.mioclient;

import me.mioclient.module.combat.AntiPhase;

/* loaded from: mio-yarn.jar:me/mioclient/AntiPhasePredicate.class */
public class AntiPhasePredicate implements java.util.function.Predicate {
    public AntiPhase antiPhase;

    public AntiPhasePredicate(AntiPhase antiPhase) {
        this.antiPhase = antiPhase;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.antiPhase.render.is623();
    }
}
