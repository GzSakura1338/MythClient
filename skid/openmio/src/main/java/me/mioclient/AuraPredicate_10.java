package me.mioclient;

import me.mioclient.module.combat.Aura;

/* loaded from: mio-yarn.jar:me/mioclient/AuraPredicate_10.class */
public class AuraPredicate_10 implements java.util.function.Predicate {
    public Aura aura;

    public AuraPredicate_10(Aura aura) {
        this.aura = aura;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return (this.aura.swap.getValue() == Aura.AuraMode.SILENT && this.aura.weapon.getValue() == Aura.AuraPredicateMode.SWAP) ? false : true;
    }
}
