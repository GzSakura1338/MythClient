package me.mioclient;

import me.mioclient.module.render.Hitmarker;

/* loaded from: mio-yarn.jar:me/mioclient/HitmarkerPredicate.class */
public class HitmarkerPredicate implements java.util.function.Predicate {
    public Hitmarker hitmarker;

    public HitmarkerPredicate(Hitmarker hitmarker) {
        this.hitmarker = hitmarker;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.hitmarker.draw.is623();
    }
}
