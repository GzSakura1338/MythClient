package me.mioclient;

import me.mioclient.module.movement.Sprint;

/* loaded from: mio-yarn.jar:me/mioclient/SprintPredicate.class */
public class SprintPredicate implements java.util.function.Predicate {
    public Sprint sprint;

    public SprintPredicate(Sprint sprint) {
        this.sprint = sprint;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.sprint.mode.getValue() != Sprint.SprintPredicateMode.INSTANT;
    }
}
