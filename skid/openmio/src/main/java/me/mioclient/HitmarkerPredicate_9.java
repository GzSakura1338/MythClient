package me.mioclient;

import me.mioclient.module.render.Hitmarker;

/* loaded from: mio-yarn.jar:me/mioclient/HitmarkerPredicate_9.class */
public class HitmarkerPredicate_9 implements java.util.function.Predicate {
    public Hitmarker hitmarker;

    public HitmarkerPredicate_9(Hitmarker hitmarker) {
        this.hitmarker = hitmarker;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.hitmarker.sound2.is623();
    }
}
