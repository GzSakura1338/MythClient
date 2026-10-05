package me.mioclient;

import me.mioclient.module.player.SpeedMine;

/* loaded from: mio-yarn.jar:me/mioclient/SpeedMinePredicate_5.class */
public class SpeedMinePredicate_5 implements java.util.function.Predicate {
    public SpeedMine speedMine;

    public SpeedMinePredicate_5(SpeedMine speedMine) {
        this.speedMine = speedMine;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.speedMine.autoSwap2.is623();
    }
}
