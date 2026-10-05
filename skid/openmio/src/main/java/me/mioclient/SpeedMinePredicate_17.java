package me.mioclient;

import me.mioclient.module.player.SpeedMine;

/* loaded from: mio-yarn.jar:me/mioclient/SpeedMinePredicate_17.class */
public class SpeedMinePredicate_17 implements java.util.function.Predicate {
    public SpeedMine speedMine;

    public SpeedMinePredicate_17(SpeedMine speedMine) {
        this.speedMine = speedMine;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.speedMine.render.is623();
    }
}
