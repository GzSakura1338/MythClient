package me.mioclient;

import me.mioclient.module.combat.BowAim;

/* loaded from: mio-yarn.jar:me/mioclient/BowAimPredicate_4.class */
public class BowAimPredicate_4 implements java.util.function.Predicate {
    public BowAim bowAim;

    public BowAimPredicate_4(BowAim bowAim) {
        this.bowAim = bowAim;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.bowAim.targets.is623() && this.bowAim.players.is623();
    }
}
