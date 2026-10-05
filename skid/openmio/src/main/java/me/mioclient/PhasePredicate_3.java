package me.mioclient;

import me.mioclient.module.exploit.Phase;

/* loaded from: mio-yarn.jar:me/mioclient/PhasePredicate_3.class */
public class PhasePredicate_3 implements java.util.function.Predicate {
    public Phase phase;

    public PhasePredicate_3(Phase phase) {
        this.phase = phase;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.phase.mode.getValue() == Phase.PhasePredicateMode.PEARL;
    }
}
