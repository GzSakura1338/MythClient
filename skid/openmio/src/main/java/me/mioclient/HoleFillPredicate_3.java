package me.mioclient;

import me.mioclient.module.combat.HoleFill;

/* loaded from: mio-yarn.jar:me/mioclient/HoleFillPredicate_3.class */
public class HoleFillPredicate_3 implements java.util.function.Predicate {
    public HoleFill holeFill;

    public HoleFillPredicate_3(HoleFill holeFill) {
        this.holeFill = holeFill;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.holeFill.extrapolation.is623();
    }
}
