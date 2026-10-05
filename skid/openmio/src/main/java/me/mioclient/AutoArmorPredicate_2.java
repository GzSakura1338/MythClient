package me.mioclient;

import me.mioclient.module.combat.AutoArmor;

/* loaded from: mio-yarn.jar:me/mioclient/AutoArmorPredicate_2.class */
public class AutoArmorPredicate_2 implements java.util.function.Predicate {
    public AutoArmor autoArmor;

    public AutoArmorPredicate_2(AutoArmor autoArmor) {
        this.autoArmor = autoArmor;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.autoArmor.safe.is623();
    }
}
