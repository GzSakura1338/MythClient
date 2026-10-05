package me.mioclient;

import me.mioclient.module.movement.Step;

/* loaded from: mio-yarn.jar:me/mioclient/StepPredicate.class */
public class StepPredicate implements java.util.function.Predicate {
    public Step step;

    public StepPredicate(Step step) {
        this.step = step;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.step.mode.getValue() == Step.StepMode.NORMAL;
    }
}
