package me.mioclient;

import me.mioclient.module.misc.KillEffects;

/* loaded from: mio-yarn.jar:me/mioclient/KillEffectsPredicate_6.class */
public class KillEffectsPredicate_6 implements java.util.function.Predicate {
    public KillEffects killEffects;

    public KillEffectsPredicate_6(KillEffects killEffects) {
        this.killEffects = killEffects;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.killEffects.killStreak.is623();
    }
}
