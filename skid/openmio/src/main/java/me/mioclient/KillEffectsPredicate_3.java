package me.mioclient;

import me.mioclient.module.misc.KillEffects;

/* loaded from: mio-yarn.jar:me/mioclient/KillEffectsPredicate_3.class */
public class KillEffectsPredicate_3 implements java.util.function.Predicate {
    public KillEffects killEffects;

    public KillEffectsPredicate_3(KillEffects killEffects) {
        this.killEffects = killEffects;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.killEffects.killSound.is623();
    }
}
