package me.mioclient;

import me.mioclient.module.render.Trails;

/* loaded from: mio-yarn.jar:me/mioclient/TrailsPredicate_4.class */
public class TrailsPredicate_4 implements java.util.function.Predicate {
    public Trails trails;

    public TrailsPredicate_4(Trails trails) {
        this.trails = trails;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.trails.targets.is623();
    }
}
