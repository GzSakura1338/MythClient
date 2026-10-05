package me.mioclient;

import me.mioclient.module.movement.HoleSnap;

/* loaded from: mio-yarn.jar:me/mioclient/HoleSnapPredicate.class */
public class HoleSnapPredicate implements java.util.function.Predicate {
    public HoleSnap holeSnap;

    public HoleSnapPredicate(HoleSnap holeSnap) {
        this.holeSnap = holeSnap;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return !SearchHelper4_8.is724();
    }
}
