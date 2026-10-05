package me.mioclient;

import me.mioclient.module.render.Ambience;

/* loaded from: mio-yarn.jar:me/mioclient/AmbiencePredicate_2.class */
public class AmbiencePredicate_2 implements java.util.function.Predicate {
    public Ambience ambience;

    public AmbiencePredicate_2(Ambience ambience) {
        this.ambience = ambience;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.ambience.worldTime.is623() && !this.ambience.sync.getValue().booleanValue();
    }
}
