package me.mioclient;

import me.mioclient.module.exploit.Phase;

/* loaded from: mio-yarn.jar:me/mioclient/PhasePredicate.class */
public class PhasePredicate implements java.util.function.Predicate {
    public Phase phase;

    public PhasePredicate(Phase phase) {
        this.phase = phase;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.phase.mode.getValue() == Phase.PhasePredicateMode.PEARL;
    }
}
