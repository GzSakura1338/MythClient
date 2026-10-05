package me.mioclient;

import me.mioclient.module.combat.AnchorAura;

/* loaded from: mio-yarn.jar:me/mioclient/AnchorAuraPredicate.class */
public class AnchorAuraPredicate implements java.util.function.Predicate {
    public AnchorAura anchorAura;

    public AnchorAuraPredicate(AnchorAura anchorAura) {
        this.anchorAura = anchorAura;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.anchorAura.render.is623();
    }
}
