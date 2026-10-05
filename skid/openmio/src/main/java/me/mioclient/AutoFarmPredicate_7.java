package me.mioclient;

import me.mioclient.module.player.AutoFarm;

/* loaded from: mio-yarn.jar:me/mioclient/AutoFarmPredicate_7.class */
public class AutoFarmPredicate_7 implements java.util.function.Predicate {
    public AutoFarm autoFarm;

    public AutoFarmPredicate_7(AutoFarm autoFarm) {
        this.autoFarm = autoFarm;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.autoFarm.mode.getValue() == AutoFarm.AutoFarmMode.autoFarmMode2;
    }
}
