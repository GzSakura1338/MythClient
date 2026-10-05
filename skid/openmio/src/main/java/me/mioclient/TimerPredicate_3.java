package me.mioclient;

import me.mioclient.module.exploit.Timer;

/* loaded from: mio-yarn.jar:me/mioclient/TimerPredicate_3.class */
public class TimerPredicate_3 implements java.util.function.Predicate {
    public Timer timer;

    public TimerPredicate_3(Timer timer) {
        this.timer = timer;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.timer.mode.getValue() == Timer.TimerPredicateMode.timerPredicateMode2;
    }
}
