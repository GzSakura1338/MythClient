package me.mioclient;

import me.mioclient.module.movement.Step;

/* loaded from: mio-yarn.jar:me/mioclient/StepPredicate_2.class */
public class StepPredicate_2 implements java.util.function.Predicate {
    public Step step;

    public StepPredicate_2(Step step) {
        this.step = step;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.step.mode.getValue() == Step.StepMode.NORMAL && this.step.useTimer.is623();
    }
}
