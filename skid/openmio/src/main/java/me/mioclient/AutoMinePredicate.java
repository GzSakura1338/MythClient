package me.mioclient;

import me.mioclient.module.combat.AutoMine;

/* loaded from: mio-yarn.jar:me/mioclient/AutoMinePredicate.class */
public class AutoMinePredicate implements java.util.function.Predicate {
    public AutoMine autoMine;

    public AutoMinePredicate(AutoMine autoMine) {
        this.autoMine = autoMine;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return false;
    }
}
