package me.mioclient;

import me.mioclient.module.player.AutoTame;

/* loaded from: mio-yarn.jar:me/mioclient/AutoTamePredicate_3.class */
public class AutoTamePredicate_3 implements java.util.function.Predicate {
    public AutoTame autoTame;

    public AutoTamePredicate_3(AutoTame autoTame) {
        this.autoTame = autoTame;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.autoTame.targets.is623();
    }
}
