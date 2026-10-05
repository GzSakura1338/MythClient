package me.mioclient;

import me.mioclient.module.player.AutoFarm;

/* loaded from: mio-yarn.jar:me/mioclient/AutoFarmPredicate_5.class */
public class AutoFarmPredicate_5 implements java.util.function.Predicate {
    public AutoFarm autoFarm;

    public AutoFarmPredicate_5(AutoFarm autoFarm) {
        this.autoFarm = autoFarm;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.autoFarm.remove.is623();
    }
}
