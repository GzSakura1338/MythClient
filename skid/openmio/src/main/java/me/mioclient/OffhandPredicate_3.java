package me.mioclient;

import me.mioclient.module.combat.Offhand;

/* loaded from: mio-yarn.jar:me/mioclient/OffhandPredicate_3.class */
public class OffhandPredicate_3 implements java.util.function.Predicate {
    public Offhand offhand;

    public OffhandPredicate_3(Offhand offhand) {
        this.offhand = offhand;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.offhand.item.getValue() == OffhandMode.Gapple || this.offhand.gappleBind.is623();
    }
}
