package me.mioclient;

import me.mioclient.module.combat.AutoArmor;

/* loaded from: mio-yarn.jar:me/mioclient/AutoArmorPredicate.class */
public class AutoArmorPredicate implements java.util.function.Predicate {
    public AutoArmor autoArmor;

    public AutoArmorPredicate(AutoArmor autoArmor) {
        this.autoArmor = autoArmor;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.autoArmor.inRange.is623();
    }
}
