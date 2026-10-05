package me.mioclient;

import me.mioclient.module.movement.AntiVoid;

/* loaded from: mio-yarn.jar:me/mioclient/AntiVoidPredicate.class */
public class AntiVoidPredicate implements java.util.function.Predicate {
    public AntiVoid antiVoid;

    public AntiVoidPredicate(AntiVoid antiVoid) {
        this.antiVoid = antiVoid;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.antiVoid.mode.getValue() == AntiVoid.AntiVoidMode.CANCEL;
    }
}
