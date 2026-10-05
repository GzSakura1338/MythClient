package me.mioclient;

import me.mioclient.module.combat.Arrows;

/* loaded from: mio-yarn.jar:me/mioclient/ArrowsPredicate.class */
public class ArrowsPredicate implements java.util.function.Predicate {
    public Arrows arrows;

    public ArrowsPredicate(Arrows arrows) {
        this.arrows = arrows;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return !this.arrows.autoShoot.getValue().booleanValue();
    }
}
