package me.mioclient;

import me.mioclient.module.combat.AutoCrystal;

/* loaded from: mio-yarn.jar:me/mioclient/AutoCrystalPredicate_2.class */
public class AutoCrystalPredicate_2 implements java.util.function.Predicate {
    public AutoCrystal autoCrystal;

    public AutoCrystalPredicate_2(AutoCrystal autoCrystal) {
        this.autoCrystal = autoCrystal;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.autoCrystal.instant.getValue() != AutoCrystalMode.NONE;
    }
}
