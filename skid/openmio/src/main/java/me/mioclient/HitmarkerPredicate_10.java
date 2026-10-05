package me.mioclient;

import me.mioclient.module.render.Hitmarker;

/* loaded from: mio-yarn.jar:me/mioclient/HitmarkerPredicate_10.class */
public class HitmarkerPredicate_10 implements java.util.function.Predicate {
    public Hitmarker hitmarker;

    public HitmarkerPredicate_10(Hitmarker hitmarker) {
        this.hitmarker = hitmarker;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.hitmarker.targets.is623();
    }
}
