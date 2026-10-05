package me.mioclient;

import me.mioclient.module.combat.AutoArmor;

/* loaded from: mio-yarn.jar:me/mioclient/AutoArmorPredicate_3.class */
public class AutoArmorPredicate_3 implements java.util.function.Predicate {
    public AutoArmor autoArmor;

    public AutoArmorPredicate_3(AutoArmor autoArmor) {
        this.autoArmor = autoArmor;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.autoArmor.inRange.is623();
    }
}
