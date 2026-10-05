package me.mioclient;

import me.mioclient.module.render.Chams;

/* loaded from: mio-yarn.jar:me/mioclient/ChamsPredicate.class */
public class ChamsPredicate implements java.util.function.Predicate {
    public Chams chams;

    public ChamsPredicate(Chams chams) {
        this.chams = chams;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.chams.speed.is2327();
    }
}
