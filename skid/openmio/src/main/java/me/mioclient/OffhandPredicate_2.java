package me.mioclient;

import me.mioclient.module.combat.Offhand;

/* loaded from: mio-yarn.jar:me/mioclient/OffhandPredicate_2.class */
public class OffhandPredicate_2 implements java.util.function.Predicate {
    public Offhand offhand;

    public OffhandPredicate_2(Offhand offhand) {
        this.offhand = offhand;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.offhand.gappleBind.is623();
    }
}
