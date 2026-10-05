package me.mioclient;

import me.mioclient.module.player.SpeedMine;

/* loaded from: mio-yarn.jar:me/mioclient/SpeedMinePredicate_2.class */
public class SpeedMinePredicate_2 implements java.util.function.Predicate {
    public SpeedMine speedMine;

    public SpeedMinePredicate_2(SpeedMine speedMine) {
        this.speedMine = speedMine;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.speedMine.autoSwap.getValue() == SpeedMineMode_3.NORMAL;
    }
}
