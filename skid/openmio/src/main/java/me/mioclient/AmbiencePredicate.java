package me.mioclient;

import me.mioclient.module.render.Ambience;

/* loaded from: mio-yarn.jar:me/mioclient/AmbiencePredicate.class */
public class AmbiencePredicate implements java.util.function.Predicate {
    public Ambience ambience;

    public AmbiencePredicate(Ambience ambience) {
        this.ambience = ambience;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.ambience.weather.getValue() == Ambience.AmbiencePredicateMode.DUSTY;
    }
}
