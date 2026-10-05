package me.mioclient;

import me.mioclient.module.combat.AutoExp;

/* loaded from: mio-yarn.jar:me/mioclient/AutoExpPredicate.class */
public class AutoExpPredicate implements java.util.function.Predicate {
    public AutoExp autoExp;

    public AutoExpPredicate(AutoExp autoExp) {
        this.autoExp = autoExp;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.autoExp.stop.is623();
    }
}
