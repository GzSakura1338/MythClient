package me.mioclient;

import me.mioclient.module.player.Scaffold;

/* loaded from: mio-yarn.jar:me/mioclient/ScaffoldPredicate_3.class */
public class ScaffoldPredicate_3 implements java.util.function.Predicate {
    public Scaffold scaffold;

    public ScaffoldPredicate_3(Scaffold scaffold) {
        this.scaffold = scaffold;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.scaffold.render.is623();
    }
}
