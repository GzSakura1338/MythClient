package me.mioclient;

import me.mioclient.module.combat.Aura;

/* loaded from: mio-yarn.jar:me/mioclient/AuraPredicate.class */
public class AuraPredicate implements java.util.function.Predicate {
    public Aura aura;

    public AuraPredicate(Aura aura) {
        this.aura = aura;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.aura.weapon.getValue() == Aura.AuraPredicateMode.SWAP;
    }
}
