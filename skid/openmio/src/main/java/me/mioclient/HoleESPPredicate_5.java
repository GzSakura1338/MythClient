package me.mioclient;

import me.mioclient.module.render.HoleESP;

/* loaded from: mio-yarn.jar:me/mioclient/HoleESPPredicate_5.class */
public class HoleESPPredicate_5 implements java.util.function.Predicate {
    public HoleESP holeESP;

    public HoleESPPredicate_5(HoleESP holeESP) {
        this.holeESP = holeESP;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.holeESP.unsafe.is623();
    }
}
