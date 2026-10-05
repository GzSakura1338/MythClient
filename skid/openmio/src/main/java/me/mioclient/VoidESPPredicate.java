package me.mioclient;

import me.mioclient.module.render.VoidESP;

/* loaded from: mio-yarn.jar:me/mioclient/VoidESPPredicate.class */
public class VoidESPPredicate implements java.util.function.Predicate {
    public VoidESP voidESP;

    public VoidESPPredicate(VoidESP voidESP) {
        this.voidESP = voidESP;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.voidESP.colors.is623();
    }
}
