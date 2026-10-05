package me.mioclient;

import me.mioclient.module.exploit.Timer;

/* loaded from: mio-yarn.jar:me/mioclient/TimerPredicate_2.class */
public class TimerPredicate_2 implements java.util.function.Predicate {
    public Timer timer;

    public TimerPredicate_2(Timer timer) {
        this.timer = timer;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.timer.mode.getValue() == Timer.TimerPredicateMode.timerPredicateMode2;
    }
}
