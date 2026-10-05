package me.mioclient;

import me.mioclient.module.combat.HoleFill;

/* loaded from: mio-yarn.jar:me/mioclient/HoleFillPredicate_2.class */
public class HoleFillPredicate_2 implements java.util.function.Predicate {
    public HoleFill holeFill;

    public HoleFillPredicate_2(HoleFill holeFill) {
        this.holeFill = holeFill;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.holeFill.mode.getValue() == HoleFill.HoleFillPredicateMode.SMART;
    }
}
