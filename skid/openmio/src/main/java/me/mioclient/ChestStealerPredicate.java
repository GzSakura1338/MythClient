package me.mioclient;

import me.mioclient.module.player.ChestStealer;

/* loaded from: mio-yarn.jar:me/mioclient/ChestStealerPredicate.class */
public class ChestStealerPredicate implements java.util.function.Predicate {
    public ChestStealer chestStealer;

    public ChestStealerPredicate(ChestStealer chestStealer) {
        this.chestStealer = chestStealer;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.chestStealer.select.getValue() != ScaffoldMode_2.ANY;
    }
}
