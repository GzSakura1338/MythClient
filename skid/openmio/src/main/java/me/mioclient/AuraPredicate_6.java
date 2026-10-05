package me.mioclient;

import me.mioclient.module.combat.Aura;

/* loaded from: mio-yarn.jar:me/mioclient/AuraPredicate_6.class */
public class AuraPredicate_6 implements java.util.function.Predicate {
    public Aura aura;

    public AuraPredicate_6(Aura aura) {
        this.aura = aura;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.aura.mace.is2349() && (this.aura.mace.getValue() == Aura.AuraMode_2.DENSITY || this.aura.mace.getValue() == Aura.AuraMode_2.SMART);
    }
}
