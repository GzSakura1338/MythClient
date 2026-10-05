package me.mioclient;

import me.mioclient.module.render.Hitmarker;

/* loaded from: mio-yarn.jar:me/mioclient/HitmarkerPredicate_7.class */
public class HitmarkerPredicate_7 implements java.util.function.Predicate {
    public Hitmarker hitmarker;

    public HitmarkerPredicate_7(Hitmarker hitmarker) {
        this.hitmarker = hitmarker;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.hitmarker.draw.is623();
    }
}
