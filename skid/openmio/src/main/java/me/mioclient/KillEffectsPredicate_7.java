package me.mioclient;

import me.mioclient.module.misc.KillEffects;

/* loaded from: mio-yarn.jar:me/mioclient/KillEffectsPredicate_7.class */
public class KillEffectsPredicate_7 implements java.util.function.Predicate {
    public KillEffects killEffects;

    public KillEffectsPredicate_7(KillEffects killEffects) {
        this.killEffects = killEffects;
    }

    @Override // java.util.function.Predicate
    public boolean test(Object obj) {
        return this.killEffects.killSound.is623() && this.killEffects.players.is623();
    }
}
