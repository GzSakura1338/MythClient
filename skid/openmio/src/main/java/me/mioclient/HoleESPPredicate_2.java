package me.mioclient;

import me.mioclient.module.render.HoleESP;

/* loaded from: mio-yarn.jar:me/mioclient/HoleESPPredicate_2.class */
public class HoleESPPredicate_2 implements java.util.function.Predicate {
    public HoleESP holeESP;

    public HoleESPPredicate_2(HoleESP holeESP) {
        this.holeESP = holeESP;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.holeESP.trapped.is623();
    }
}
