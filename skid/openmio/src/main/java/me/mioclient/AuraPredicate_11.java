package me.mioclient;

import me.mioclient.module.combat.Aura;

/* loaded from: mio-yarn.jar:me/mioclient/AuraPredicate_11.class */
public class AuraPredicate_11 implements java.util.function.Predicate {
    public Aura aura;

    public AuraPredicate_11(Aura aura) {
        this.aura = aura;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return BaritoneHelper_3.obstaclePasserHelper.is709();
    }
}
