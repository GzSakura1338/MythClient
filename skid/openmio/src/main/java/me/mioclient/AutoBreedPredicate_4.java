package me.mioclient;

import me.mioclient.module.player.AutoBreed;

/* loaded from: mio-yarn.jar:me/mioclient/AutoBreedPredicate_4.class */
public class AutoBreedPredicate_4 implements java.util.function.Predicate {
    public AutoBreed autoBreed;

    public AutoBreedPredicate_4(AutoBreed autoBreed) {
        this.autoBreed = autoBreed;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.autoBreed.targets.is623();
    }
}
