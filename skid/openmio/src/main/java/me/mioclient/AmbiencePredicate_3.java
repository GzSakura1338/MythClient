package me.mioclient;

import me.mioclient.module.render.Ambience;

/* loaded from: mio-yarn.jar:me/mioclient/AmbiencePredicate_3.class */
public class AmbiencePredicate_3 implements java.util.function.Predicate {
    public Ambience ambience;

    public AmbiencePredicate_3(Ambience ambience) {
        this.ambience = ambience;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.ambience.brightness.getValue() == Ambience.MixinEntityRendererMode.SKY;
    }
}
