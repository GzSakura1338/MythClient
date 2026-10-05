package me.mioclient;

import me.mioclient.module.combat.Aura;

/* loaded from: mio-yarn.jar:me/mioclient/AuraPredicate_14.class */
public class AuraPredicate_14 implements java.util.function.Predicate {
    public Aura aura;

    public AuraPredicate_14(Aura aura) {
        this.aura = aura;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.aura.targets.is623();
    }
}
