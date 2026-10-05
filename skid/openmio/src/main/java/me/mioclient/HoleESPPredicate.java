package me.mioclient;

import me.mioclient.module.render.HoleESP;

/* loaded from: mio-yarn.jar:me/mioclient/HoleESPPredicate.class */
public class HoleESPPredicate implements java.util.function.Predicate {
    public HoleESP holeESP;

    public HoleESPPredicate(HoleESP holeESP) {
        this.holeESP = holeESP;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.holeESP.trapped.is623();
    }
}
