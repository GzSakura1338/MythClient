package me.mioclient;

import me.mioclient.module.render.Trails;

/* loaded from: mio-yarn.jar:me/mioclient/TrailsPredicate_3.class */
public class TrailsPredicate_3 implements java.util.function.Predicate {
    public Trails trails;

    public TrailsPredicate_3(Trails trails) {
        this.trails = trails;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.trails.targets.is623();
    }
}
