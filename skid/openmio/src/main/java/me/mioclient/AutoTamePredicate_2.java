package me.mioclient;

import me.mioclient.module.player.AutoTame;

/* loaded from: mio-yarn.jar:me/mioclient/AutoTamePredicate_2.class */
public class AutoTamePredicate_2 implements java.util.function.Predicate {
    public AutoTame autoTame;

    public AutoTamePredicate_2(AutoTame autoTame) {
        this.autoTame = autoTame;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.autoTame.targets.is623();
    }
}
