package me.mioclient;

import me.mioclient.module.player.AutoTame;

/* loaded from: mio-yarn.jar:me/mioclient/AutoTamePredicate.class */
public class AutoTamePredicate implements java.util.function.Predicate {
    public AutoTame autoTame;

    public AutoTamePredicate(AutoTame autoTame) {
        this.autoTame = autoTame;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.autoTame.targets.is623();
    }
}
