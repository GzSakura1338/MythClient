package me.mioclient;

import me.mioclient.module.render.Trails;

/* loaded from: mio-yarn.jar:me/mioclient/TrailsPredicate_2.class */
public class TrailsPredicate_2 implements java.util.function.Predicate {
    public Trails trails;

    public TrailsPredicate_2(Trails trails) {
        this.trails = trails;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.trails.targets.is623();
    }
}
