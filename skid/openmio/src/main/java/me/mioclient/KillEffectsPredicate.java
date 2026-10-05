package me.mioclient;

import me.mioclient.module.misc.KillEffects;

/* loaded from: mio-yarn.jar:me/mioclient/KillEffectsPredicate.class */
public class KillEffectsPredicate implements java.util.function.Predicate {
    public KillEffects killEffects;

    public KillEffectsPredicate(KillEffects killEffects) {
        this.killEffects = killEffects;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.killEffects.killSound.is623() && this.killEffects.self.is623();
    }
}
