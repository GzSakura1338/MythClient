package me.mioclient;

import me.mioclient.module.combat.Blocker;

/* loaded from: mio-yarn.jar:me/mioclient/BlockerPredicate.class */
public class BlockerPredicate implements java.util.function.Predicate {
    public Blocker blocker;

    public BlockerPredicate(Blocker blocker) {
        this.blocker = blocker;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.blocker.offsets.getValue() != BlockerPredicateMode.CEV;
    }
}
