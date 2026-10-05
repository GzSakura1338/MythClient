package me.mioclient;

import me.mioclient.module.render.Hitmarker;

/* loaded from: mio-yarn.jar:me/mioclient/HitmarkerPredicate_3.class */
public class HitmarkerPredicate_3 implements java.util.function.Predicate {
    public Hitmarker hitmarker;

    public HitmarkerPredicate_3(Hitmarker hitmarker) {
        this.hitmarker = hitmarker;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.hitmarker.draw.is623();
    }
}
