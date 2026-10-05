package me.mioclient;

import me.mioclient.module.player.SpeedMine;

/* loaded from: mio-yarn.jar:me/mioclient/SpeedMinePredicate.class */
public class SpeedMinePredicate implements java.util.function.Predicate {
    public SpeedMine speedMine;

    public SpeedMinePredicate(SpeedMine speedMine) {
        this.speedMine = speedMine;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.speedMine.rotate.is623();
    }
}
