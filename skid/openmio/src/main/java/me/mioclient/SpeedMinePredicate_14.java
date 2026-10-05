package me.mioclient;

import me.mioclient.module.player.SpeedMine;

/* loaded from: mio-yarn.jar:me/mioclient/SpeedMinePredicate_14.class */
public class SpeedMinePredicate_14 implements java.util.function.Predicate {
    public SpeedMine speedMine;

    public SpeedMinePredicate_14(SpeedMine speedMine) {
        this.speedMine = speedMine;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.speedMine.rebreak2.getValue() == SpeedMineMode_2.INSTANT && this.speedMine.rebreak.is623();
    }
}
