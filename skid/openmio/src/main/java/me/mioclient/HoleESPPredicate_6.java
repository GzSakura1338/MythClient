package me.mioclient;

import me.mioclient.module.render.HoleESP;

/* loaded from: mio-yarn.jar:me/mioclient/HoleESPPredicate_6.class */
public class HoleESPPredicate_6 implements java.util.function.Predicate {
    public HoleESP holeESP;

    public HoleESPPredicate_6(HoleESP holeESP) {
        this.holeESP = holeESP;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.holeESP.fade.is623();
    }
}
