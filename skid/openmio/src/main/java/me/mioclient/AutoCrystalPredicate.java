package me.mioclient;

import me.mioclient.module.combat.AutoCrystal;

/* loaded from: mio-yarn.jar:me/mioclient/AutoCrystalPredicate.class */
public class AutoCrystalPredicate implements java.util.function.Predicate {
    public AutoCrystal autoCrystal;

    public AutoCrystalPredicate(AutoCrystal autoCrystal) {
        this.autoCrystal = autoCrystal;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.autoCrystal.autoSwap.getValue() == AutoCrystalMode_2.NORMAL;
    }
}
