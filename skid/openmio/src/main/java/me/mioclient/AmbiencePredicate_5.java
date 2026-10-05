package me.mioclient;

import me.mioclient.module.render.Ambience;

/* loaded from: mio-yarn.jar:me/mioclient/AmbiencePredicate_5.class */
public class AmbiencePredicate_5 implements java.util.function.Predicate {
    public Ambience ambience;

    public AmbiencePredicate_5(Ambience ambience) {
        this.ambience = ambience;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.ambience.brightness.getValue() == Ambience.MixinEntityRendererMode.SCREEN;
    }
}
