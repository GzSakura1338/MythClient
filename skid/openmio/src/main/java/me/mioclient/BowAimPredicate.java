package me.mioclient;

import me.mioclient.module.combat.BowAim;

/* loaded from: mio-yarn.jar:me/mioclient/BowAimPredicate.class */
public class BowAimPredicate implements java.util.function.Predicate {
    public BowAim bowAim;

    public BowAimPredicate(BowAim bowAim) {
        this.bowAim = bowAim;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.bowAim.targets.is623();
    }
}
