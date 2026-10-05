package me.mioclient;

import me.mioclient.module.render.VoidESP;

/* loaded from: mio-yarn.jar:me/mioclient/VoidESPPredicate_3.class */
public class VoidESPPredicate_3 implements java.util.function.Predicate {
    public VoidESP voidESP;

    public VoidESPPredicate_3(VoidESP voidESP) {
        this.voidESP = voidESP;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.voidESP.fade.getValue().booleanValue();
    }
}
