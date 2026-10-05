package me.mioclient;

import me.mioclient.module.exploit.Timer;

/* loaded from: mio-yarn.jar:me/mioclient/TimerPredicate.class */
public class TimerPredicate implements java.util.function.Predicate {
    public Timer timer;

    public TimerPredicate(Timer timer) {
        this.timer = timer;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.timer.mode.getValue() == Timer.TimerPredicateMode.timerPredicateMode2;
    }
}
