package me.mioclient;

import me.mioclient.module.player.Scaffold;

/* loaded from: mio-yarn.jar:me/mioclient/ScaffoldPredicate.class */
public class ScaffoldPredicate implements java.util.function.Predicate {
    public Scaffold scaffold;

    public ScaffoldPredicate(Scaffold scaffold) {
        this.scaffold = scaffold;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.scaffold.render.is623() && this.scaffold.fade.getValue().booleanValue();
    }
}
