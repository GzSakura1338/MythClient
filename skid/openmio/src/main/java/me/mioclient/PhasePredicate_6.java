package me.mioclient;

import me.mioclient.module.exploit.Phase;

/* loaded from: mio-yarn.jar:me/mioclient/PhasePredicate_6.class */
public class PhasePredicate_6 implements java.util.function.Predicate {
    public Phase phase;

    public PhasePredicate_6(Phase phase) {
        this.phase = phase;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.phase.mode.getValue() == Phase.PhasePredicateMode.CLIP;
    }
}
