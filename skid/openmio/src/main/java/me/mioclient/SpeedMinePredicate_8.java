package me.mioclient;

import me.mioclient.module.player.SpeedMine;

/* loaded from: mio-yarn.jar:me/mioclient/SpeedMinePredicate_8.class */
public class SpeedMinePredicate_8 implements java.util.function.Predicate {
    public SpeedMine speedMine;

    public SpeedMinePredicate_8(SpeedMine speedMine) {
        this.speedMine = speedMine;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.speedMine.render.is623() && this.speedMine.colorMode.getValue() == SpeedMineMode_5.CUSTOM;
    }
}
