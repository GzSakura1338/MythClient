package me.mioclient;

import me.mioclient.module.player.SpeedMine;

/* loaded from: mio-yarn.jar:me/mioclient/SpeedMinePredicate_12.class */
public class SpeedMinePredicate_12 implements java.util.function.Predicate {
    public SpeedMine speedMine;

    public SpeedMinePredicate_12(SpeedMine speedMine) {
        this.speedMine = speedMine;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.speedMine.autoSwap.getValue() == SpeedMineMode_3.SILENT && this.speedMine.autoSwap2.is623();
    }
}
