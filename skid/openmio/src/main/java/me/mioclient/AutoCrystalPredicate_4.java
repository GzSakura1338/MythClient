package me.mioclient;

import me.mioclient.module.combat.AutoCrystal;

/* loaded from: mio-yarn.jar:me/mioclient/AutoCrystalPredicate_4.class */
public class AutoCrystalPredicate_4 implements java.util.function.Predicate {
    public AutoCrystal autoCrystal;

    public AutoCrystalPredicate_4(AutoCrystal autoCrystal) {
        this.autoCrystal = autoCrystal;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.autoCrystal.autoSwap.getValue() == AutoCrystalMode_2.NORMAL;
    }
}
