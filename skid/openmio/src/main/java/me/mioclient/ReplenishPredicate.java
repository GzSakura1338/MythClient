package me.mioclient;

import me.mioclient.module.player.Replenish;

/* loaded from: mio-yarn.jar:me/mioclient/ReplenishPredicate.class */
public class ReplenishPredicate implements java.util.function.Predicate {
    public Replenish replenish;

    public ReplenishPredicate(Replenish replenish) {
        this.replenish = replenish;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.replenish.limit.is623();
    }
}
