package me.mioclient;

import me.mioclient.module.player.Scaffold;

/* loaded from: mio-yarn.jar:me/mioclient/ScaffoldPredicate_4.class */
public class ScaffoldPredicate_4 implements java.util.function.Predicate {
    public Scaffold scaffold;

    public ScaffoldPredicate_4(Scaffold scaffold) {
        this.scaffold = scaffold;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.scaffold.render.is623();
    }
}
