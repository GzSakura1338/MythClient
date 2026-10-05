package me.mioclient;

import me.mioclient.module.movement.Sprint;

/* loaded from: mio-yarn.jar:me/mioclient/SprintPredicate_4.class */
public class SprintPredicate_4 implements java.util.function.Predicate {
    public Sprint sprint;

    public SprintPredicate_4(Sprint sprint) {
        this.sprint = sprint;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.sprint.mode.getValue() == Sprint.SprintPredicateMode.INSTANT;
    }
}
