package me.mioclient;

import me.mioclient.module.movement.Speed;

/* loaded from: mio-yarn.jar:me/mioclient/SpeedPredicate_5.class */
public class SpeedPredicate_5 implements java.util.function.Predicate {
    public Speed speed;

    public SpeedPredicate_5(Speed speed) {
        this.speed = speed;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return (this.speed.mode.getValue() == Speed.SpeedPredicateMode.speedPredicateMode2 || this.speed.mode.getValue() == Speed.SpeedPredicateMode.speedPredicateMode5) ? false : true;
    }
}
